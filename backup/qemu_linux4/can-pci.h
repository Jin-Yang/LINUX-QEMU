#ifndef HW_CAN_PCI_H
#define HW_CAN_PCI_H

#define PCI_MEM_SIZE				128
#define PCI_DEVICE_ID_CANBUS		0xbeef
#define PCI_REVISION_ID_CANBUS 		0x73 

#define IO_BAR						0
#define MEM_BAR						1



#define DEBUG_CAN
#ifdef DEBUG_CAN
#define DPRINTF(fmt, ...) \
   do { fprintf(stderr, "[mycan]: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
   do {} while (0)
#endif



/*
 * Controller Area Network Identifier structure
 *
 * bit 0-28	: CAN identifier (11/29 bit)
 * bit 29	: error frame flag (0 = data frame, 1 = error frame)
 * bit 30	: remote transmission request flag (1 = rtr frame)
 * bit 31	: frame format flag (0 = standard 11 bit, 1 = extended 29 bit)
 */
typedef uint32_t canid_t;

struct can_frame {
	canid_t can_id;  /* 32 bit CAN_ID + EFF/RTR/ERR flags */
	__u8    can_dlc; /* data length code: 0 .. 8 */
	__u8    data[8] __attribute__((aligned(8)));
};
/*
typedef struct SJA1000_PeliCAN {
	


} PeliCAN;
*/



typedef struct CanState {
	/* Some registers ... */
	uint8_t			mode; // Mode register, addr 0, DataSheet p26
	uint8_t			status; // PeliCAN, Status register, addr 2, p15
	
	uint8_t			command; //


	uint8_t			trx_buff[13]; // 16~28, RX/TX buffer, operatioin mode
	uint8_t			code_mask[8]; // 16~23, acceptance code/mask, reset mode
	uint8_t			rx_buff[64];  // 32~95
	uint8_t			tx_buff[13];  // 96~108









    qemu_irq 		irq;
	void 			*mem_base;
	int 			offset;







    struct QEMUTimer *transmit_timer;

    CharDriverState *chr;
    MemoryRegion 	portio;
    MemoryRegion 	memio;
} CanState;

typedef struct PCICanState {
    PCIDevice 		dev;
    CanState 		state;
} PCICanState;




#endif

