#ifndef SYSTEM_INC_H
#define SYSTEM_INC_H

#include <stm32f405xx.h>

#define RESET() SCB->AIRCR ^= 0xFFFF0000 | SCB_AIRCR_SYSRESETREQ_Msk
#define CAN_RX0_Handler_Func CAN1_RX0_IRQHandler
#define CAN_RX1_Handler_Func CAN1_RX1_IRQHandler
#define CAN_RX0_IRQn_Value CAN1_RX0_IRQn
#define CAN_RX1_IRQn_Value CAN1_RX1_IRQn
#define CAN CAN1
// Prescaler = 3, Seg1 = 11, Seg2 = 2, SJW = 1
#define CAN_BTR_Value 0x001a0002
#define SYSTEM_CLOCK_KHZ 168000

#endif
