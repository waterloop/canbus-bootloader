#include "canbus.h"
#include "utils.h"
#include "flash.h"

#include <stm32l432xx.h>
#include <stdbool.h>


void __attribute__ ((interrupt ("IRQ"))) CAN1_RX0_IRQHandler() {
    uint32_t id;
    uint64_t data;
    uint8_t len;
    bool reply = false;

    canbus_receive(&id, &data, &len, 0);

    // Get actual id from internal ID representation
    id = CAN_GET_EXID(id);

    uint8_t short_device_id = id & 0xFF;
    if(!short_device_id) {
//        short_device_id = get_short_device_id();
    }

    switch(id >> 16) {
        case 0x1FF1:
            id = 0;
            break;
    }

    if(reply) {
        canbus_transmit_fast(id, &data, len);
    }
}

int main(void) {
    uint64_t device_id = compute_device_id();
    uint8_t short_device_id = 1;
    uint8_t mode = 0;
	canbus_init(short_device_id);
	canbus_transmit_fast(CAN_EXID(0x1FF00000 | short_device_id | (mode << 8)), &device_id, 8);

    flash_unlock();

    struct flash_eeprom_entry to_write = {
	        .is_free = false,
	};

    struct flash_eeprom_entry *entry;
    if(flash_eeprom_get_addr(&entry) == FLASH_OK) {
        to_write.short_id = entry->short_id + 1;
    }

    flash_eeprom_write(&to_write);

	while(1) {
	    struct flash_eeprom_entry *entry;
	    flash_ret_t ret = flash_eeprom_find_free(&entry);
	}

//	uint64_t v = 0;
//    int k = 1;
//
//	while(1) {
//	    if(k)
//		    while(canbus_transmit_fast(CAN_STID(0x555), &v, 8) != CANBUS_OK);
//            while(canbus_transmit_fast(CAN_EXID(0xAAAAAA), &v, 8) != CANBUS_OK);
//		v += 1;
//		for(int i = 0; i < 2000000; ++i) {
//		    k += 1 + v;
//		}
//		k += 1;
//	}

	return 0;
}
