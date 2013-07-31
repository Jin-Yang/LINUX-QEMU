/* 
 * QEMU memory pci emulation (PCI to ISA bridge) 
 *
*************************************************************************
Additionaly, the following should be done.
1. there are two ways to add this device to QEMU
  * add the following code to hw/pc_piix.c-pc_init1()
   ------------------------------  BEGIN  -------------------------------
    pc_cmos_init(below_4g_mem_size, above_4g_mem_size, boot_device,
                 floppy, idebus[0], idebus[1], rtc_state);
	// Next is what we should add.
	pci_create_simple_multifunction(pci_bus, -1, true ,"pci_simple_tst");
   -------------------------------  END  --------------------------------
  * OR, when start QEMU pass argument "-device pci_simple_tst" to it.

2. add the following to hw/Makefile.objs
	hw-obj-y		+= pci_testdev2.c
*/
#include "hw/hw.h" 
#include "char/char.h"
#include "hw/pci/pci.h"

#define PCI_DEVICE_ID_SIMPLE_TST	0x1234
#define PCI_REVISION_ID_SIMPLE_TST	0x73
#define PCI_MEM_SIZE				16 

typedef struct SimplePCIState { 
    MemoryRegion mmio;
    MemoryRegion portio;
} SimplePCIState;  
      
typedef struct PCISimpleTstState {  
	PCIDevice 		pci_dev;
    MemoryRegion 	mmio;
    MemoryRegion 	portio;

	void 			*pci_mem_base;  
	SimplePCIState 	state;
} PCISimpleTstState;  

static void mem_pci_write(void *opaque, hwaddr addr,  
						  uint64_t value, unsigned int size)  
{
    PCISimpleTstState 	*s = opaque;
	void    			*pci_mem_addr;
	int    				region_size;

	pci_mem_addr = s->pci_mem_base;  
	pci_mem_addr = ((char *)pci_mem_addr) + addr;  
	region_size = (int)memory_region_size(&s->mmio);  

	if(addr > region_size)
		return ;  
	printf("write 0x%llx to %p\n", value, pci_mem_addr);
	memcpy(pci_mem_addr, (void *)&value, size);	
	printf("   pci_mem_addr 0x%llx\n", *(uint64_t *)pci_mem_addr);

}  

static uint64_t mem_pci_read(void *opaque, hwaddr addr,  
                             unsigned int size)  
{  
	void    *pci_mem_addr;  
	int     region_size;
	unsigned int temp;

	pci_mem_addr = (void *)((PCISimpleTstState *)opaque)->pci_mem_base;  
	pci_mem_addr = ((char *)pci_mem_addr) + addr;  
	region_size = memory_region_size(&((PCISimpleTstState *)opaque)->mmio);      
      
	if(addr > region_size)  
		return 0;

	memcpy(&temp, pci_mem_addr, size);
	printf("read %d bytes of 0x%x from %p\n", size, temp, pci_mem_addr);

	return temp;
}  

static const MemoryRegionOps pci_simple_tst_mmio_ops = {  
	.read = mem_pci_read,  
	.write = mem_pci_write,  
	.endianness = DEVICE_LITTLE_ENDIAN,
/*    .impl = {
        .min_access_size = 1,
        .max_access_size = 1,
    },*/
};

static const MemoryRegionOps pci_simple_tst_pio_ops = {  
	.read = mem_pci_read,  
	.write = mem_pci_write,  
	.endianness = DEVICE_LITTLE_ENDIAN,
/*    .impl = {
        .min_access_size = 1,
        .max_access_size = 1,
    },*/
};  

static Property pci_simple_tst_properties[] = {
	DEFINE_PROP_PTR("membase", PCISimpleTstState, pci_mem_base),
	DEFINE_PROP_END_OF_LIST()  
};  

static int pci_simple_tst_init(PCIDevice *dev)
{
	PCISimpleTstState *pci = DO_UPCAST(PCISimpleTstState, pci_dev, dev);

	pci->pci_mem_base    = g_malloc0(PCI_MEM_SIZE);

	memory_region_init_io(&pci->mmio, &pci_simple_tst_mmio_ops, pci,
						  "pci_simple_tst_mmio", PCI_MEM_SIZE); 
	memory_region_init_io(&pci->portio, &pci_simple_tst_pio_ops, pci,
						  "pci_simple_tst_portio", PCI_MEM_SIZE);
 
    pci_register_bar(dev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &pci->mmio);
    pci_register_bar(dev, 1, PCI_BASE_ADDRESS_SPACE_IO, &pci->portio); 

	return 0;  
}  


static void pci_simple_tst_exit(PCIDevice *dev)
{
	PCISimpleTstState *pci = DO_UPCAST(PCISimpleTstState, pci_dev, dev);


    g_free(pci->pci_mem_base);
    memory_region_destroy(&pci->mmio);
    memory_region_destroy(&pci->portio);
}

static const VMStateDescription vmstate_pci_simple_tst = {  
	.name = "pci_simple_tst",  
	.version_id = 0,  
	.minimum_version_id = 0,  
	.fields = (VMStateField[]) {
		VMSTATE_PCI_DEVICE(pci_dev, PCISimpleTstState),  
		VMSTATE_END_OF_LIST()  
	},  
};


static void pci_simple_tst_class_init(ObjectClass *klass, void *data)  
{
	PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);
	DeviceClass *dc = DEVICE_CLASS(klass);
      
	k->init = pci_simple_tst_init;
    k->exit = pci_simple_tst_exit;
	k->vendor_id = PCI_VENDOR_ID_REDHAT;
	k->device_id = PCI_DEVICE_ID_SIMPLE_TST;
	k->revision  = PCI_REVISION_ID_SIMPLE_TST;

	dc->vmsd = &vmstate_pci_simple_tst;
	dc->props = pci_simple_tst_properties;
}  

static TypeInfo pci_simple_tst_info = {
	.name = "pci_simple_tst",
	.parent = TYPE_PCI_DEVICE,
	.instance_size = sizeof(PCISimpleTstState),
	.class_init = pci_simple_tst_class_init,
};

static void pci_simple_tst_register_types(void)
{
	type_register_static(&pci_simple_tst_info);
}

type_init(pci_simple_tst_register_types)
