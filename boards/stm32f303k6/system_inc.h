#ifndef SYSTEM_INC_H
#define SYSTEM_INC_H

#include <stm32f303x8.h>

#define RESET() SCB->AIRCR ^= 0xFFFF0000 | SCB_AIRCR_SYSRESETREQ_Msk
#define CAN_RX0_Handler_Func CAN_RX0_IRQHandler
#define CAN_RX1_Handler_Func CAN_RX1_IRQHandler
#define CAN_RX0_IRQn_Value CAN_RX0_IRQn
#define CAN_RX1_IRQn_Value CAN_RX1_IRQn
// Prescaler = 2, Seg1 = 15, Seg2 = 2, SJW = 1
#define CAN_BTR_Value 0x001e0001
#define SYSTEM_CLOCK_KHZ 72000

#endif
