#pragma once

#include <stdint.h>

typedef enum {
    CAN_OK = 0b0000U,
    TIMEOUT_ERR = 0b0001U,
    TX_MB_FULL_ERR = 0b0010U,
    RX_MB_EMPTY_ERR = 0b0100U,
    INVALID_ID_TYPE_ERR = 0b1000U
} CAN_Status;

CAN_Status can_init();
CAN_Status can_tx(uint16_t id, uint8_t* data, uint8_t len);
CAN_Status can_rx(uint16_t* id, uint8_t* data, uint8_t* len);
