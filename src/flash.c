#include "flash.h"
#include "utils.h"

#include <system_inc.h>

extern uint32_t _sapp_rom;
extern uint32_t _eapp_rom;
extern uint32_t _sboot_config;
extern uint32_t _eboot_config;

#define PAGE_SIZE_SHIFT 11
#define ADDR_TO_PAGE(x) ((x - FLASH_BASE) >> PAGE_SIZE_SHIFT)

#define EEPROM_PAGE_START ADDR_TO_PAGE((uint32_t) (&_sboot_config))
#define EEPROM_PAGE_END ADDR_TO_PAGE((uint32_t) (&_eboot_config))

void flash_eeprom_find_free(struct flash_eeprom_entry **entry_addr) {
    uint32_t start = (uint32_t) &_sboot_config;
    uint32_t end = (uint32_t) &_eboot_config;
    while(end - start >= sizeof(struct flash_eeprom_entry)) {
        uint32_t mid = start + ((end - start) >> 1);
        if(*(uint64_t *) mid == 0xFFFFFFFFFFFFFFFFL) {
            end = mid;
        } else {
            start = mid;
        }
    }
    end &= ~(sizeof(struct flash_eeprom_entry) - 1);
    *entry_addr = (struct flash_eeprom_entry *) end;
}

flash_ret_t flash_eeprom_get_addr(struct flash_eeprom_entry **entry_addr) {
    flash_eeprom_find_free(entry_addr);
    if(*entry_addr == (struct flash_eeprom_entry *) &_sboot_config) {
        return FLASH_EEPROM_NO_ENTRIES;
    }
    *entry_addr -= 1;
    return FLASH_OK;
}

void flash_unlock(void) {
    // Unlock FLASH_CR register
    FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;
}

void flash_lock(void) {
    FLASH->CR |= FLASH_CR_LOCK;
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
    uint32_t startPage = (start - FLASH_BASE) >> PAGE_SIZE_SHIFT;
    uint32_t endPage = (start + len - 1 - FLASH_BASE) >> PAGE_SIZE_SHIFT;
    for(uint32_t page = startPage; page <= endPage; ++page) {
        ret = flash_erase_page(page);
        if(ret != FLASH_OK) {
            return ret;
        }
    }
    return FLASH_OK;
}

flash_ret_t flash_eeprom_write(const struct flash_eeprom_entry *entry) {
    flash_ret_t ret;
    struct flash_eeprom_entry *entry_addr;
    flash_eeprom_find_free(&entry_addr);
    if(entry_addr == (void *) &_eboot_config) {
        for(uint32_t page = EEPROM_PAGE_START; page < EEPROM_PAGE_END; ++page) {
            ret = flash_erase_page(page);
            if(ret != FLASH_OK) {
                return ret;
            }
        }
        entry_addr = (struct flash_eeprom_entry *) &_sboot_config;
    }
    return flash_write(entry_addr, entry, 8);
}