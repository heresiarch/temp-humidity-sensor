#include "utils.h"
// Function to calculate CRC32 without lookup table
uint32_t calculateCRC32(const uint8_t *data, size_t size) {
  const uint32_t polynomial = 0xEDB88320;
  uint32_t crc = 0xFFFFFFFF;

  for (size_t i = 0; i < size; i++) {
    crc ^= data[i];

    for (size_t j = 0; j < 8; j++) {
      crc = (crc & 1) ? (crc >> 1) ^ polynomial : (crc >> 1);
    }
  }

  return crc ^ 0xFFFFFFFF;
}