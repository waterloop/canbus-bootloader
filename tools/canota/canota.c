#include "canota.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <poll.h>
#include <linux/can.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include "crc32.h"

#define PROTOCOL_STATUS_NONE 0x00
#define PROTOCOL_STATUS_ERASE_OK 0x01
#define PROTOCOL_STATUS_COMMIT_OK 0x02
#define PROTOCOL_STATUS_ERASE_FAIL 0x81
#define PROTOCOL_STATUS_CRC_OUT_OF_BOUNDS 0x82
#define PROTOCOL_STATUS_COMMIT_CRC_FAIL 0x83
#define PROTOCOL_STATUS_COMMIT_FLASH_FAIL 0x84
#define PROTOCOL_STATUS_COMMIT_OUT_OF_BOUNDS 0x85

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
};

struct canota_ctx *canota_init(const char *iface) {
	int can_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if(can_fd < 0) {
		return NULL;
	}

	struct ifreq ifr;
	strncpy(ifr.ifr_name, iface, IFNAMSIZ);
	if(ioctl(can_fd, SIOCGIFINDEX, &ifr) == -1) {
		perror("[canota_lib] socketcan ioctl failed");
		return NULL;
	}

	struct sockaddr_can addr = {};
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if(bind(can_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		perror("[canota_lib] socketcan bind failed");
		return NULL;
	}

	struct canota_ctx *ctx = calloc(sizeof(struct canota_ctx), 1);
	ctx->can_fd = can_fd;
	ctx->can_pollfd.fd = can_fd;
	ctx->can_pollfd.events = POLLIN;
	ctx->timeout = 5000;
	return ctx;
}

bool canota_raw_cmd(struct canota_device_ctx *dev, uint8_t cmd, uint8_t param, void *data, uint8_t len) {
	if(len > 8) {
		return false;
	}
	struct can_frame frame = {
			.can_id = CAN_EFF_FLAG | 0x1F000000 | (cmd << 16) | (param << 8) | dev->short_device_id,
			.can_dlc = len,
	};
	memcpy(frame.data, data, len);
	return write(dev->ctx->can_fd, &frame, sizeof(frame)) == sizeof(frame);
}

struct mask_match {
	uint32_t mask;
	uint32_t match;
};

bool canota_raw_recv_frame(struct canota_ctx *ctx, struct can_frame *frame, struct mask_match *matches, size_t nmatches) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	uint64_t end = ts.tv_sec * 1000 + ts.tv_nsec / 1000000 + ctx->timeout;
	while(1) {
		clock_gettime(CLOCK_MONOTONIC, &ts);
		uint64_t now = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
		if(now >= end) {
			return false;
		}
		if(poll(&ctx->can_pollfd, 1, (int) (end - now)) == -1) {
			if(errno == EINTR || errno == EAGAIN) {
				continue;
			} else {
				perror("[canota_lib] poll failed");
				return false;
			}
		}
		if(ctx->can_pollfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
			fprintf(stderr, "[canota_lib] poll fd error: revents = %hx\n", ctx->can_pollfd.revents);
			return false;
		} else if(ctx->can_pollfd.revents & POLLIN) {
			if(read(ctx->can_fd, frame, sizeof(struct can_frame)) == -1) {
				perror("[canota_lib] read failed");
				return false;
			}
			for(size_t i = 0; i < nmatches; ++i) {
				if((frame->can_id & matches[i].mask) == matches[i].match) {
					return true;
				}
			}
		}
	}
}

bool canota_raw_recv(struct canota_device_ctx *dev, uint8_t cmd, uint8_t *param, void *data, uint8_t *len) {
	struct can_frame frame;
	struct mask_match match = {
			.mask = 0x1FFF00FF,
			.match = 0x1F000000 | (cmd << 16) | dev->short_device_id
	};
	bool success = canota_raw_recv_frame(dev->ctx, &frame, &match, 1);
	if(!success) {
		return false;
	}
	memcpy(data, frame.data, frame.can_dlc);
	*len = frame.can_dlc;
	*param = (frame.can_id >> 8) & 0xFF;
	return true;
}

bool canota_raw_recv_and_status(struct canota_device_ctx *dev, uint8_t cmd, uint8_t *param, void *data, uint8_t *len, uint8_t *status) {
	struct can_frame frame;
	struct mask_match match[2] = {
			{
					.mask = 0x1FFF00FF,
					.match = 0x1F000000 | (cmd << 16) | dev->short_device_id
			},
			{
					.mask = 0x1FFF00FF,
					.match = 0x1F000000 | (CMD_STATUS << 16) | dev->short_device_id
			},
	};
	bool success = canota_raw_recv_frame(dev->ctx, &frame, match, 2);
	if(!success) {
		return false;
	}
	memcpy(data, frame.data, frame.can_dlc);
	*len = frame.can_dlc;
	*param = (frame.can_id >> 8) & 0xFF;
	if((frame.can_id & 0xFF0000) == (CMD_STATUS << 16)) {
		*status = *param;
	} else {
		*status = PROTOCOL_STATUS_NONE;
	}
	return true;
}

