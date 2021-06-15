#include "utils.h"

#include <system_inc.h>

void init_crc(void) {
}

uint32_t compute_device_id(void) {
    CRC->INIT = 0xFFFFFFFF;
    CRC->DR = *(uint32_t *) (UID_BASE + 8);
    CRC->DR = *(uint32_t *) (UID_BASE + 4);
    CRC->DR = *(uint32_t *) (UID_BASE + 0);
    return CRC->DR & 0xFFFFFFFF;
}