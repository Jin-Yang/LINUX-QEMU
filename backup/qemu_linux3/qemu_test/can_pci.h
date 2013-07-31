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
	struct can_frame	mem[BUFFERSIZE];


	
	void __iomem *mem_base, *io_base;
	resource_size_t io_len, mem_len;


};











#define DEBUG_CAN
#ifdef DEBUG_CAN
#define DPRINTF(fmt, ...) \
   do { printk(KERN_WARNING "[can]: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
   do {} while (0)
#endif




