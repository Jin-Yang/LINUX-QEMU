#include "pci/pci.h"
#include "char/char.h"
#include "qemu/timer.h"
#include "exec/address-spaces.h"
#include <linux/types.h>

#define PCI_MEM_SIZE				32
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
typedef __u32 canid_t;

struct can_frame {
	canid_t can_id;  /* 32 bit CAN_ID + EFF/RTR/ERR flags */
	__u8    can_dlc; /* data length code: 0 .. 8 */
	__u8    data[8] __attribute__((aligned(8)));
};




typedef struct CanState {
	/* Some registers ... */
    qemu_irq irq;
	void 			*mem_base;
	int 			offset;

    CharDriverState *chr;
    MemoryRegion 	portio;
    MemoryRegion 	memio;
} CanState;

typedef struct PCICanState {
    PCIDevice 		dev;
    CanState 		state;
} PCICanState;




const uint8_t whatever[] = "ttt";
static void can_ioport_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
//    CanState *s = opaque;
//	qemu_chr_fe_write(s->chr, whatever, 3); // write to the backends.
}

static uint64_t can_ioport_read(void *opaque, hwaddr addr, unsigned size)
{
	DPRINTF("%s-%s() called\n", __FILE__, __FUNCTION__);
    return 0;
}

const MemoryRegionOps can_io_ops = {
    .read 		= can_ioport_read,
    .write 		= can_ioport_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl 		= {
        .min_access_size = 1,
        .max_access_size = 1,
    },
};

static void display_msg(struct can_frame *msg)
{
	int i;

	printf("id 0x%x, data(%d):\n", (msg->can_id & 0x1fffffff), msg->can_dlc);
	for(i = 0; i < msg->can_dlc; i++) {
		printf("0x%02x ", msg->data[i]);
	}
	printf("\n");
}


static void can_mem_write(void *opaque, hwaddr addr, uint64_t val, unsigned size) 
{
    CanState 			*s = opaque;
	void    			*pci_mem_addr;
	int    				region_size;

	pci_mem_addr = s->mem_base;  
	pci_mem_addr = ((char *)pci_mem_addr) + addr;  
	region_size  = (int)memory_region_size(&s->memio);
	if(addr > region_size)
		return ;

	DPRINTF("write 0x%llx addr(%d) to %p\n", val, (int)addr, pci_mem_addr);
	if ((addr = 30) && (val == 0x55)) {
		display_msg((struct can_frame*)s->mem_base);
		qemu_chr_fe_write(s->chr, s->mem_base, sizeof(struct can_frame)); // write to the backends.
		printf("PPPPPPPPPPPPPPPPPPPP\n");
		s->offset = 0;
		return;
	}

	memcpy(pci_mem_addr, (void *)&val, size);
}  

static uint64_t can_mem_read(void *opaque, hwaddr addr, unsigned size) 
{  
    CanState 			*s = opaque;
	void    			*pci_mem_addr;  
	int     			region_size;
	unsigned int 		temp;

	pci_mem_addr = s->mem_base;  
	pci_mem_addr = ((char *)pci_mem_addr) + addr;  
	region_size  = memory_region_size(&s->memio);
	if(addr > region_size)  
		return 0;

	memcpy(&temp, pci_mem_addr, size);
	DPRINTF("read %d bytes of 0x%x from %p\n", size, temp, pci_mem_addr);

	return temp;
}

static const MemoryRegionOps can_mem_ops = {  
	.read 		= can_mem_read,  
	.write 		= can_mem_write,  
	.endianness = DEVICE_LITTLE_ENDIAN,
    .impl 		= {
		// how many bytes can we read/write every time.
        .min_access_size = 1, 
        .max_access_size = 32,
    },
};


static int can_pci_init(PCIDevice *dev)
{
	// Get the address of PCICanState through PCIDevice.
    PCICanState *pci = DO_UPCAST(PCICanState, dev, dev); 
    CanState *s = &pci->state;

	DPRINTF("%s-%s() called\n", __FILE__, __FUNCTION__);
	if (!s->chr) {
        fprintf(stderr, "Can't create can device, empty char device\n");
		exit(1);
    }
	
	s->mem_base = g_malloc0(PCI_MEM_SIZE);

    pci->dev.config[PCI_INTERRUPT_PIN] = 0x01;
    s->irq = pci->dev.irq[0];

	qemu_irq_lower(s->irq);
	
    memory_region_init_io(&s->portio, &can_io_ops, s, "can", 8);
    memory_region_init_io(&s->memio, &can_mem_ops, s, "can", PCI_MEM_SIZE);
    pci_register_bar(&pci->dev, IO_BAR, PCI_BASE_ADDRESS_SPACE_IO, &s->portio);
    pci_register_bar(&pci->dev, MEM_BAR, PCI_BASE_ADDRESS_SPACE_MEMORY, &s->memio);

//    memory_region_add_subregion(system_io, base, &s->io);
    return 0;
}

static void can_pci_exit(PCIDevice *dev)
{
    PCICanState *pci = DO_UPCAST(PCICanState, dev, dev);
    CanState *s = &pci->state;

    g_free(s->mem_base);
    memory_region_destroy(&s->memio);
    memory_region_destroy(&s->portio);
}


static const VMStateDescription vmstate_pci_can = {
    .name = "pci-can",
    .version_id = PCI_REVISION_ID_CANBUS,
    .minimum_version_id = 1,
    .fields      = (VMStateField[]) {
        VMSTATE_PCI_DEVICE(dev, PCICanState),
        VMSTATE_END_OF_LIST()
    }
};

static Property can_pci_properties[] = {
    DEFINE_PROP_CHR("chardev",  PCICanState, state.chr),
    DEFINE_PROP_END_OF_LIST(),
};

static void can_pci_class_initfn(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *pc = PCI_DEVICE_CLASS(klass);
	
    pc->init = can_pci_init;
    pc->exit = can_pci_exit;
    pc->vendor_id = PCI_VENDOR_ID_REDHAT; // 0x1b36
    pc->device_id = PCI_DEVICE_ID_CANBUS;
    pc->revision = PCI_REVISION_ID_CANBUS;
    pc->class_id = PCI_CLASS_OTHERS;

    dc->desc = "PCI CAN SJA1000";
    dc->vmsd = &vmstate_pci_can;
    dc->props = can_pci_properties;
}

static const TypeInfo can_pci_info = {
    .name          = "pci-can",
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(PCICanState),
    .class_init    = can_pci_class_initfn,
};

static void can_pci_register_types(void)
{
    type_register_static(&can_pci_info);
}

type_init(can_pci_register_types)
