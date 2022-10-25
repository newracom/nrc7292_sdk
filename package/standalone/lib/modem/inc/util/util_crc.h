#ifndef UTIL_CRC_H
#define UTIL_CRC_H

#include "system.h"

uint32_t util_crc_compute_crc32(uint8_t *data, uint32_t length);
uint8_t util_crc_compute_crc7(const uint8_t *data, int len);
uint32_t reverse_32(uint32_t data);

#endif /* UTIL_CRC_H */
