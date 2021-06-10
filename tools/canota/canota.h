#ifndef BOOT_LIB_H
#define BOOT_LIB_H

#include <linux/can.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <poll.h>

union u64_aliases {
	uint64_t u64;
	uint32_t u32[2];
	uint16_t u16[4];
	uint8_t u8[8];
};

enum canota_mode {
	CANOTA_MODE_APPLICATION = 0x00,
	CANOTA_MODE_BOOTLOADER = 0x01,
	CANOTA_MODE_RESET_ONLY = 0xFF
};

struct canota_device_ctx {
	struct canota_ctx *ctx;
	uint32_t device_id;
	uint8_t short_device_id;
	enum canota_mode mode;
	struct {
		uint8_t device_type;
		uint8_t version_minor;
		uint8_t version_major;
		uint8_t reserved0;
	} __attribute__((__packed__)) info;
};

struct canota_ctx {
	int can_fd;
	struct pollfd can_pollfd;
	uint32_t timeout;
	uint32_t write_delay;
};

struct mask_match {
	uint32_t mask;
	uint32_t match;
};

struct canota_ctx *canota_init(const char *iface);
struct canota_device_ctx *canota_device_from_short_id(struct canota_ctx *ctx, uint8_t id);

bool canota_raw_cmd(struct canota_device_ctx *dev, uint8_t cmd, uint8_t param, void *data, uint8_t len);
bool canota_raw_recv_frame(struct canota_ctx *ctx, struct can_frame *frame, struct mask_match *matches, size_t nmatches);
bool canota_raw_recv(struct canota_device_ctx *dev, uint8_t cmd, uint8_t *param, void *data, uint8_t *len);
bool canota_raw_recv_and_status(struct canota_device_ctx *dev, uint8_t cmd, uint8_t *param, void *data, uint8_t *len, uint8_t *status);
bool canota_cmd_device_ident_req(struct canota_device_ctx *dev);
bool canota_cmd_mode_switch(struct canota_device_ctx *dev, enum canota_mode mode);
bool canota_cmd_change_short_id(struct canota_device_ctx *dev, uint8_t new_id);
bool canota_cmd_flash_erase(struct canota_device_ctx *dev, uint32_t start_addr, uint32_t len);
bool canota_cmd_flash_checksum(struct canota_device_ctx *dev, uint32_t start_addr, uint32_t len);
bool canota_cmd_flash_buffer_write_data(struct canota_device_ctx *dev, void *data, uint8_t offset);
bool canota_cmd_flash_buffer_commit(struct canota_device_ctx *dev, uint32_t addr, uint32_t checksum, uint8_t len);

bool canota_device_ident_update(struct canota_device_ctx *dev);
bool canota_mode_switch(struct canota_device_ctx *dev, enum canota_mode mode);
bool canota_change_short_id(struct canota_device_ctx *dev, uint8_t new_short_id);
bool canota_flash(struct canota_device_ctx *dev, uint32_t start_addr, void *data, uint32_t len);
bool canota_checksum(struct canota_device_ctx *dev, uint32_t checksum_start, uint32_t checksum_len, uint32_t *checksum_out);
bool canota_flash_erase(struct canota_device_ctx *dev, uint32_t erase_start, uint32_t erase_len);
bool canota_flash_write_page(struct canota_device_ctx *dev, void *data, uint32_t data_len, uint32_t addr);
bool canota_flash_write(struct canota_device_ctx *dev, void *data, uint32_t len, uint32_t start_addr);

#endif
