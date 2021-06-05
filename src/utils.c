#include "utils.h"

#include "stm32l432xx.h"

void init_crc(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
}

uint32_t compute_device_id(void) {
    CRC->INIT = 0;
    CRC->DR = *(uint32_t *) (UID_BASE + 8);
    CRC->DR = *(uint32_t *) (UID_BASE + 4);
    CRC->DR = *(uint32_t *) (UID_BASE + 0);
    return CRC->DR;
}