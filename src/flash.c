#include "flash.h"
#include "crc32.h"
#include "platform.h"

extern uint32_t _sapp_rom;
extern uint32_t _eapp_rom;
extern uint32_t _sboot_config;
extern uint32_t _eboot_config;

#define CHECKSUM_INIT_CRC 0xb102abec

void flash_eeprom_find_free(const struct flash_eeprom_entry **entry_addr) {
    uint32_t start = (uint32_t) &_sboot_config;
    uint32_t end = (uint32_t) &_eboot_config;
    while(end - start >= 8) {
        uint32_t mid = start + ((end - start) >> 1);
        if(*(uint64_t *) mid == 0xFFFFFFFFFFFFFFFFL) {
            end = mid;
        } else {
            start = mid;
        }
    }
    end &= ~7;
    *entry_addr = (struct flash_eeprom_entry *) end;
}

flash_ret_t flash_eeprom_read(const struct flash_eeprom_entry **entry_addr) {
    flash_eeprom_find_free(entry_addr);
    if(*entry_addr == (struct flash_eeprom_entry *) &_sboot_config) {
        return FLASH_EEPROM_NO_ENTRIES;
    }
	*(uint32_t *) entry_addr -= 8;
	if(((uint32_t *) *entry_addr)[1] != ~crc32(CHECKSUM_INIT_CRC, *entry_addr, 4)) {
		return FLASH_EEPROM_NO_ENTRIES;
	}
    return FLASH_OK;
}

flash_ret_t flash_eeprom_write(const struct flash_eeprom_entry *entry) {
	flash_ret_t ret;
	const struct flash_eeprom_entry *entry_addr;
	flash_eeprom_find_free(&entry_addr);
	for(uint8_t tries = 0; tries < 2; ++tries) {
		if(entry_addr == (void *) &_eboot_config) {
			if((ret = flash_erase_page(flash_addr_to_page((uint32_t) &_sboot_config)) != FLASH_OK)) {
				return ret;
			}
			entry_addr = (struct flash_eeprom_entry *) &_sboot_config;
		}
		uint32_t entry_to_write[2];
		entry_to_write[0] = *(uint32_t *) entry;
		entry_to_write[1] = ~crc32(CHECKSUM_INIT_CRC, &entry_to_write[0], 4);
		if((ret = flash_write((uint32_t) entry_addr, entry_to_write, 8)) == FLASH_OK) {
			return ret;
		}
		entry_addr = (void *) &_eboot_config;
	}
	return ret;
}

flash_ret_t flash_range_check(uint32_t start, uint32_t len) {
    if(start < (uint32_t) &_sapp_rom || start + len > (uint32_t) &_eapp_rom || len > (uint32_t) &_eapp_rom - (uint32_t) &_sapp_rom) {
        return FLASH_OUT_OF_BOUNDS;
    }
    return FLASH_OK;
}

flash_ret_t flash_erase_addr_range(uint32_t start, uint32_t len) {
    flash_ret_t ret = flash_range_check(start, len);
    if(ret != FLASH_OK) {
        return ret;
    }
    uint32_t startPage = flash_addr_to_page(start);
    uint32_t endPage = flash_addr_to_page(start + len - 1);
    for(uint32_t page = startPage; page <= endPage; ++page) {
        ret = flash_erase_page(page);
        if(ret != FLASH_OK) {
            return ret;
        }
    }
    return FLASH_OK;
}
