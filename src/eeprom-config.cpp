#include <EEPROM.h>
#include "eeprom-config.h"
#include "utils.h"

bool loadConfig(Configuration& config, const uint16_t eepromAddress) {
  EEPROM.get(eepromAddress, config);
  // Calculate CRC for the stored configuration
  uint32_t calculatedCRC = calculateCRC32((uint8_t*)&config, sizeof(Configuration) - sizeof(uint32_t));
  if (calculatedCRC == config.crc) {
    return true;
  } else {
    return false;
  }
}

void doConfig(Configuration& config, Stream &serialport, const uint16_t eepromAddress){
  // Prompt for and validate Parameter 1 (16 Byte ASCII Key)
  serialport.println("Enter 16 Byte ASCII Key:");
  size_t keyIndex = 0;
  while (keyIndex < MAX_KEY_LENGTH) {
    while (!serialport.available()) {
      delay(100);
    }
    char inputChar = serialport.read();

    // Echo the character back to the terminal
    serialport.print(inputChar);

    // Store the character in the key
    if (inputChar == '\n' || inputChar == '\r') {
      config.key[keyIndex] = '\0'; // Null-terminate the string
      break;
    } else {
      config.key[keyIndex++] = inputChar;
    }
  }

  // Prompt for and validate Parameter 2 (Node ID from 0-255)
  while (true) {
    serialport.println("\nEnter Node ID (0-255):");
    while (!serialport.available()) {
      delay(100);
    }
    config.nodeID = serialport.parseInt();

    // Validate Node ID
    if (config.nodeID >= 0 && config.nodeID <= 255) {
      break; // Exit the loop if Node ID is valid
    } else {
      serialport.println("Invalid Node ID. Please enter a value between 0 and 255.");
    }
  }

  // Prompt for and validate Parameter 3 (Network ID from 0-255)
  while (true) {
    serialport.println("\nEnter Network ID (0-255):");
    while (!serialport.available()) {
      delay(100);
    }
    config.networkID = serialport.parseInt();

    // Validate Network ID
    if (config.networkID >= 0 && config.networkID <= 255) {
      break; // Exit the loop if Network ID is valid
    } else {
      serialport.println("Invalid Network ID. Please enter a value between 0 and 255.");
    }
  }

   // Prompt for and validate Parameter 4 (Gateway ID from 0-255)
  while (true) {
    serialport.println("\nEnter Gateway ID (0-255):");
    while (!serialport.available()) {
      delay(100);
    }
    config.gatewayID = serialport.parseInt();

    // Validate Network ID
    if (config.gatewayID>= 0 && config.gatewayID <= 255) {
      break; // Exit the loop if Network ID is valid
    } else {
      serialport.println("Invalid Network ID. Please enter a value between 0 and 255.");
    }
  }

  // Prompt for and validate Parameter 5 (Sensor Type)
  while (true) {
    serialport.println("\nEnter Sensor Type (0 for BME280, 1 for SHT31):");
    while (!serialport.available()) {
      delay(100);
    }
    int sensorTypeValue = serialport.parseInt();

    // Validate Sensor Type
    if (sensorTypeValue == _BME280 || sensorTypeValue == _SHT31) {
      config.sensorType = static_cast<SensorType>(sensorTypeValue);
      break; // Exit the loop if Sensor Type is valid
    } else {
      serialport.println("Invalid Sensor Type. Please enter 0 for BME280 or 1 for SHT31.");
    }
  }

  // Prompt for and validate Parameter 6 (RFM69 Module Type)
  while (true) {
    serialport.println("\nEnter RFM69 Module Type (0 for RFM69HW/HCW, 1 for RFM69W/CW):");
    while (!serialport.available()) {
      delay(100);
    }
    int rfm69TypeValue = serialport.parseInt();

    // Validate RFM69 Module Type
    if (rfm69TypeValue == _RFM69HW || rfm69TypeValue == _RFM69W) {
      config.rfm69Type = static_cast<RFM69Type>(rfm69TypeValue);
      break; // Exit the loop if RFM69 Module Type is valid
    } else {
      serialport.println("Invalid RFM69 Module Type. Please enter 0 for RFM69HW/HCW or 1 for RFM69W/CW.");
    }
  }

  // Parameters are validated, store them in EEPROM
  config.crc = calculateCRC32((uint8_t*)&config, sizeof(Configuration) - sizeof(uint32_t));
  EEPROM.put(eepromAddress, config);
  serialport.println("\nParameters stored in EEPROM.");
}
