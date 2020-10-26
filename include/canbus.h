#ifndef CANBUS_H
#define CANBUS_H

#include <stdint.h>

typedef enum {
	CANBUS_OK = 0,
	CANBUS_NOT_READY = -1,
	CANBUS_BUFFER_FULL = -2,
	CANBUS_NO_DATA = -3,
	CANBUS_INVALID_PARAM = -4,
} canbus_ret_t;

void canbus_init(uint8_t short_device_id);
canbus_ret_t canbus_transmit_fast(uint32_t id, const void *data, uint8_t len);
canbus_ret_t canbus_transmit(uint32_t id, const void *data, uint8_t len);
void canbus_set_user_isr(void (*callback)());
canbus_ret_t canbus_receive(uint32_t *id, void *data, uint8_t *len, uint8_t mailbox);


#define CAN_STID(x) ((x) << 21)
#define CAN_EXID(x) (((x) << 3) | 4)
#define CAN_EXID_BIT 4
#define CAN_GET_STID(x) ((x) >> 21)
#define CAN_GET_EXID(x) ((x) >> 3)

#endif
