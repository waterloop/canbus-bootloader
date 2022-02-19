#include <stdint.h>
#include "bsp.h"
#include "utils.h"
#include "can.h"

CAN_Status can_init() {
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
    CAN->MCR &= ~(CAN_MCR_SLEEP);
    while (BIT_IS_SET(CAN->MSR, CAN_MSR_SLAK_Pos)) { asm("NOP"); }

    // put CAN peripheral in initialization mode
    CAN->MCR |= CAN_MCR_INRQ;
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
    /*
    bit timings setup done...
    */

    // put the CAN peripheral in normal mode
    CAN->MCR &= ~(CAN_MCR_INRQ);
    while (BIT_IS_SET(CAN->MSR, CAN_MSR_INAK_Pos)) { asm("NOP"); }

    // enable IRQ on RX FIFO 0
    CAN->IER |= CAN_IER_FMPIE0;
    NVIC_EnableIRQ(CAN_RX0_IRQn);

    return CAN_OK;
}

CAN_Status can_tx(uint16_t id, uint8_t* data, uint8_t len) {
    if( (CAN->TSR & CAN_TSR_TME_Msk) == 0 ) {
        return TX_MB_FULL_ERR;
    }

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
    CAN->sTxMailBox[mb_num].TIR = (id << CAN_TI0R_STID_Pos);

    // set the TXRQ bit to send the message
    CAN->sTxMailBox[mb_num].TIR |= (CAN_TI0R_TXRQ);

    // wait for the message to go through
    while (BIT_IS_CLEAR(CAN->TSR, CAN_TSR_TME0_Pos + mb_num)) { asm("NOP"); }

    return CAN_OK;
}

CAN_Status can_rx(uint16_t* id, uint8_t* data, uint8_t* len) {
    if ( (CAN->RF0R & CAN_RF0R_FMP0) == 0 ) {
        return RX_MB_EMPTY_ERR;
    }

    // hardcoded to use mailbox 0 for now
    uint8_t mb = 0;

    uint8_t ide = (CAN->sFIFOMailBox[mb].RIR >> 2) & 1;
    if (ide == 1) {
        // extended id
        return INVALID_ID_TYPE_ERR;
    }

    *id = (CAN->sFIFOMailBox[mb].RIR >> 21) & 0x7ff;
    *len = (CAN->sFIFOMailBox[mb].RDTR & CAN_RDT0R_DLC);

    data[0] = (CAN->sFIFOMailBox[mb].RDLR >> 0) & 0xff;
    data[1] = (CAN->sFIFOMailBox[mb].RDLR >> 8) & 0xff;
    data[2] = (CAN->sFIFOMailBox[mb].RDLR >> 16) & 0xff;
    data[3] = (CAN->sFIFOMailBox[mb].RDLR >> 24) & 0xff;

    data[4] = (CAN->sFIFOMailBox[mb].RDHR >> 0) & 0xff;
    data[5] = (CAN->sFIFOMailBox[mb].RDHR >> 8) & 0xff;
    data[6] = (CAN->sFIFOMailBox[mb].RDHR >> 16) & 0xff;
    data[7] = (CAN->sFIFOMailBox[mb].RDHR >> 24) & 0xff; 

    CAN->RF0R |= CAN_RF0R_RFOM0;

    return CAN_OK;
}

