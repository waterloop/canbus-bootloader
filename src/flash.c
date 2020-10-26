#include "flash.h"
#include "utils.h"

#include <stm32l432xx.h>

extern uint32_t _sboot_config;
extern uint32_t _eboot_config;

#define PAGE_SIZE_SHIFT 11
#define PAGE_SIZE (1 << PAGE_SIZE_SHIFT)

#define EEPROM_PAGE_START ((uint32_t) (&_sboot_config) >> PAGE_SIZE_SHIFT)
#define EEPROM_PAGE_END ((uint32_t) (&_eboot_config) >> PAGE_SIZE_SHIFT)

flash_ret_t flash_eeprom_find_free(struct flash_eeprom_entry **entry_addr) {
    uint32_t start = (uint32_t) &_sboot_config;
    uint32_t end = (uint32_t) &_eboot_config;
    while(end - start >= sizeof(struct flash_eeprom_entry)) {
        uint32_t mid = start + ((end - start) >> 1);
        if(((struct flash_eeprom_entry *) mid)->is_free) {
            end = mid;
        } else {
            start = mid;
        }
    }
    end &= ~(sizeof(struct flash_eeprom_entry) - 1);
    *entry_addr = (struct flash_eeprom_entry *) end;
    return FLASH_OK;
}

flash_ret_t flash_eeprom_get_addr(struct flash_eeprom_entry **entry_addr) {
    flash_eeprom_find_free(entry_addr);
    if(entry_addr == (void *) &_sboot_config) {
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

static void flash_wait_write_ready(void) {
    while(FLASH->SR & FLASH_SR_BSY_Msk);
    // Clear all error bits
    FLASH->SR |= FLASH_SR_PROGERR | FLASH_SR_SIZERR | FLASH_SR_PGAERR | FLASH_SR_MISERR | FLASH_SR_FASTERR;
    FLASH->CR = (FLASH->CR & ~(FLASH_CR_PNB_Msk | FLASH_CR_MER1_Msk | FLASH_CR_PG_Msk | FLASH_CR_PER_Msk));
}

flash_ret_t flash_erase(uint32_t page) {
    flash_wait_write_ready();
    FLASH->CR |= REG_FIELD(FLASH_CR_PNB, page) | FLASH_CR_PER;
    FLASH->CR |= FLASH_CR_STRT;
    if(FLASH->CR & FLASH_SR_PGSERR) {
        return FLASH_WRITE_FAIL;
    }
    return FLASH_OK;
}

flash_ret_t flash_write(void *addr, const void *data, uint32_t len) {
    flash_wait_write_ready();
    FLASH->CR |= FLASH_CR_PG;
    uint32_t *dst = addr;
    const uint32_t *src = data;
    uint32_t *end = (uint32_t *) ((uint8_t *) src + len);
    while(src < end) {
        *dst = *src;
        *(dst + 1) = *(src + 1);
        src += 2;
        dst += 2;
        while(FLASH->SR & FLASH_SR_BSY_Msk);
        // Check for success
        if(!(FLASH->SR & FLASH_SR_EOP_Msk)) {
            return FLASH_WRITE_FAIL;
        }
        FLASH->SR |= FLASH_SR_EOP;
    }
    FLASH->CR &= ~FLASH_CR_PG;
    return FLASH_OK;
}

flash_ret_t flash_eeprom_write(const struct flash_eeprom_entry *entry) {
    flash_ret_t ret;
    struct flash_eeprom_entry *entry_addr;
    ret = flash_eeprom_find_free(&entry_addr);
    if(ret) {
        return ret;
    }
    if(entry_addr == (void *) &_eboot_config) {
        for(uint32_t page = EEPROM_PAGE_START; page < EEPROM_PAGE_END; ++page) {
            ret = flash_erase(_sboot_config);
            if(ret != FLASH_OK) {
                return ret;
            }
        }
        entry_addr = (struct flash_eeprom_entry *) &_sboot_config;
    }
    return flash_write(entry_addr, entry, 8);
}