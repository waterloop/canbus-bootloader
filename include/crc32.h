#ifndef CRC_H
#define CRC_H

#include <stdint.h>

#define CRC32_INIT 0xFFFFFFFF
uint32_t crc32(uint32_t crc, void *data, uint32_t len);

#endif
