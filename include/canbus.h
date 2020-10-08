#ifndef CANBUS_H
#define CANBUS_H

typedef enum {
	CANBUS_OK = 0,
	CANBUS_NOT_READY = -1,
	CANBUS_BUFFER_FULL = -2,
	CANBUS_NO_DATA = -3,
} canbus_ret_t;


#endif
