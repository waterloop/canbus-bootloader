#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <poll.h>
#include <stdarg.h>
#include <string.h>
#include "canota.h"

//int printf_tty(const char *format, ...) {
//    va_list args;
//    va_start(args, format);
//    int ret = 0;
//    if(isatty(1)) {
//        ret = vprintf(format, args);
//    }
//    va_end(args);
//    return ret;
//}

void print_usage(char *argv0) {
	fprintf(stderr, "Usage: %s INTERFACE COMMAND [ARGS...]\n", argv0);
	fprintf(stderr, "    COMMAND must be one of: [scan, info, change_id, mode, reset, erase, checksum, flash]\n");
}

char *decode_type(uint8_t type) {
	switch(type) {
		case 0:
			return "stm32l432kc";
		default:
			return "unknown";
	}
}

char *decode_mode(uint8_t mode) {
	if(mode == 0) {
		return "application";
	} else if(mode == 1) {
		return "bootloader";
	} else {
		return "unknown";
	}
}

void print_info(struct canota_device_ctx *dev) {
	printf("[0x%02x] device_id=0x%08x mode=%s version=%u.%u type=%s\n",
	       dev->short_device_id,
	       dev->device_id,
	       decode_mode(dev->mode),
	       dev->info.version_major, dev->info.version_minor,
	       decode_type(dev->info.device_type));
}

int cmd_scan(char *iface) {
	// TODO proper interface for scanning devices
	struct canota_ctx *ctx = canota_init(iface);
	if(!ctx) {
		return 1;
	}
	struct canota_device_ctx dev = {
			.ctx = ctx
	};
	for(int i = 0; i < 256; ++i) {
		dev.short_device_id = i;
		if(!canota_cmd_device_ident_req(&dev)) {
			return 1;
		}
	}
	struct mask_match match = {.match = 0x1F010000, .mask = 0x1FFF0000};
	struct can_frame frame;
	while(true) {
		if(!canota_raw_recv_frame(ctx, &frame, &match, 1)) {
			fprintf(stderr, "No more devices responded in time\n");
			return 0;
		}
		union u64_aliases data;
		data.u64 = *(uint64_t *) frame.data;
		dev.mode = (frame.can_id >> 8) & 0xFF;
		dev.short_device_id = frame.can_id & 0xFF;
		*(uint32_t *) &dev.info = data.u32[0];
		dev.device_id = data.u32[1];
		print_info(&dev);
	}
}

#define GET_DEV()\
	struct canota_ctx *ctx = canota_init(iface);\
	if(!ctx) {\
		return 1;\
	}\
	struct canota_device_ctx *dev = canota_device_from_short_id(ctx, short_device_id);\
	if(!dev) {\
		return 1;\
	}

int cmd_info(char *iface, uint8_t short_device_id) {
	GET_DEV();
	print_info(dev);
	return 0;
}

int cmd_change_id(char *iface, uint8_t short_device_id, uint8_t new_id) {
	GET_DEV();
	return canota_change_short_id(dev, new_id);
}

int cmd_mode(char *iface, uint8_t short_device_id, enum canota_mode mode) {
	GET_DEV();
	return !canota_mode_switch(dev, mode);
}

int cmd_erase(char *iface, uint8_t short_device_id, uint32_t start, uint32_t len) {
	GET_DEV();
	return !canota_flash_erase(dev, start, len);
}

int cmd_checksum(char *iface, uint8_t short_device_id, uint32_t start, uint32_t len) {
	GET_DEV();
	uint32_t checksum;
	bool res = canota_checksum(dev, start, len, &checksum);
	if(!res) {
		return 1;
	}
	printf("%08x\n", checksum);
	return 0;
}

int cmd_flash(char *iface, uint8_t short_device_id, char *file, uint32_t start, uint32_t len_limit) {
	FILE *f = fopen(file, "rb");
	if(!f) {
		perror("cannot open file");
		return 1;
	}
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	if(fsize > len_limit) {
		fsize = len_limit;
	}

	char *contents = malloc(fsize);
	fread(contents, 1, fsize, f);
	fclose(f);

	GET_DEV();
	return canota_flash_write(dev, contents, fsize, start);
}

uint32_t parse_uint32(char *str) {
	char *eptr;
	uint32_t val = strtoul(str, &eptr, 0);
	if(*eptr) {
		fprintf(stderr, "\"%s\" is not a valid unsigned integer\n", str);
		exit(1);
	}
	return val;
}

int main(int argc, char *argv[]) {
	if(argc < 3) {
		print_usage(argv[0]);
		return 1;
	}

	const char *ops[] = {"scan", "info", "mode", "erase", "checksum", "flash", "reset"};
	const int n_ops = sizeof(ops) / sizeof(ops[0]);
	int op = -1;

	if(strcmp(argv[2], "scan") == 0) {
		return cmd_scan(argv[1]);
	} else if(strcmp(argv[2], "info") == 0) {
		if(argc < 4) {
			fprintf(stderr, "Not enough arguments.\nUsage: %s %s info SHORT_ID\n", argv[0], argv[1]);
			return 1;
		}
		return cmd_info(argv[1], parse_uint32(argv[3]));
	} else if(strcmp(argv[2], "change_id") == 0) {
		if(argc < 5) {
			fprintf(stderr, "Not enough arguments.\nUsage: %s %s change_id SHORT_ID NEW_SHORT_ID\n", argv[0], argv[1]);
			return 1;
		}
		return cmd_change_id(argv[1], parse_uint32(argv[3]), parse_uint32(argv[4]));
	} else if(strcmp(argv[2], "mode") == 0) {
		if(argc < 5) {
			fprintf(stderr, "Not enough arguments.\nUsage: %s %s mode SHORT_ID MODE\n", argv[0], argv[1]);
			return 1;
		}
		return cmd_mode(argv[1], parse_uint32(argv[3]), parse_uint32(argv[4]));
	} else if(strcmp(argv[2], "reset") == 0) {
		if(argc < 4) {
			fprintf(stderr, "Not enough arguments.\nUsage: %s %s reset SHORT_ID\n", argv[0], argv[1]);
			return 1;
		}
		return cmd_mode(argv[1], parse_uint32(argv[3]), CANOTA_MODE_RESET_ONLY);
	} else if(strcmp(argv[2], "erase") == 0) {
		if(argc < 6) {
			fprintf(stderr, "Not enough arguments.\nUsage: %s %s erase SHORT_ID START_ADDR LENGTH\n", argv[0], argv[1]);
			return 1;
		}
		return cmd_erase(argv[1], parse_uint32(argv[3]), parse_uint32(argv[4]), parse_uint32(argv[5]));
	} else if(strcmp(argv[2], "checksum") == 0) {
		if(argc < 6) {
			fprintf(stderr, "Not enough arguments.\nUsage: %s %s checksum SHORT_ID START_ADDR LENGTH\n", argv[0], argv[1]);
			return 1;
		}
		return cmd_checksum(argv[1], parse_uint32(argv[3]), parse_uint32(argv[4]), parse_uint32(argv[5]));
	} else if(strcmp(argv[2], "flash") == 0) {
		if(argc < 6) {
			fprintf(stderr, "Not enough arguments.\nUsage: %s %s flash SHORT_ID FILE START_ADDR [LENGTH]\n", argv[0], argv[1]);
			return 1;
		}

		uint32_t len = 0xFFFFFFFF;

		if(argc >= 7) {
			len = parse_uint32(argv[6]);
		}

		return cmd_flash(argv[1], parse_uint32(argv[3]), argv[4], parse_uint32(argv[5]), len);
	}

	return 0;
}
