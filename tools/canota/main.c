#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <poll.h>
#include <stdarg.h>
#include "canota.h"

int printf_tty(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int ret = 0;
    if(isatty(1)) {
        ret = vprintf(format, args);
    }
    va_end(args);
    return ret;
}

//int command_scan(int argc, char *argv[]) {
//	printf_tty("Sending requests...\n");
//	struct can_frame frame = {};
//	for(unsigned id = 0; id < 256; ++id) {
//		frame.can_id = CAN_EFF_FLAG | 0x11000000 | id;
//		canbus_send(&frame);
//	}
//	printf_tty("Waiting for responses...\n\n");
//    printf_tty("id \tdevice_id\n---------------------------\n");
//	unsigned cnt = 0;
//	while(canbus_recv(&frame, 0x1FFF0000, 0x11010000)) {
//		if(frame.can_dlc == 8) {
//			printf("%u\t%016llx\n", frame.can_id & 0xFF, *(uint64_t *) frame.data);
//            cnt ++;
//		}
//	}
//    printf_tty("\nReceived %u response(s)\n", cnt);
//	return 0;
//}

int main(int argc, char *argv[]) {
//	char *str = "The quick brown fox jumps over ";
//	uint32_t crc = CRC32_INIT;
//	printf("%x\n", crc32(str, strlen(str), &crc));
//	str = "the lazy dog";
//	printf("%x\n", crc32(str, strlen(str), &crc));
//	return 0;
	struct canota_ctx *ctx = canota_init("vcan0");
	struct canota_device_ctx *dev = canota_device_from_short_id(ctx, 0x85);

	FILE *f = fopen("/home/jacky/stm32/Test/build/Test.bin", "rb");
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

	char *file = malloc(fsize);
	fread(file, 1, fsize, f);
	fclose(f);

//	canota_mode_switch(dev, CANOTA_MODE_BOOTLOADER);

	canota_flash_write(dev, file, fsize, 0x08002000);
	canota_mode_switch(dev, CANOTA_MODE_APPLICATION);
//	canota_flash_erase(dev, 0x08002000, 10);

//	uint32_t checksum;
//	canota_checksum(dev, 0x8002000, 0x3E000, &checksum);
//	printf("%08x\n", checksum);



//	canota_mode_switch(dev, CANOTA_MODE_BOOTLOADER);

//	canota_change_short_id(dev, 0x85);

//	canota_flash(dev, 0, NULL, 0);

	return 0;

//	if(argc < 3) {
//		fprintf(stderr, "Usage: %s <iface> <command> [<args>]\n", argv[0]);
//		return 1;
//	}
//
//	// Initialize socketcan
//	can_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
//	if(can_fd < 0) {
//		perror("socketcan socket failed");
//		return 1;
//	}
//
//	struct ifreq ifr;
//	strncpy(ifr.ifr_name, argv[1], IFNAMSIZ);
//	ioctl(can_fd, SIOCGIFINDEX, &ifr);
//
//	struct sockaddr_can addr = {};
//	addr.can_family = AF_CAN;
//	addr.can_ifindex = ifr.ifr_ifindex;
//
//	if(bind(can_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
//		perror("socketcan bind failed");
//		return 1;
//	}
//
//    can_pollfd.fd = can_fd;
//	can_pollfd.events = POLLIN;
//
//	timeout = 100;
//
//	// Process commands
//	char *cmd = argv[2];
//
//	if(strcmp(cmd, "scan") == 0) {
//		return command_scan(argc - 2, argv + 2);
//	}
	
	return 0;
}
