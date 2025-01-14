#include "pid_controller.hpp"
#include "control_decisions.hpp"

#include "update_controller.hpp"
#include "registry_entries.hpp"
#include "data_registry.hpp"

#include "logging.hpp"

// Forward declarations
telemetrics_data get_metrics();
regulator_param_data get_params();
regulator_sample_data get_samples();
mission_data get_mission_data();
std::vector<path_step> get_path();
mode get_mode();
control_change_data get_request();
sensor_data get_sensor_data();

void set_output(regulator_out_data);
void set_samples(regulator_sample_data);
void set_mission_data(mission_data);
void set_path(std::vector<path_step>);


void pid_ctrl_thread_main(const std::atomic_bool& running){

  update_controller upd_controller{};

  auto previous_time = std::chrono::steady_clock::now();

  while (running) {

    auto current_time = std::chrono::steady_clock::now();

    upd_controller.start();

    uint8_t mode = get_mode();

    // In manual mode, just forward the requested output
    if (mode == MANUAL) {
        control_change_data requested = get_request();
        regulator_out_data reg_out =
        {
            .angle = (double)requested.angle,
            .speed = (double)requested.speed
        };
        set_output(reg_out);

        upd_controller.wait();
        continue;
    }

    mission_data mission_data = get_mission_data();
    // Check if we have any current missions to run
    if (mission_data.missions.empty()) {
        upd_controller.wait();
        continue;
    }

    // Get all data for the regulator
    telemetrics_data metrics = get_metrics();
    regulator_param_data params = get_params();
    regulator_sample_data samples = get_samples();
    std::vector<path_step> path = get_path();
    sensor_data sensor_data = get_sensor_data();

    // Set deltatime
    double dt = std::chrono::duration_cast<std::chrono::duration<double>>(current_time - previous_time).count();

    std::pair<int, int> mission = mission_data.missions[0];

    // If we are not already at the start position for some reason, go there
    if (mission_data.previous_pos != mission.first &&
            (path.empty() || path.back().node != mission.second))
    {
        edge e = mission_data.g.get_edges(mission.first)[0]; // Assuming we are never starting in a crossing
        path = find_shortest_path(mission_data.g, mission_data.previous_pos, e.end);
        mission = std::make_pair(mission_data.previous_pos, e.end);
        mission_data.missions.push_front(mission);
    }
    // Or if we haven't found the path yet, do it.
    else if (path.empty() || path.back().node != mission.second)
    {
        path = find_shortest_path(mission_data.g, mission_data.previous_pos,
                                    mission.second);
    }
    set_path(path);

    // Define input to the regulator
    pid_decision_in dec_in =
      {
       .metrics = metrics,
       .sensor_data = sensor_data,
       .params = params,
       .dt = dt,
       .samples = samples,
       .map.g = mission_data.g,
       .map.path = path,
       .map.previous_pos = mission_data.previous_pos,
       .map.next_pos = mission_data.next_pos,
       .map.index = mission_data.index
      };

    // Regulate
    pid_decision_return regulate = pid_decision(dec_in);

    // Stay at the end for some time, then continue
    if (regulate.index >= path.size())
    {
        regulate.speed = 0;
        regulate.mission_finished = true;
    }
    if (regulate.mission_finished){
        queue_message("MISSION FINISHED");
        mission_data.missions.pop_front();
        regulate.index = 0;
        regulate.samples.dist_stop_line = IMAGE_HEIGHT * STOP_LINE_FACTOR;
    }

    // Update current position
    mission_data.previous_pos = regulate.previous_pos;
    mission_data.next_pos = regulate.next_pos;
    mission_data.index = regulate.index;

    samples = regulate.samples;

    // Bind the angle
    if (regulate.angle > MAX_INPUT_ANGLE) regulate.angle = MAX_INPUT_ANGLE;
    else if (regulate.angle < -MAX_INPUT_ANGLE) regulate.angle = -MAX_INPUT_ANGLE;

    // Bind the speed
    if (regulate.speed > MAX_INPUT_SPEED) regulate.speed = MAX_INPUT_SPEED;
    else if (regulate.angle < -MAX_INPUT_SPEED) regulate.speed = -MAX_INPUT_SPEED;

    regulator_out_data reg_out =
      {
       .angle = regulate.angle,
       .speed = regulate.speed
      };

    // Send output
    set_output(reg_out);
    set_samples(samples);
    set_mission_data(mission_data);

    if (regulate.mission_finished)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    previous_time = current_time;
    upd_controller.wait();
  }

}

