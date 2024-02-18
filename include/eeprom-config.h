#ifndef EEPROM_CONFIG_H
#define EEPROM_CONFIG_H

#include <inttypes.h>
#include <Arduino.h>
const int MAX_KEY_LENGTH = 16;

// Sensor type enumeration
enum SensorType {
  _BME280,
  _SHT31
};

// RFM69 module type enumeration
enum RFM69Type {
  _RFM69HW,
  _RFM69W
};

// Define the structure for parameters
struct Configuration {
  char key[MAX_KEY_LENGTH + 1];
  uint8_t nodeID;
  uint8_t networkID;
  uint8_t gatewayID;
  SensorType sensorType; // Added field for sensor type
  RFM69Type rfm69Type;   // Added field for RFM69 module type
  uint32_t crc;          // CRC32 checksum
};
// checks a config in eeprom
bool loadConfig(Configuration& config, const uint16_t eepromAddress);
// create a new config
void doConfig(Configuration& config, Stream &serialport, const uint16_t eepromAddress);
#endif

