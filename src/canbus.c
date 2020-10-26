#include "canbus.h"
#include "utils.h"

#include <stm32l432xx.h>

void (*canbus_isr_handler)();

void canbus_init(uint8_t short_device_id) {
	// Enable APB1 clock for CAN1
	RCC->APB1ENR1 |= RCC_APB1ENR1_CAN1EN;
	// Enable AHB2 clock for GPIO Port A
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

	// Clear alt functions for PA11 and PA12, and set them to alt function 9
	GPIOA->AFR[1] = (GPIOA->AFR[1] & ~(GPIO_AFRH_AFSEL11_Msk | GPIO_AFRH_AFSEL12_Msk)) | REG_FIELD(GPIO_AFRH_AFSEL11, 9) | REG_FIELD(GPIO_AFRH_AFSEL12, 9);
	// Configure GPIO pins PA11 and PA12 to be very high speed
	GPIOA->OSPEEDR |= REG_FIELD(GPIO_OSPEEDR_OSPEED11, 3) | REG_FIELD(GPIO_OSPEEDR_OSPEED12, 3);
	// Clear port mode configuration for PA11 and PA12, and set them to Alterate function mode
	GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODE11_Msk | GPIO_MODER_MODE12_Msk)) | REG_FIELD(GPIO_MODER_MODE11, 2) | REG_FIELD(GPIO_MODER_MODE12, 2);

	// Request to switch into initialization mode and exit sleep mode. Also set debug freeze to 0
	CAN->MCR = CAN_MCR_INRQ;

	// Wait for initialization mode confirmation
	while(!(CAN->MSR & CAN_MSR_INAK_Msk));

	// SJW = 0 (tRJW: 1q), TS2 = 0 (tBS2: 1q), TS1 = 5 (tBS1: 6q), BRP = 9 (prescaler: 10, f = 8MHz)
	CAN->BTR = REG_FIELD(CAN_BTR_SJW, 0) | REG_FIELD(CAN_BTR_TS2, 0) | REG_FIELD(CAN_BTR_TS1, 5) | REG_FIELD(CAN_BTR_BRP, 9);

	// Request to switch into normal mode
	// While we're at it, enable auto bus-off management, and auto wake-up
	CAN->MCR = CAN_MCR_ABOM | CAN_MCR_AWUM;

	// Wait for normal mode configuration
	while(CAN->MCR & CAN_MSR_INAK_Msk);

	// Start init filters
	CAN->FMR |= CAN_FMR_FINIT;

	// CAN_FM1R set filter banks to identifier mask mode. This is already the default

	// Set filter scale of bank 0-3 to single 32-bit scale configuration
	CAN->FS1R |= CAN_FS1R_FSC0 | CAN_FS1R_FSC1 | CAN_FS1R_FSC2 | CAN_FS1R_FSC3;

	// Assign filters 0, 1 to FIFO 0 and filter 2,3 to FIFO 1
	CAN->FFA1R |= CAN_FFA1R_FFA2 | CAN_FFA1R_FFA3;

	// Activate filters 0-3
	CAN->FA1R |= CAN_FA1R_FACT0 | CAN_FA1R_FACT1 | CAN_FA1R_FACT2 | CAN_FA1R_FACT3;

	// Set filter bank 0 to match 0x1xxxxxii, where ii = short_device_id
	CAN->sFilterRegister[0].FR1 = CAN_EXID(0x10000000 | short_device_id);
    CAN->sFilterRegister[0].FR2 = CAN_EXID(0x100000FF);

    // Set filter bank 1 to match 0x1xxxxx00, for broadcast
    CAN->sFilterRegister[1].FR1 = CAN_EXID(0x10000000);
    CAN->sFilterRegister[1].FR2 = CAN_EXID(0x100000FF);

    // Set filter bank 2 to match all standard ID lengths
    CAN->sFilterRegister[2].FR1 = CAN_STID(0);
    CAN->sFilterRegister[2].FR2 = CAN_EXID(0);

    // Set filter bank 3 to match 0x0xxxxxxx
    CAN->sFilterRegister[3].FR1 = CAN_EXID(0x00000000);
    CAN->sFilterRegister[3].FR2 = CAN_EXID(0x10000000);

    // Finish init filters
    CAN->FMR &= ~CAN_FMR_FINIT;

    // Enable interrupts on FIFO 0 first
    CAN->IER |= CAN_IER_FMPIE0;
    NVIC->ISER[CAN1_RX0_IRQn >> 5] |= (1 << (CAN1_RX0_IRQn & 31));
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
	CAN->sTxMailBox[tx_mb].TDLR = *(uint32_t *) data;
	CAN->sTxMailBox[tx_mb].TDHR = *((uint32_t *) data + 1);
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
		bytes[i] = *((uint8_t *) data + i);
	}
	return canbus_transmit_fast(id, bytes, len);
}

void __attribute__ ((interrupt ("IRQ"))) CAN1_RX1_IRQHandler() {
    canbus_isr_handler();
}

void canbus_set_user_isr(void (*callback)()) {
	canbus_isr_handler = callback;
    // Enable interrupts on FIFO 1
    CAN->IER |= CAN_IER_FMPIE1;
    NVIC->ISER[CAN1_RX1_IRQn >> 5] |= (1 << (CAN1_RX1_IRQn & 31));
}

canbus_ret_t canbus_receive(uint32_t *id, void *data, uint8_t *len, uint8_t mailbox) {
    volatile uint32_t *RFxR = &CAN1->RF0R + mailbox;
    // Check for non empty FIFO
    if((*RFxR & CAN_RF0R_FMP0_Msk) == 0) {
        return CANBUS_NO_DATA;
    }
    // Copy data into RAM
    *id = CAN1->sFIFOMailBox[mailbox].RIR;
    *(uint32_t*) data = CAN1->sFIFOMailBox[mailbox].RDLR;
    *((uint32_t*) data + 1) = CAN1->sFIFOMailBox[mailbox].RDHR;
    *len = CAN1->sFIFOMailBox[mailbox].RDTR & CAN_RDT0R_DLC_Msk;
    *RFxR |= CAN_RF0R_RFOM0;
	return CANBUS_OK;
}