#define CHECK(expr, msg, lbl) { if(!(expr)) {fprintf(stderr, "[canota_lib] " msg "\n"); goto lbl;} }

bool canota_cmd_device_ident_req(struct canota_device_ctx *dev) {
	return canota_raw_cmd(dev, CMD_DEVICE_IDENT_REQ, 0, NULL, 0);
}

bool canota_cmd_mode_switch(struct canota_device_ctx *dev, enum canota_mode mode) {
	union u64_aliases data = {.u32 = {*(uint32_t *) &dev->info, dev->device_id}};
	return canota_raw_cmd(dev, CMD_MODE_SWITCH, mode, &data, 8);
}

bool canota_cmd_change_short_id(struct canota_device_ctx *dev, uint8_t new_id) {
	union u64_aliases data = {.u32 = {*(uint32_t *) &dev->info, dev->device_id}};
	return canota_raw_cmd(dev, CMD_CHANGE_SHORT_ID, new_id, &data, 8);
}

bool canota_cmd_flash_erase(struct canota_device_ctx *dev, uint32_t start_addr, uint32_t len) {
	union u64_aliases data = {.u32 = {start_addr, len}};
	return canota_raw_cmd(dev, CMD_FLASH_ERASE, 0, &data, 8);
}

bool canota_cmd_flash_checksum(struct canota_device_ctx *dev, uint32_t start_addr, uint32_t len) {
	union u64_aliases data = {.u32 = {start_addr, len}};
	return canota_raw_cmd(dev, CMD_FLASH_CHECKSUM, 0, &data, 8);
}

bool canota_cmd_flash_buffer_write_data(struct canota_device_ctx *dev, void *data, uint8_t offset) {
	return canota_raw_cmd(dev, CMD_FLASH_BUFFER_WRITE_DATA, offset, data, 8);
}

bool canota_cmd_flash_buffer_commit(struct canota_device_ctx *dev, uint32_t addr, uint32_t checksum, uint8_t len) {
	union u64_aliases data = {.u32 = {addr, checksum}};
	return canota_raw_cmd(dev, CMD_FLASH_BUFFER_COMMIT, len, &data, 8);
}

bool canota_device_ident_update(struct canota_device_ctx *dev) {
	uint8_t len;
	uint8_t param;
	union u64_aliases data;
	CHECK(canota_cmd_device_ident_req(dev), "write fail", check_fail);
	CHECK(canota_raw_recv(dev, CMD_DEVICE_IDENT, &param, &data, &len), "no response to device_ident_req", check_fail);
	CHECK(len == 8, "device_ident recv length != 8", check_fail);
	dev->mode = param;
	*(uint32_t *) &dev->info = data.u32[0];
	dev->device_id = data.u32[1];
	return true;

check_fail:
	return false;
}

bool canota_mode_switch(struct canota_device_ctx *dev, enum canota_mode mode) {
	uint8_t len;
	uint8_t param;
	union u64_aliases data;
	CHECK(canota_cmd_mode_switch(dev, mode), "write fail", check_fail);
	CHECK(canota_raw_recv(dev, CMD_RESET_INFO, &param, &data, &len), "no response to mode_switch", check_fail);
	CHECK(len == 8, "reset_info recv len != 8", check_fail);
	dev->mode = param;
	*(uint32_t *) &dev->info = data.u32[0] & 0xFFFFFF;
	dev->device_id = data.u32[1];
	return true;
check_fail:
	return false;
}

bool canota_change_short_id(struct canota_device_ctx *dev, uint8_t new_short_id) {
	uint8_t len;
	uint8_t param;
	uint8_t old_short_id = dev->short_device_id;
	union u64_aliases data;
	CHECK(canota_cmd_change_short_id(dev, new_short_id), "write fail", check_fail);
	dev->short_device_id = new_short_id;
	CHECK(canota_raw_recv(dev, CMD_RESET_INFO, &param, &data, &len), "no response to change_short_id", check_fail);
	CHECK(len == 8, "reset_info recv len != 8", check_fail);
	dev->short_device_id = new_short_id;
	dev->mode = param;
	*(uint32_t *) &dev->info = data.u32[0] & 0xFFFFFF;
	dev->device_id = data.u32[1];
	return true;

check_fail:
	dev->short_device_id = old_short_id;
	return false;
}

