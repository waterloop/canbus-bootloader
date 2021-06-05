#include "canbus.h"
#include "utils.h"
#include "flash.h"

#include <stm32l432xx.h>

extern uint32_t _sapp_rom;

#define BOOTLOADER_KEY 0xc1a93d759cb6be44

#define VERSION_MAJOR 0x01
#define VERSION_MINOR 0x00
#define DEVICE_TYPE 0x00
#define INFO ((VERSION_MAJOR << 24) | (VERSION_MINOR << 16) | (DEVICE_TYPE))

#define MODE_APPLICATION 0
#define MODE_BOOTLOADER 1

// Used to determine whether we have entered user application
uint64_t bootloader_key = BOOTLOADER_KEY;

#define PROTOCOL_STATUS_ERASE_OK 0x01
#define PROTOCOL_STATUS_COMMIT_OK 0x02
#define PROTOCOL_STATUS_ERASE_FAIL 0x81
#define PROTOCOL_STATUS_CRC_OUT_OF_BOUNDS 0x82
#define PROTOCOL_STATUS_COMMIT_CRC_FAIL 0x83
#define PROTOCOL_STATUS_COMMIT_FLASH_FAIL 0x84

#define CMD_DEVICE_IDENT_REQ 0x00
#define CMD_DEVICE_IDENT 0x01
#define CMD_MODE_SWITCH 0x02
#define CMD_DEBUG_MESSAGE 0x03
#define CMD_CHANGE_SHORT_ID 0x04
#define CMD_FLASH_ERASE 0x05
#define CMD_FLASH_CHECKSUM 0x06
#define CMD_FLASH_CHECKSUM_RESP 0x07
#define CMD_FLASH_BUFFER_WRITE_DATA 0x08
#define CMD_FLASH_BUFFER_COMMIT 0x09
#define CMD_STATUS 0xFE
#define CMD_RESET_INFO 0xFF

union u64_aliases {
    uint64_t u64;
    uint32_t u32[2];
    uint16_t u16[4];
    uint8_t u8[8];
};

union u64_aliases flash_write_buffer[256];

void change_mode(uint8_t mode) {
    struct flash_eeprom_entry *old_entry;
    struct flash_eeprom_entry entry;
    if(flash_eeprom_get_addr(&old_entry) == FLASH_OK) {
        if(mode != old_entry->mode) {
            entry = *old_entry;
            entry.mode = mode;
            flash_unlock();
            flash_eeprom_write(&entry);
        }
    }
}

void change_short_device_id(uint32_t short_device_id) {
    struct flash_eeprom_entry *old_entry;
    struct flash_eeprom_entry entry;
    if(flash_eeprom_get_addr(&old_entry) == FLASH_OK) {
        if(short_device_id != old_entry->short_device_id) {
            entry = *old_entry;
            entry.short_device_id = short_device_id;
            flash_unlock();
            flash_eeprom_write(&entry);
        }
    }
}

#define CMD_ID(cmd) (0x1F | (cmd << 16))

