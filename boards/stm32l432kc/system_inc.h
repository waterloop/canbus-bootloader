#ifndef SYSTEM_INC_H
#define SYSTEM_INC_H

#include <stm32l432xx.h>
#include "flash.h"

#define RESET() SCB->AIRCR ^= 0xFFFF0000 | SCB_AIRCR_SYSRESETREQ_Msk
#define CAN_RX0_Handler_Func CAN1_RX0_IRQHandler
#define CAN_RX1_Handler_Func CAN1_RX1_IRQHandler
#define CAN_RX0_IRQn_Value CAN1_RX0_IRQn
#define CAN_RX1_IRQn_Value CAN1_RX1_IRQn
// Prescaler = 5, Seg1 = 13, Seg2 = 2, SJW = 1
#define CAN_BTR_Value 0x001c0004

void canbus_setup(void);
flash_ret_t flash_erase_page(uint32_t page);
flash_ret_t flash_write(void *addr, const void *data, uint32_t len);

#endif
