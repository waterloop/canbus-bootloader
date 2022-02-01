#include <stdint.h>
#include "bsp.h"
#include "utils.h"
#include "can.h"

void can_init() {
    // put CAN peripheral in initialization mode
    CAN->MCR |= CAN_MCR_INRQ;

    // wait until the peripheral has entered initialization mode
    while (BIT_IS_CLEAR(CAN->MSR, CAN_MSR_INAK)) { asm("NOP"); }

    // turn off silent and loopback mode
    CAN->BTR &= ~(CAN_BTR_SILM | CAN_BTR_LBKM);

    // make the CAN peripheral work in debug mode (there's no #define
    // for this apparently... but it's in the 16th bit)
    CAN->BTR |= (1U << 16);

    // turn on automatic retransmission
    CAN->BTR &= ~(CAN_MCR_NART);

    /*
    configure the bit timings for 500kbps...
    */
    // set time quanta in bit segment 1 to 15 times
    CAN->BTR |= (15U << CAN_BTR_TS1_Pos);

    // set time quanta in bit segement 2 to 2 times
    CAN->BTR |= (2 << CAN_BTR_TS2_Pos);

    // set prescalar for time quanta to 4
    CAN->BTR |= (4 << CAN_BTR_BRP_Pos);
    /*
    bit timings setup done...
    */

    // put the CAN peripheral in normal mode
    CAN->BTR &= ~(CAN_MCR_INRQ);

    // wait for the peripheral to enter normal mode
    while (BIT_IS_SET(CAN->MCR, CAN_MSR_INAK)) { asm("NOP"); }
}

void can_tx(uint16_t id, uint8_t* data, uint8_t len) {
    // we're always gonna transmit 8 bytes cus lmao
    uint8_t tx_data[8] = {0};
    for (uint8_t i = 0; i < len; i++) { tx_data[i] = data[i]; }

    // get a free mailbox
    uint8_t mb_num = (CAN->TSR >> CAN_TSR_CODE_Pos) & 0b111;

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
    while (BIT_IS_SET(CAN->TSR, CAN_TSR_TME0_Pos + mb_num)) { asm("NOP"); }
}

