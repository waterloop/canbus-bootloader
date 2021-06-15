#ifndef FLASH_H
#define FLASH_H

#include <stdint.h>

typedef enum {
    FLASH_OK = 0,
    FLASH_EEPROM_NO_ENTRIES = -1,
    FLASH_WRITE_FAIL = -2,
    FLASH_ERASE_FAIL = -3,
    FLASH_OUT_OF_BOUNDS = -4,
} flash_ret_t;

struct flash_eeprom_entry {
    uint8_t is_free: 1;
    uint64_t reserved0: 15;
    uint8_t mode;
    uint8_t short_device_id;
    uint32_t reserved1;
} __attribute__ ((__packed__));

void flash_eeprom_find_free(struct flash_eeprom_entry **entry_addr);
flash_ret_t flash_eeprom_get_addr(struct flash_eeprom_entry **entry_addr);
flash_ret_t flash_eeprom_write(const struct flash_eeprom_entry *entry);

void flash_unlock(void);
void flash_lock(void);

flash_ret_t flash_range_check(uint32_t start, uint32_t len);
flash_ret_t flash_erase_addr_range(uint32_t start, uint32_t len);

#endif
