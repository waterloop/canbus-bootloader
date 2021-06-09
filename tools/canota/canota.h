#ifndef BOOT_LIB_H
#define BOOT_LIB_H

#include <linux/can.h>
#include <stdbool.h>
#include <stdint.h>

enum canota_mode {
	CANOTA_MODE_APPLICATION = 0x00,
	CANOTA_MODE_BOOTLOADER = 0x01,
	CANOTA_MODE_RESET_ONLY = 0xFF
};

struct canota_device_ctx;

struct canota_ctx;

struct canota_ctx *canota_init(const char *iface);
struct canota_device_ctx *canota_device_from_short_id(struct canota_ctx *ctx, uint8_t id);

bool canota_device_ident_update(struct canota_device_ctx *dev);
bool canota_mode_switch(struct canota_device_ctx *dev, enum canota_mode mode);
bool canota_change_short_id(struct canota_device_ctx *dev, uint8_t new_short_id);
bool canota_flash(struct canota_device_ctx *dev, uint32_t start_addr, void *data, uint32_t len);
bool canota_checksum(struct canota_device_ctx *dev, uint32_t checksum_start, uint32_t checksum_len, uint32_t *checksum_out);
bool canota_flash_erase(struct canota_device_ctx *dev, uint32_t erase_start, uint32_t erase_len);
bool canota_flash_write_page(struct canota_device_ctx *dev, void *data, uint32_t data_len, uint32_t addr);
bool canota_flash_write(struct canota_device_ctx *dev, void *data, uint32_t len, uint32_t start_addr);

#endif
