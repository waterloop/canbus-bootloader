#include "canbus.h"
#include "platform.h"

#include <system_inc.h>

void canbus_init(uint8_t short_device_id) {
	// Request to switch into initialization mode and exit sleep mode. Also set debug freeze to 0
	CAN->MCR = CAN_MCR_INRQ;

	// Wait for initialization mode confirmation
	while(!(CAN->MSR & CAN_MSR_INAK_Msk));

	// Configure CAN bus dividers and timing. This value is board specific
	CAN->BTR = CAN_BTR_Value;

	// Request to switch into normal mode
	// While we're at it, enable auto bus-off management, and auto wake-up
	CAN->MCR = CAN_MCR_ABOM | CAN_MCR_AWUM;

	// Wait for normal mode configuration
	while(CAN->MCR & CAN_MSR_INAK_Msk);

	// Start init filters
	CAN->FMR |= CAN_FMR_FINIT;

	// CAN_FM1R set filter banks to identifier mask mode. This is already the default

	// Set filter scale of bank 0-3 to single 32-bit scale configuration
	CAN->FS1R |= CAN_FS1R_FSC0 | CAN_FS1R_FSC1 | CAN_FS1R_FSC2;

	// Assign filter 0 FIFO 0 and filter 1,2 to FIFO 1
	CAN->FFA1R |= CAN_FFA1R_FFA2;

	// Activate filters 0-2
	CAN->FA1R |= CAN_FA1R_FACT0 | CAN_FA1R_FACT1 | CAN_FA1R_FACT2;

	// Set filter bank 0 to match 0x1xxxxxii, where ii = short_device_id
	CAN->sFilterRegister[0].FR1 = CAN_EXID(0x10000000 | short_device_id);
    CAN->sFilterRegister[0].FR2 = CAN_EXID(0x100000FF);

    // Set filter bank 1 to match all standard ID lengths
    CAN->sFilterRegister[1].FR1 = CAN_STID(0);
    CAN->sFilterRegister[1].FR2 = CAN_EXID(0);

    // Set filter bank 2 to match 0x0xxxxxxx
    CAN->sFilterRegister[2].FR1 = CAN_EXID(0x00000000);
    CAN->sFilterRegister[2].FR2 = CAN_EXID(0x10000000);

    // Finish init filters
    CAN->FMR &= ~CAN_FMR_FINIT;

    // Enable interrupts on FIFO 0 first
    CAN->IER |= CAN_IER_FMPIE0;

	NVIC->ISER[CAN_RX0_IRQn_Value >> 5] |= (1 << (CAN_RX0_IRQn_Value & 31));
}

// Skips length checks and assumes data is 8 bytes long
canbus_ret_t canbus_transmit_fast(uint32_t id, const void *data, uint8_t len) {
	// Check for empty transmit mailboxes
	if((CAN->TSR & CAN_TSR_TME_Msk) == 0) {
		return CANBUS_BUFFER_FULL;
	}
	// Get a free mailbox
	uint8_t tx_mb = REG_FIELD_GET(CAN->TSR, CAN_TSR_CODE);
	// Fill the mailbox and transmit
	CAN->sTxMailBox[tx_mb].TDLR = ((uint32_t *) data)[0];
	CAN->sTxMailBox[tx_mb].TDHR = ((uint32_t *) data)[1];
	CAN->sTxMailBox[tx_mb].TDTR = len;
	CAN->sTxMailBox[tx_mb].TIR = id | CAN_TI0R_TXRQ;
	return CANBUS_OK;
}

// Performs the extra checks and copies, allowing data to be a buffer of any length <= 8
canbus_ret_t canbus_transmit(uint32_t id, const void *data, uint8_t len) {
	if(len > 8) {
		return CANBUS_INVALID_PARAM;
	}
	// Copy into local bytes array to make sure we're giving a buffer of size 8
	uint8_t bytes[8];
	for(uint8_t i = 0; i < 8; ++i) {
		bytes[i] = ((uint8_t *) data)[i];
	}
	return canbus_transmit_fast(id, bytes, len);
}

void canbus_enable_isr2(void) {
    // Enable interrupts on FIFO 1
    CAN->IER |= CAN_IER_FMPIE1;
    NVIC->ISER[CAN_RX1_IRQn_Value >> 5] |= (1 << (CAN_RX1_IRQn_Value & 31));
}

canbus_ret_t canbus_receive(uint32_t *id, void *data, uint8_t *len, uint8_t mailbox) {
    volatile uint32_t *RFxR = &CAN->RF0R + mailbox;
    // Check for non empty FIFO
    if((*RFxR & CAN_RF0R_FMP0_Msk) == 0) {
        return CANBUS_NO_DATA;
    }
    // Copy data into RAM
    *id = CAN->sFIFOMailBox[mailbox].RIR;
	((uint32_t *) data)[0] = CAN->sFIFOMailBox[mailbox].RDLR;
	((uint32_t *) data)[1] = CAN->sFIFOMailBox[mailbox].RDHR;
    *len = CAN->sFIFOMailBox[mailbox].RDTR & CAN_RDT0R_DLC_Msk;
    *RFxR |= CAN_RF0R_RFOM0;
	return CANBUS_OK;
}