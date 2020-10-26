#include "utils.h"

#include "stm32l432xx.h"

uint64_t compute_device_id(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
    CRC->INIT = 0;
    CRC->DR = *(uint32_t *) (UID_BASE + 8);
    CRC->DR = *(uint32_t *) (UID_BASE + 4);
    CRC->DR = *(uint32_t *) (UID_BASE + 0);
    uint32_t output = CRC->DR;
    CRC->DR = *(uint32_t *) (UID_BASE + 8);
    CRC->DR = *(uint32_t *) (UID_BASE + 4);
    CRC->DR = *(uint32_t *) (UID_BASE + 0);
    return ((uint64_t) output << 32LLU) | CRC->DR;
}