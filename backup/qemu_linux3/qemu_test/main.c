#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <linux/types.h>


/*
 * Controller Area Network Identifier structure
 *
 * bit 0-28	: CAN identifier (11/29 bit)
 * bit 29	: error frame flag (0 = data frame, 1 = error frame)
 * bit 30	: remote transmission request flag (1 = rtr frame)
 * bit 31	: frame format flag (0 = standard 11 bit, 1 = extended 29 bit)
 */
typedef __u32 canid_t;

struct can_frame {
	canid_t can_id;  /* 32 bit CAN_ID + EFF/RTR/ERR flags */
	__u8    can_dlc; /* data length code: 0 .. 8 */
	__u8    data[8] __attribute__((aligned(8)));
};



void display(struct can_frame *msg)
{
	int i;

	printf("id 0x%x, data(%d):\n", (msg->can_id & 0x1fffffff), msg->can_dlc);
	for(i = 0; i < msg->can_dlc; i++) {
		printf("0x%02x ", msg->data[i]);
	}
	printf("\n");
}


int main( void )
{
	int fp = 0, i;
	struct can_frame cansnd, canrcv;
	memset(&cansnd, 0, sizeof(cansnd));
	
	cansnd.can_id = 0x987;
	cansnd.can_dlc = 3;
	cansnd.data[0] = 0x12;
	cansnd.data[1] = 0x34;
	cansnd.data[2] = 0x56;

	fp = open("/dev/can0", O_RDWR, S_IRUSR|S_IWUSR);
	if (fp < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	if (write(fp, &cansnd, sizeof(struct can_frame)) < 0) {
		perror("write");
		close(fp);
		exit(EXIT_FAILURE);
	}

	i = read(fp, &canrcv, sizeof(struct can_frame));
	printf("Read %d bytes\n", i);
	display(&canrcv);

	close(fp);
}

