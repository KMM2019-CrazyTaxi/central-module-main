
#include "pathfinding.hpp"

#include <utility>
#include <algorithm>

#include "graph.hpp"

std::vector<path_step> find_shortest_path(graph& g, int start, int end) {

    int original_start = -1;
    bool insert_original_start = false;

    // If start and end are equal, return a full traverse
    // rather than an empty path
    if (start == end) {
        original_start = start;
        start = g.get_edges(start)[0].end;
    }


    std::unordered_map<int, int> previous;
    std::unordered_map<int, int> dist;

    // Lambda function that compares costs of nodes by binding the dist map to it
    auto comparator = [&dist](int lhs, int rhs) { return dist[lhs] > dist[rhs]; };

    std::vector<int> queue;

    // Add start node with 0 cost
    dist[start] = 0;
    queue.push_back(start);

    // Initialise all other nodes to infinite cost and add them to the priority queue
    for (int i = 0; i < g.get_nodes(); i++) {

        if (i == start) continue;

        dist[i] = std::numeric_limits<int>::max();
        queue.push_back(i);
    }

    // Heapify queue
    std::make_heap(queue.begin(), queue.end(), comparator);

    while (!queue.empty()) {

        // Pop node with lowest cost
        int current = queue.front();
        std::pop_heap(queue.begin(), queue.end(), comparator);
        queue.pop_back();

        bool costs_updated = false;

        // Iterate over all the outgoing edges of the popped node
        for (const edge& e : g.get_edges(current)) {

            int new_dist = dist[current] + e.cost;

            if (new_dist < dist[e.end]) {
                dist[e.end] = new_dist;
                previous[e.end] = current;

                costs_updated = true;

            }
        }

        // If costs have been updated, resort heap
        if (costs_updated) std::make_heap(queue.begin(), queue.end(), comparator);
    }

    // All shortest paths have been found, unwind path
    int current = end;
    std::vector<path_step> path;

    while (current != start) {

        int prev = previous[current];

        // Add destination node (current) and the direction for each step
        edge e = g.get_edge(prev, current);
        path.push_back((path_step){current, e.dir});
        current = prev;
    }

    // Insert path between new start and original start at the end
    if (insert_original_start) {
        path.push_back((path_step){start, g.get_edges(original_start)[0].dir});
    }

    // Path is backwards so reverse it
    std::reverse(path.begin(), path.end());

    return path;
}