// Get metrics from global registry
telemetrics_data get_metrics(){
    telemetrics_data metrics;

    telemetrics_data *registry_entry =
      (telemetrics_data*) data_registry::get_instance().
      acquire_data(TELEMETRICS_DATA_ID);
    metrics = *registry_entry;
    data_registry::get_instance().release_data(TELEMETRICS_DATA_ID);
    return metrics;
}

// Get params from global registry
regulator_param_data get_params(){
    regulator_param_data params;

    regulator_param_data *registry_entry =
      (regulator_param_data*) data_registry::get_instance().
      acquire_data(REGULATOR_PARAM_DATA_ID);
    params = *registry_entry;
    data_registry::get_instance().release_data(REGULATOR_PARAM_DATA_ID);
    return params;
}

// Get params from global registry
regulator_sample_data get_samples(){
    regulator_sample_data samples;

    regulator_sample_data *registry_entry =
      (regulator_sample_data*) data_registry::get_instance().
      acquire_data(REGULATOR_SAMPLE_DATA_ID);
    samples = *registry_entry;
    data_registry::get_instance().release_data(REGULATOR_SAMPLE_DATA_ID);
    return samples;
}

// Get mission data from global registry
mission_data get_mission_data(){
    mission_data data;

    mission_data *registry_entry =
      (mission_data*) data_registry::get_instance().
      acquire_data(MISSION_DATA_ID);
    data = *registry_entry;
    data_registry::get_instance().release_data(MISSION_DATA_ID);
    return data;
}

// Get path from global registry
std::vector<path_step> get_path(){
    std::vector<path_step> p;

    std::vector<path_step> *registry_entry =
      (std::vector<path_step>*) data_registry::get_instance().
      acquire_data(PATH_ID);
    p = *registry_entry;
    data_registry::get_instance().release_data(PATH_ID);
    return p;
}

// Get mode from global registry
mode get_mode(){
    mode m;

    mode *registry_entry =
      (mode*) data_registry::get_instance().
      acquire_data(MODE_ID);
    m = *registry_entry;
    data_registry::get_instance().release_data(MODE_ID);
    return m;
}

// Get request from global registry
control_change_data get_request(){
    control_change_data ccd;

    control_change_data *registry_entry =
      (control_change_data*) data_registry::get_instance().
      acquire_data(CONTROL_CHANGE_DATA_ID);
    ccd = *registry_entry;
    data_registry::get_instance().release_data(CONTROL_CHANGE_DATA_ID);
    return ccd;
}

// Get sensor data from global registry
sensor_data get_sensor_data(){
    sensor_data sd;

    sensor_data *registry_entry =
      (sensor_data*) data_registry::get_instance().
      acquire_data(SENSOR_DATA_ID);
    sd = *registry_entry;
    data_registry::get_instance().release_data(SENSOR_DATA_ID);
    return sd;
}

void set_output(regulator_out_data output){
    regulator_out_data *registry_entry =
      (regulator_out_data*) data_registry::get_instance().
      acquire_data(REGULATOR_OUT_DATA_ID);
    *registry_entry = output;
    data_registry::get_instance().release_data(REGULATOR_OUT_DATA_ID);

}

void set_samples(regulator_sample_data samples){
    regulator_sample_data *registry_entry =
      (regulator_sample_data*) data_registry::get_instance().
      acquire_data(REGULATOR_SAMPLE_DATA_ID);
    *registry_entry = samples;
    data_registry::get_instance().release_data(REGULATOR_SAMPLE_DATA_ID);

}

void set_mission_data(mission_data data){
    mission_data *registry_entry =
      (mission_data*) data_registry::get_instance().
      acquire_data(MISSION_DATA_ID);
    *registry_entry = data;
    data_registry::get_instance().release_data(MISSION_DATA_ID);
}

void set_path(std::vector<path_step> path){
    std::vector<path_step> *registry_entry =
      (std::vector<path_step>*) data_registry::get_instance().
      acquire_data(PATH_ID);
    *registry_entry = path;
    data_registry::get_instance().release_data(PATH_ID);
}