void __attribute__ ((interrupt ("IRQ"))) CAN1_RX0_IRQHandler() {
    uint32_t id;
    union u64_aliases data;
    uint8_t len;

    canbus_receive(&id, &data, &len, 0);

    // Get actual id from internal ID representation
    id = CAN_GET_EXID(id);

    if(id >> 24 != 0x1F) {
    	return;
    }

    uint8_t short_device_id = id & 0xFF;
    uint8_t param = (id >> 8) & 0xFF;

	switch((id >> 16) & 0xFF) {
		case CMD_DEVICE_IDENT_REQ:
			id = CMD_ID(CMD_DEVICE_IDENT);
			param = bootloader_key == BOOTLOADER_KEY ? MODE_BOOTLOADER : MODE_APPLICATION;
			data.u32[0] = INFO;
			data.u32[1] = compute_device_id();
			len = 8;
			goto transmit;
		case CMD_MODE_SWITCH:
			if(len != 8 || data.u32[0] != INFO || data.u32[1] != compute_device_id()) {
				return;
			}
			if(param != MODE_APPLICATION && param != MODE_BOOTLOADER && param != 0xFF) {
				return;
			}
			if(param != 0xFF) {
				change_mode(param);
			}
			// System reset
			SCB->AIRCR |= SCB_AIRCR_SYSRESETREQ_Msk; // Does not return
			return;
	}

    if(bootloader_key != BOOTLOADER_KEY) {
        goto transmit;
    }

	switch((id >> 16) & 0xFF) {
		case CMD_CHANGE_SHORT_ID:
			if(len != 8 || data.u64 != compute_device_id()) {
				return;
			}
			change_short_device_id(param);
			// System reset
			SCB->AIRCR |= SCB_AIRCR_SYSRESETREQ_Msk; // Does not return
			return;
		case CMD_FLASH_ERASE:
			if(len != 8) {
				return;
			}
			id = CMD_ID(CMD_STATUS);
			if((data.u8[0] = flash_erase_addr_range(data.u32[0], data.u32[1])) != FLASH_OK) {
				len = 1;
				param = PROTOCOL_STATUS_ERASE_FAIL;
			} else {
				len = 0;
				param = PROTOCOL_STATUS_ERASE_OK;
			}
			goto transmit;
		case CMD_FLASH_CHECKSUM:
			if(len != 8) {
				return;
			}
			if(flash_range_check(data.u32[0], data.u32[1]) != FLASH_OK) {
				len = 0;
				param = PROTOCOL_STATUS_CRC_OUT_OF_BOUNDS;
			}
			CRC->INIT = 0;
			for(uint32_t ptr = data.u32[0]; ptr < data.u32[0] + data.u32[1]; ++ptr) {
				CRC->DR = *(uint8_t *) ptr;
			}
			id = CMD_ID(CMD_FLASH_CHECKSUM_RESP);
			data.u32[1] = CRC->DR;
			param = 0;
			goto transmit;
		case CMD_FLASH_BUFFER_WRITE_DATA:
			if(len != 8) {
				return;
			}
			flash_write_buffer[param].u64 = data.u64;
			return;
		case CMD_FLASH_BUFFER_COMMIT:
			if(len != 8 || data.u32[0] & 7) { // Make sure that address is a multiple of 8
				return;
			}
			CRC->INIT = 0;
			CRC->DR = data.u32[0];
			for(uint16_t i = 0; i <= param; ++i) {
				CRC->DR = flash_write_buffer[i].u32[0];
				CRC->DR = flash_write_buffer[i].u32[1];
			}
			id = CMD_ID(CMD_STATUS);
			if(CRC->DR == data.u32[1]) {
				if(flash_write((void *) data.u32[0], flash_write_buffer, ((uint32_t) param + 1) << 3) != FLASH_OK) {
					len = 0;
					param = PROTOCOL_STATUS_COMMIT_FLASH_FAIL;
				} else {
					len = 0;
					param = PROTOCOL_STATUS_COMMIT_OK;
				}
			} else {
				len = 0;
				param = PROTOCOL_STATUS_COMMIT_CRC_FAIL;
			}
			goto transmit;
    }

    return;
transmit:
    canbus_transmit_fast(CAN_EXID(id | short_device_id | (param << 8)), &data, len);
}

int main(void) {
    init_crc();
    uint32_t device_id = compute_device_id();

    struct flash_eeprom_entry entry = {};
    struct flash_eeprom_entry *old_entry;
    if(flash_eeprom_get_addr(&old_entry) == FLASH_OK) {
        entry = *old_entry;
    } else {
        // State not configured, generate some default states and store it
        entry.is_free = 0;
        entry.short_device_id = device_id & 0xFF;
        entry.mode = MODE_BOOTLOADER;
        flash_eeprom_write(&entry);
    }

	canbus_init(entry.short_device_id);

    union u64_aliases reset_info = {};
	reset_info.u32[0] = INFO;
	reset_info.u8[3] = (RCC->CSR & 0xFF000000) >> 24;
	reset_info.u32[1] = device_id;

    // Boot message, include reset reason
	canbus_transmit_fast(CAN_EXID(CMD_ID(CMD_RESET_INFO) | (entry.mode << 8) | entry.short_device_id), &reset_info, 8);

	// Reset reset reason
	RCC->CSR |= RCC_CSR_RMVF;

	if(entry.mode == MODE_APPLICATION) {
	    // Reset bootloader key so ISR knows which mode we're in
	    bootloader_key = 0;

	    __disable_irq();

	    SCB->VTOR = (uint32_t) &_sapp_rom;

	    // Update stack pointer to the one specified in user application
	    __asm volatile("mov sp, %0" : : "r" (&_sapp_rom));
	    // Jump to the user application. Does not return
		__asm volatile("bx %0" : : "r" (&_sapp_rom + 1));
	} else {
        flash_unlock();
    }

    return 0;
}
