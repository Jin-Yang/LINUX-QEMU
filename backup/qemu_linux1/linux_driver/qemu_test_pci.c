#include <linux/kernel.h>  
#include <linux/module.h>  
#include <linux/pci.h>  
#include <linux/init.h>  
  

#define PCI_VENDOR_ID_REDHAT   		0x1b36
#define PCI_DEVICE_ID_SIMPLE_TST   	0x1234
#define PCI_REVISION_ID_SIMPLE_TST 	0x73  
  
/* return 0 means success */  
static int pcisimple_probe(struct pci_dev *dev, const struct pci_device_id *id)  
{  
	/* Do probing type stuff here. Like calling request_region(); */ 
	unsigned long flags;
	resource_size_t start, len;
    void __iomem * addressio;
	u8 revision;

    pci_read_config_byte(dev, PCI_REVISION_ID, &revision);
    if (revision != PCI_REVISION_ID_SIMPLE_TST)  
        return 1;  
  
    pci_enable_device(dev);  
  
    start = pci_resource_start(dev, 0);
	len = pci_resource_len(dev, 0);  
    flags = pci_resource_flags(dev, 0);  


    printk("resource length %d\n", len);

    addressio = pci_iomap(dev, 0, len); 
    *(unsigned int *)addressio = 0x12345678;
    printk("write to 0x%p and read back 0x%x\n", addressio, ioread32(addressio));

    iowrite8(0x89, addressio + 8);
    printk("write to 0x%p and read back 0x%x\n", (addressio + 8), ioread8(addressio + 8));
  
    return 0;  
}  
  
static void pcisimple_remove(struct pci_dev *dev)  
{  
    /* clean up any allocated resources and stuff here. 
    * like call release_region(); 
    */  
    pci_disable_device(dev);  
}  


static struct pci_device_id simple_pci_tbl[] = {  
    { PCI_DEVICE(PCI_VENDOR_ID_REDHAT, PCI_DEVICE_ID_SIMPLE_TST), },
    { 0, }  
};

static struct pci_driver pcisimple_driver = {  
    .name 			= "pci_simple_tst",
	.probe			= pcisimple_probe,
	.remove			= __devexit_p(pcisimple_remove),
    .id_table 		= simple_pci_tbl,  
};  


static int __init mem_pci_init(void)  
{  
    return pci_register_driver(&pcisimple_driver);  
}  
  
static void __exit mem_pci_exit(void)  
{  
    pci_unregister_driver(&pcisimple_driver);  
}

module_init(mem_pci_init);
module_exit(mem_pci_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jin-Yang");
MODULE_DESCRIPTION("A Simple Device Driver for QEMU SimpleTest");
MODULE_DEVICE_TABLE(pci, simple_pci_tbl);
