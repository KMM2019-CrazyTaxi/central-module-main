#ifndef CM_SPI_H
#define CM_SPI_H

#define SPI_START       0xAA
#define SPI_NAN         0x00
#define SPI_ERROR       0x55
#define SPI_FINISHED    0x66
#define SPI_RESTART     0x99
#define SPI_ACK         0xEE

#define SPI_CONTROL_INIT_MSG_SIZE       1
#define SPI_CONTROL_DATA_MSG_SIZE       3
#define SPI_CONTROL_FINISH_MSG_SIZE     1

#define SPI_FAILED_WAIT_MS 1

#define SPI_FREQ    1000000 / 32
#define SPI_CONTROL 0
#define SPI_SENSOR  1
#define SPI_CHANNEL 0

#define SPI_FAIL_COUNT 5

#define SPI_CONTROL_SS_PIN 0 // Physical pin 11
#define SPI_SENSOR_SS_PIN  1 // Physical pin 12

#endif