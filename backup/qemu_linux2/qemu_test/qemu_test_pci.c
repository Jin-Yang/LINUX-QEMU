#include <linux/kernel.h>  
#include <linux/module.h>  
#include <linux/pci.h>  
#include <linux/init.h>  
#include <linux/interrupt.h>

#define PCI_VENDOR_ID_REDHAT   		0x1b36
#define PCI_DEVICE_ID_CANBUS		0xbeef
#define PCI_REVISION_ID_CANBUS 		0x73  

#define IO_BAR						0
#define MEM_BAR						1



static irqreturn_t irq_handler(int irq, void *dev)
{
	printk("irq\n");

	return IRQ_HANDLED;
}



/* return 0 means success */  
static int pcisimple_probe(struct pci_dev *dev, const struct pci_device_id *id)  
{  
	/* Do probing type stuff here. Like calling request_region(); */ 
	unsigned long flags;
	int rc;
	resource_size_t start, len;
    void __iomem * addressio;
	u8 revision;

    pci_read_config_byte(dev, PCI_REVISION_ID, &revision);
    if (revision != PCI_REVISION_ID_CANBUS)  
        return 1;
  	
	rc = pci_enable_device(dev);
	pci_save_state(dev);
	if (rc)
		return rc; 
  

    printk("interrupt %d\n", dev->irq);
	if(request_irq(dev->irq, irq_handler, IRQF_SHARED, "mycan", dev)) {
		printk(KERN_ERR "mycan interrupt can't register %d IRQ\n", dev->irq);
		return -EIO;
	}
  
    start = pci_resource_start(dev, IO_BAR);
	len = pci_resource_len(dev, IO_BAR);  
    flags = pci_resource_flags(dev, IO_BAR);  

    printk("resource io length %d ==========\n", len);
    addressio = pci_iomap(dev, IO_BAR, len);
    iowrite8(0x89, addressio);
    printk("write to 0x%p and read back 0x%x\n", addressio, ioread8(addressio));



    start = pci_resource_start(dev, MEM_BAR);
	len = pci_resource_len(dev, MEM_BAR);  
    flags = pci_resource_flags(dev, MEM_BAR);  

    printk("resource mem length %d ==========\n", len);

    addressio = pci_iomap(dev, MEM_BAR, len); 
    *(unsigned int *)addressio = 0x12345678;
    printk("write to 0x%p and read back 0x%x\n", addressio, ioread32(addressio));

    iowrite8(0x89, addressio + 8);
    printk("write to 0x%p and read back 0x%x\n", (addressio + 8), ioread8(addressio + 8));


    return 0;  
}  
  
static void pcisimple_remove(struct pci_dev *dev)  
{  
	free_irq(dev->irq, dev);
    pci_disable_device(dev);  
}  


static struct pci_device_id simple_pci_tbl[] = {  
    { PCI_DEVICE(PCI_VENDOR_ID_REDHAT, PCI_DEVICE_ID_CANBUS), },
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
MODULE_AUTHOR("Andy"); 
MODULE_DESCRIPTION("A Simple Device Driver for QEMU SimpleTest");
MODULE_DEVICE_TABLE(pci, simple_pci_tbl);
