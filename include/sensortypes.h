#ifndef _SENSOR_TYPES_H_
#define _SENSOR_TYPES_H_
#include <inttypes.h>
typedef struct {
    uint8_t id;    // byte from 0-255
    uint8_t sensortype;    // byte from 0-255
    int16_t temperature;      // temperature * 100 to avoid float
    uint8_t humidity;    // byte relative humidity from 0 to 100
    uint16_t batttery_voltage; // battery * 100
    uint32_t checksum; // checksum byte
} SensorData;
#endif

