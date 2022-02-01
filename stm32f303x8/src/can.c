#include <stdint.h>
#include "bsp.h"
#include "utils.h"
#include "can.h"

void can_init() {
    // enable APB1 clock for CAN
    RCC->AHBENR |= (RCC_AHBENR_GPIOFEN | RCC_AHBENR_GPIOAEN);
    RCC->APB1ENR |= RCC_APB1ENR_CANEN;

    /*
    configure GPIO for CAN...
    */
    // set PA11 and PA12 to alternate function 9 (CAN)
    GPIOA->AFR[1] &= ~(GPIO_AFRH_AFRH3 | GPIO_AFRH_AFRH4);
    GPIOA->AFR[1] |= (9U << GPIO_AFRH_AFRH3_Pos) | (9U << GPIO_AFRH_AFRH4_Pos);

    // set PA11 and PA12 to very high speed
    GPIOA->OSPEEDR |= (3U << GPIO_OSPEEDER_OSPEEDR11_Pos) | (3U << GPIO_OSPEEDER_OSPEEDR12_Pos);

    // set PA11 and PA12 to alternate function mode
    GPIOA->MODER &= ~(GPIO_MODER_MODER11 | GPIO_MODER_MODER12);
    GPIOA->MODER |= (2U << GPIO_MODER_MODER11_Pos) | (2U << GPIO_MODER_MODER12_Pos);
    /*
    GPIO configuration done...
    */

    // exit sleep mode
    CAN->MCR |= CAN_MCR_SLEEP;

    // wait until the peripheral has exited sleep mode
    while (BIT_IS_SET(CAN->MSR, CAN_MSR_SLAK)) { asm("NOP"); }

    // put CAN peripheral in initialization mode
    CAN->MCR |= CAN_MCR_INRQ;

    // wait until the peripheral has entered initialization mode
    while (BIT_IS_CLEAR(CAN->MSR, CAN_MSR_INAK_Pos)) { asm("NOP"); }

    // turn on automatic retransmission and automatic wakeup
    CAN->MCR |= CAN_MCR_AWUM;
    CAN->MCR &= ~(CAN_MCR_NART);

    // make the CAN peripheral work in debug mode (there's no #define
    // for this apparently... but it's in the 16th bit)
    CAN->MCR &= ~(1U << 16);

    /*
    configure the bit timings for 500kbps...
    */
    // set prescalar for time quanta to 4
    CAN->BTR &= ~(CAN_BTR_BRP);
    CAN->BTR |= (3U << CAN_BTR_BRP_Pos);

    // set time quanta in bit segment 1 to 13
    CAN->BTR &= ~(CAN_BTR_TS1);
    CAN->BTR |= (12U << CAN_BTR_TS1_Pos);

    // set time quanta in bit segment 2 to 2
    CAN->BTR &= ~(CAN_BTR_TS2);
    CAN->BTR |= (1U << CAN_BTR_TS2_Pos);

    // CAN->BTR = 1835011;
    // CAN->BTR = 1966083;
    // CAN->BTR = 1966085;
    // CAN->BTR |= (CAN_BS1_13TQ | CAN_BS2_2TQ | 3);

    // CAN->BTR |= (12U << CAN_BTR_TS1_Pos);
    // CAN->BTR |= (1U << CAN_BTR_TS2_Pos);
    // CAN->BTR |= (3U << CAN_BTR_BRP_Pos);
    /*
    bit timings setup done...
    */

    // put the CAN peripheral in normal mode
    CAN->MCR &= ~(CAN_MCR_INRQ);

    // wait for the peripheral to enter normal mode
    while (BIT_IS_SET(CAN->MSR, CAN_MSR_INAK_Pos)) { asm("NOP"); }
}

void can_tx(uint16_t id, uint8_t* data, uint8_t len) {
    // we're always gonna transmit 8 bytes cus lmao
    uint8_t tx_data[8] = {0};
    for (uint8_t i = 0; i < len; i++) { tx_data[i] = data[i]; }

    // get a free mailbox
    uint8_t mb_num = (CAN->TSR >> CAN_TSR_CODE_Pos) & 0b11;

    // send the frame by filling the mailbox
    CAN->sTxMailBox[mb_num].TDHR = (tx_data[7] << 24) |
                                   (tx_data[6] << 16) |
                                   (tx_data[5] << 8)  |
                                   (tx_data[4]);
    CAN->sTxMailBox[mb_num].TDLR = (tx_data[3] << 24) |
                                   (tx_data[2] << 16) |
                                   (tx_data[1] << 8)  |
                                   (tx_data[0]);
    CAN->sTxMailBox[mb_num].TDTR = len;
    CAN->sTxMailBox[mb_num].TIR = id;

    // set the TXRQ bit to send the message
    CAN->sTxMailBox[mb_num].TIR |= (CAN_TI0R_TXRQ);

    uint32_t thing = CAN->ESR;
    thing |= 1;

    uint32_t thing2 = CAN->TSR;
    thing2 |= 1;

    // wait for the message to go through
    while (( CAN->TSR & (1 << (CAN_TSR_TME0_Pos + mb_num)) ) == 0) {
        asm("NOP");
    }
}

