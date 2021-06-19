#include "crc32.h"

// This function is not hardware accelerated due to the dependency on the CRC unit in the STM.
// The CRC unit is not consistent across all board versions, and may be used by user code
uint32_t crc32(uint32_t crc, const void *data, uint32_t len) {
	for(uint32_t i = 0; i < len; ++i) {
		crc = crc ^ ((uint8_t *) data)[i];
		for(uint8_t j = 0; j < 8; ++j) {
			crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
		}
	}
	return crc;
}