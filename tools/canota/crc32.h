#ifndef CRC32_H
#define CRC32_H

#include <stdint.h>
#include <stddef.h>

#define CRC32_INIT 0xFFFFFFFF

uint32_t crc32(void *data, size_t len, uint32_t *init);

#endif