// TODO TEST
bool canota_flash_erase(struct canota_device_ctx *dev, uint32_t erase_start, uint32_t erase_len) {
	uint8_t len;
	uint8_t param;
	union u64_aliases data;
	CHECK(canota_cmd_flash_erase(dev, erase_start, erase_len), "write fail", check_fail);
	CHECK(canota_raw_recv(dev, CMD_STATUS, &param, &data, &len), "no response to flash_erase", check_fail);
	CHECK(len == 0, "flash_erase status recv length != 0", check_fail);
	CHECK(param == PROTOCOL_STATUS_ERASE_OK, "erase failed", check_fail);
	return true;

check_fail:
	return false;
}

bool canota_flash_write_page(struct canota_device_ctx *dev, void *data, uint32_t data_len, uint32_t addr) {
	// Must be non zero length, <= 2048 bytes and a multiple of 8
	uint8_t len;
	uint8_t param;
	uint32_t crc = CRC32_INIT;

	CHECK((addr & 7) == 0 && data_len > 0 && data_len <= 2048 && (data_len & 7) == 0, "invalid parameters", check_fail);
	param = (data_len >> 3) - 1;
	for(uint32_t i = 0; i < data_len >> 3; ++i) {
		CHECK(canota_cmd_flash_buffer_write_data(dev, (uint8_t *) data + (i << 3), i), "write failed", check_fail);
		usleep(100);
	}
	crc32(&addr, 4, &crc);
	crc32(&param, 1, &crc);
	crc = crc32(data, data_len, &crc);
	CHECK(canota_cmd_flash_buffer_commit(dev, addr, crc, param), "write failed", check_fail);
	CHECK(canota_raw_recv(dev, CMD_STATUS, &param, &data, &len), "no response to flash_buffer_commit", check_fail);
	CHECK(len == 0, "flash_buffer_commit status recv length != 0", check_fail);
	CHECK(param == PROTOCOL_STATUS_COMMIT_OK, "flash_buffer_commit failed", check_fail);
	return true;

check_fail:
	return false;
}

bool canota_checksum(struct canota_device_ctx *dev, uint32_t checksum_start, uint32_t checksum_len, uint32_t *checksum_out) {
	uint8_t len;
	uint8_t param;
	uint8_t status;
	union u64_aliases data;
	CHECK(canota_cmd_flash_checksum(dev, checksum_start, checksum_len), "write fail", check_fail);
	CHECK(canota_raw_recv_and_status(dev, CMD_FLASH_CHECKSUM_RESP, &param, &data, &len,&status), "no response to flash_checksum", check_fail);
	CHECK(status == PROTOCOL_STATUS_NONE, "checksum request failed, out of range", check_fail);
	CHECK(len == 8, "reset_info recv len != 8", check_fail);
	CHECK(data.u32[0] == checksum_start, "checksum response start invalid", check_fail);
	*checksum_out = data.u32[1];
	return true;

check_fail:
	return false;
}

struct canota_device_ctx *canota_device_from_short_id(struct canota_ctx *ctx, uint8_t id) {
	struct canota_device_ctx *dev = calloc(sizeof(struct canota_device_ctx), 1);
	dev->ctx = ctx;
	dev->short_device_id = id;
	if(!canota_device_ident_update(dev)) {
		free(dev);
		return NULL;
	}
	return dev;
}

bool canota_flash_write(struct canota_device_ctx *dev, void *data, uint32_t len, uint32_t start_addr) {
	CHECK((start_addr & 7) == 0 && (len & 7) == 0, "invalid parameters", check_fail);
	CHECK(canota_flash_erase(dev, start_addr, len), "erase fail", check_fail);
	for(uint32_t i = 0; i < ((len + 2047) >> 11); ++i) {
		uint32_t offset = i << 11;
		uint32_t chunk_len = len - offset;
		if(chunk_len > 2048) {
			chunk_len = 2048;
		}
		bool success = false;
		for(int tries = 0; tries < 5; ++tries) {
			if(canota_flash_write_page(dev, (uint8_t *) data + offset, chunk_len, start_addr + offset)) {
				fprintf(stdout, "[canota_lib] written %d bytes at 0x%08x\n", chunk_len, start_addr + offset);
				success = true;
				break;
			}
			fprintf(stderr, "[canota_lib] retrying");
		}
		CHECK(success, "could not commit page to flash", check_fail);
	}
	uint32_t crc = CRC32_INIT;
	crc = crc32(data, len, &crc);
	uint32_t checksum_recv;
	CHECK(canota_checksum(dev, start_addr, len, &checksum_recv), "checksum recv fail", check_fail);
	CHECK(checksum_recv == crc, "flash_write incorrect checksum", check_fail);
	return true;

check_fail:
	return false;
}