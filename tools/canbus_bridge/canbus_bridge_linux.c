#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <poll.h>

int set_interface_attribs(int fd, int speed, int parity) {
	struct termios tty;
	if(tcgetattr(fd, &tty) != 0) {
		perror("tcgetattr");
		return -1;
	}

	cfsetospeed(&tty, speed);
	cfsetispeed(&tty, speed);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
	// disable IGNBRK for mismatched speed tests; otherwise receive break
	// as \000 chars
	tty.c_iflag &= ~IGNBRK;         // disable break processing
	tty.c_lflag = 0;                // no signaling chars, no echo,
	// no canonical processing
	tty.c_oflag = 0;                // no remapping, no delays

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

	tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
	// enable reading
	tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	if(tcsetattr(fd, TCSANOW, &tty) != 0) {
		perror("tcsetattr");
		return -1;
	}
	return 0;
}


int main(int argc, char *argv[]) {
	if(argc < 3) {
		fprintf(stderr, "Usage: %s <serial port> <socketcan interface>\n", argv[0]);
		return 1;
	}

	uint8_t serial_buf[16];
	struct can_frame can_frame;

	int serial_fd = open(argv[1], O_RDWR | O_NOCTTY | O_SYNC);
	if(serial_fd == -1) {
		perror("open");
		return 1;
	}
	set_interface_attribs(serial_fd, B115200, 0);

	int can_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if(can_fd < 0) {
		perror("socket");
		return 1;
	}

	struct ifreq ifr;
	strncpy(ifr.ifr_name, argv[2], IFNAMSIZ);
	ioctl(can_fd, SIOCGIFINDEX, &ifr);

	struct sockaddr_can addr = {};
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if(bind(can_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		perror("bind");
		return 1;
	}

	struct pollfd pfds[2] = {
		{
			.fd = serial_fd,
			.events = POLLIN
		}, 
		{
			.fd = can_fd,
			.events = POLLIN
		}
	};

	char buf[4];

	while(1) {
		if(poll(pfds, 2, -1) == -1) {
			perror("poll");
			return 1;
		}
		for(unsigned i = 0; i < 2; ++i) {
			if(pfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
				fprintf(stderr, "%s error, revents = %hx\n", i ? "can_fd" : "serial_fd", pfds[i].revents);
				return 1;
			}
			if(pfds[i].revents & POLLIN) {
				if(i == 0) {
					int n = read(serial_fd, &serial_buf, sizeof(serial_buf));
					if(n == -1) {
						perror("read");
						return 1;
					}
					can_frame.can_id = *(uint32_t *) serial_buf;
					if(serial_buf[4]) {
						can_frame.can_id |= CAN_EFF_FLAG;
					}
					can_frame.can_dlc = serial_buf[5];
					memcpy(can_frame.data, serial_buf + 8, 8);
					if(write(can_fd, &can_frame, sizeof(struct can_frame)) == -1) {
						perror("write");
						return 1;
					}
				} else if(i == 1) {
					int n = read(can_fd, &can_frame, sizeof(struct can_frame));
					if(n == -1) {
						perror("read");
						return 1;
					}
					*(uint32_t *) serial_buf = can_frame.can_id & CAN_EFF_MASK;
					serial_buf[4] = can_frame.can_id & CAN_EFF_FLAG ? 1 : 0;
					serial_buf[5] = can_frame.can_dlc;
					memcpy(serial_buf + 8, can_frame.data, can_frame.can_dlc);
					if(write(serial_fd, serial_buf, 16) == -1) {
						perror("write");
						return 1;
					}
				}
			}
		}
	}

	return 0;
}
