#include "pci/pci.h"
#include "char/char.h"
#include "qemu/timer.h"
#include "exec/address-spaces.h"
#include <linux/types.h>

#include "can-pci.h"


// Reset by hardware, p10
static void can_hardware_reset(CanState *s)
{
	s->mode		= 0x01;
	s->status 	= 0x0c;
}




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

static void buff2frame(uint8_t *buff, struct can_frame *can)
{
	uint8_t i;

	if (buff[0] & 0x40) // RTR
		can->can_id = 0x01 << 30;
	can->can_dlc = buff[0] & 0x0f;

	if (buff[0] & 0x80) { // Extended
		can->can_id |= 0x01 << 31;
		can->can_id |= buff[1] << 21;
		can->can_id |= buff[2] << 13;
		can->can_id |= buff[3] << 05;
		can->can_id |= buff[4] >> 03;
		for (i = 0; i < can->can_dlc; i++)
			can->data[i] = buff[5+i];
		for (; i < 8; i++)
			can->data[i] = 0;
	} else {
		can->can_id |= buff[1] << 03;
		can->can_id |= buff[2] >> 05;
		for (i = 0; i < can->can_dlc; i++)
			can->data[i] = buff[3+i];
		for (; i < 8; i++)
			can->data[i] = 0;
	}
}


static void can_mem_write(void *opaque, hwaddr addr, uint64_t val, unsigned size) 
{
    CanState 			*s = opaque;
	void    			*pci_mem_addr;
	int    				region_size;
	struct can_frame	can;

	pci_mem_addr = s->mem_base;  
	pci_mem_addr = ((char *)pci_mem_addr) + addr;  
	region_size  = (int)memory_region_size(&s->memio);
	if(addr > region_size)
		return ;

	DPRINTF("write 0x%llx addr(%d) to %p\n", val, (int)addr, pci_mem_addr);

	switch (addr) {
		case 0:
			s->mode = 0x1f & val;
			break;

		case 1: // Command register.
			if (0x01 & val) { // Send transmission request.
				memcpy(s->tx_buff, s->trx_buff, 13);
				buff2frame(s->tx_buff, &can);
				display_msg(&can);
				// write to the backends.
				qemu_chr_fe_write(s->chr, (uint8_t *)&can, sizeof(struct can_frame));
				qemu_irq_raise(s->irq);
			}
			break;
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
			if (s->mode & 0x01) { // Reset mode
				if (addr < 24)
					s->code_mask[addr - 16] = val;
			} else
				s->trx_buff[addr - 16] = val; // Operation mode
			break;
		case 127:
			if (val == 0x33)
				qemu_irq_lower(s->irq);
			break;
	}


//		qemu_mod_timer(s->transmit_timer, 0);
//		qemu_mod_timer(s->transmit_timer, qemu_get_clock_ns(vm_clock) + 8 * get_ticks_per_sec());
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
	DPRINTF("read addr %d, region size %d\n", (int)addr, region_size);
	if(addr > region_size)  
		return 0;

	switch (addr) {
		case 0:
			temp = s->mode;
			break;
		case 2: // Status register.
			temp = s->status;
			break;

		default:
			temp = 0xff;
	}

//	memcpy(&temp, pci_mem_addr, size);
	DPRINTF("read %d bytes of 0x%x from addr %d\n", size, temp, (int)addr);

	return temp;
}

static const MemoryRegionOps can_mem_ops = {  
	.read 		= can_mem_read,  
	.write 		= can_mem_write,  
	.endianness = DEVICE_LITTLE_ENDIAN,
    .impl 		= {
		// how many bytes can we read/write every time.
        .min_access_size = 1, 
        .max_access_size = 1,
    },
};


static void serial_xmit(void *opaque)
{
    CanState *s = opaque;
	static int count = 1;

	if (count) {
	printf("xxx33333333333333333333333xxxxxxxxxxxxxxx\n");
		qemu_irq_raise(s->irq);
		count = 0;
		qemu_mod_timer(s->transmit_timer, qemu_get_clock_ns(vm_clock) + get_ticks_per_sec()/1000);
	} else {
	printf("xxx55555555555555555555xxxxxxxxxxxxxxx\n");
		count = 1;		
		qemu_irq_lower(s->irq);
	}
}
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


    s->transmit_timer = qemu_new_timer_ns(vm_clock, (QEMUTimerCB *) serial_xmit, s);



	can_hardware_reset(s);

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


