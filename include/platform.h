#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include "flash.h"

#define REG_FIELD(field, value) ((value) << field ## _Pos)
#define REG_FIELD_GET(value, field) (((value) & field ## _Msk) >> field ## _Pos)

uint32_t flash_addr_to_page(uint32_t start);
flash_ret_t flash_erase_page(uint32_t page);
flash_ret_t flash_write(void *addr, const void *data, uint32_t len);
void flash_unlock(void);
void flash_lock(void);

uint32_t compute_device_id(void);
void sleep(uint32_t ms);
void start_user_app();

#endif
