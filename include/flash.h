#ifndef FLASH_H
#define FLASH_H

#include <stdint.h>

typedef enum {
    FLASH_OK = 0,
    FLASH_EEPROM_NO_ENTRIES = -1,
    FLASH_WRITE_FAIL = -2,
} flash_ret_t;

struct flash_eeprom_entry {
    uint8_t is_free: 1;
    uint64_t reserved0: 47;
    uint8_t device_state;
    uint8_t short_id;
} __attribute__ ((__packed__));

flash_ret_t flash_eeprom_find_free(struct flash_eeprom_entry **entry_addr);
flash_ret_t flash_eeprom_get_addr(struct flash_eeprom_entry **entry_addr);
flash_ret_t flash_eeprom_write(const struct flash_eeprom_entry *entry);

void flash_unlock(void);
void flash_lock(void);

flash_ret_t flash_erase(uint32_t page);
flash_ret_t flash_write(void *addr, const void *data, uint32_t len);


#endif
