#ifndef SYSTEM_INC_H
#define SYSTEM_INC_H

#include <stm32f042x6.h>

#define RESET() SCB->AIRCR ^= 0xFFFF0000 | SCB_AIRCR_SYSRESETREQ_Msk
#define CAN_RX0_Handler_Func CEC_CAN_IRQHandler
#define CAN_RX1_Handler_Func CEC_CAN_IRQHandler
#define CAN_RX0_IRQn_Value CEC_CAN_IRQn
#define CAN_RX1_IRQn_Value CEC_CAN_IRQn
// Prescaler = 3, Seg1 = 13, Seg2 = 2, SJW = 1
#define CAN_BTR_Value 0x001c0002
#define SYSTEM_CLOCK_KHZ 48000

#endif
