#include "utils.h"

#include <system_inc.h>

void init_crc(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
    CRC->CR |= CRC_CR_REV_OUT | (3 << CRC_CR_REV_IN_Pos);
}

uint32_t compute_device_id(void) {
    CRC->INIT = 0xFFFFFFFF;
    CRC->DR = *(uint32_t *) (UID_BASE + 8);
    CRC->DR = *(uint32_t *) (UID_BASE + 4);
    CRC->DR = *(uint32_t *) (UID_BASE + 0);
    return CRC->DR & 0xFFFFFFFF;
}