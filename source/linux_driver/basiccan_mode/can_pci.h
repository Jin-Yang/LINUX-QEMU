#ifndef CAN_PCI_H__
#define CAN_PCI_H__

#include <linux/kernel.h>  
#include <linux/module.h>  
#include <linux/pci.h>  
#include <linux/cdev.h>		/* It's a cdev */
#include <linux/init.h>  
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/errno.h>	/* error codes */
#include <linux/fs.h>		/* everything... */
#include <linux/kernel.h>	/* printk() */
#include <asm/uaccess.h>	/* copy_*_user */
#include <linux/delay.h>
#include <linux/slab.h>		/* kmalloc() */
#include <linux/cdev.h>		/* It's a cdev */


#define DEVICE_MAJOR		45


#define PCI_VENDOR_ID_REDHAT   		0x1b36
#define PCI_DEVICE_ID_CANBUS		0xbeef
#define PCI_REVISION_ID_CANBUS 		0x73  

#define IO_BAR						0
#define MEM_BAR						1

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




#define BUFFERSIZE		6

struct can_dev
{
	struct cdev 		cdev;
	loff_t 				valid;


	struct can_frame	tx_buf[BUFFERSIZE];
	size_t				tx_count;
	size_t				tx_in;
	size_t				tx_out;

	struct can_frame	rx_buf[BUFFERSIZE];
	size_t				rx_count;
	size_t				rx_in;
	size_t				rx_out;


	
	void __iomem *mem_base, *io_base;
	resource_size_t io_len, mem_len;


};


// Mode register, address 0
#define SJA_MODE_REG		0
#define RESET_BIT			(1 << 0)




// TX frame information, address 16, p40.
#define TX_FRAME_INFO_REG	16
// Standard Frame Format, SFF, bit7
#define SFF					(0 << 7)
// Extended Frame Format, EFF
#define EFF					(1 << 7)
// Remote Transmission Request, RTR
#define RTR					(1 << 6)
// Data Length Code bits, DLC
#define DLC(x)				(x)


// Status register, address 2
#define SJA_STATUS_REG		2
// Tranmit buffer status, 1-released 0-locked
#define TBS					(1 << 2)



// Command register, address 1, p13.
#define SJA_COMMAND_REG		1
// Clear Data Overrun
#define CDO					(1 << 3)



// Clock Divider register, address 31, Dp55 
#define SJA_CLOCK_REG		31
#define PELI				(1 << 7)



#define SJA_INT_REG			3
#define SJA_INT_EN_REG		4
#define RI					(1 << 0)
#define RIE					(1 << 0)
#define TI					(1 << 1)
#define TIE					(1 << 1)
#define DOI					(1 << 3)
#define DOIE				(1 << 3)





// Basic mode, Control register
#define SJA_CONTROL_REG		0
// Basic mode, Acceptance mask
#define SJA_BASIC_AMR_REG	5




//#define DEBUG_CAN
#ifdef DEBUG_CAN
#define DPRINTF(fmt, ...) \
   do { printk(KERN_WARNING "[can]: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
   do {} while (0)
#endif



#endif
