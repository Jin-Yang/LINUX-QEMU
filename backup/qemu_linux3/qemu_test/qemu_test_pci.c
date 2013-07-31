#include "can_pci.h"



static int device_major 	= DEVICE_MAJOR;
struct can_dev *device;



static irqreturn_t irq_handler(int irq, void *dev)
{
	printk("irq\n");

	return IRQ_HANDLED;
}


int device_open( struct inode *node, struct file *filp )
{
	DPRINTF("device open.\n");

	memset(device->mem, 0, sizeof(struct can_frame) * BUFFERSIZE);
	filp->private_data = device;
	
	return 0;
}

int device_release( struct inode *node, struct file *filp )
{
	DPRINTF("device release.\n");

	return 0;
}

ssize_t device_read( struct file *filp, char __user *buf, size_t count, loff_t *fpos )
{
	int ret = 0;
	struct can_dev *dev = filp->private_data;

//	if (count > dev->valid - *fpos)
//		count = dev->valid - *fpos;
	
	DPRINTF("read %d bytes actually, fops %d.\n", count, (int)*fpos);

	if (copy_to_user(buf, (void *)(dev->mem + *fpos), count)) {
		ret = -EFAULT;
	} else {
//		*fpos += count;
		ret = count;
	}

	return ret;
}

ssize_t device_write( struct file *filp, const char __user *buf, size_t count, loff_t *fpos )
{
	int ret = 0, i;
	struct can_dev *dev = filp->private_data;

	DPRINTF("write %d fpos %d.\n", count, (int)*fpos);

//	if (count > BUFFERSIZE - *fpos)
//		count = BUFFERSIZE - *fpos;

	if (copy_from_user(dev->mem + *fpos, buf, count)) {
		ret = -EFAULT;
	} else {
//		*fpos += count;
		ret = count;
	}
//	dev->valid += ret;

	DPRINTF("count %d\n", count);
	for( i = 0; i < count; i++) {
		DPRINTF("%d : 0x%x\n", i, 0xff & *((char *)dev->mem + i));
    	iowrite8(0xff & *((char *)dev->mem + i), dev->mem_base + i);
	}



   	iowrite8(0x55, dev->mem_base + 30);

	return ret;
}

struct file_operations device_fops = {
	.owner =    THIS_MODULE,
	.read =     device_read,
	.write =    device_write,
	.open =     device_open,
	.release =  device_release,
};

/* return 0 means success */  
static int pcisimple_probe(struct pci_dev *dev, const struct pci_device_id *id)  
{  
	/* Do probing type stuff here. Like calling request_region(); */ 
	int result = 0;
	dev_t devid;
	u8 revision;

	DPRINTF("device init.\n");

	device = kmalloc(sizeof(struct can_dev), GFP_KERNEL);
	if (!device) {
		return -ENOMEM;
	}
	memset(device, 0, sizeof(struct can_dev));

    pci_read_config_byte(dev, PCI_REVISION_ID, &revision);
    if (revision != PCI_REVISION_ID_CANBUS)  
        return -ENODEV;
  	
	result = pci_enable_device(dev);
	if (result) {
		dev_err(&dev->dev, "%s: pci_enable_device FAILED", __func__);
		goto err_enable_device; 
  	}

	result = pci_request_regions(dev, "can_pci");
	if (result) {
		dev_err(&dev->dev, "pci_request_regions FAILED %d", result);
		goto err_request_regions; 
  	}


    device->mem_base = pci_iomap(dev, MEM_BAR, 0);
    device->io_base = pci_iomap(dev, IO_BAR, 0);
	if (!device->mem_base || !device->io_base) {
		dev_err(&dev->dev, "pci_iomap FAILED %d", result);
		goto err_iomap; 
  	}

    printk("interrupt %d\n", dev->irq);
	if(request_irq(dev->irq, irq_handler, IRQF_SHARED, "mycan", dev)) {
		dev_err(&dev->dev, "requrest_irq FAILED %d", dev->irq);
		result = -EIO;
		goto err_request_irq;
	}


	/*
	 * Get a range of minor numbers to work with, asking for a dynamic
	 * major unless directed otherwise at load time.
	 */
	devid = MKDEV(device_major, 0);
	result = register_chrdev_region(devid, 1, "can");
	if (result < 0) {
		DPRINTF("can't get major %d\n", device_major);
		goto err_register_chrdev_region;
	}

	cdev_init(&device->cdev, &device_fops);
	device->cdev.owner = THIS_MODULE;
	result = cdev_add(&device->cdev, devid, 1);
	if (result) {
		DPRINTF("Error %d while add cdev", result);
		goto err_cdev_add;
	}
	return 0;

err_cdev_add:
	unregister_chrdev_region(devid, 1);
	return result;

err_register_chrdev_region:	
	free_irq(dev->irq, dev);

err_request_irq:
	pci_iounmap(dev, device->io_base);
	pci_iounmap(dev, device->mem_base);

err_iomap:
	pci_release_regions(dev);

err_request_regions:
	pci_disable_device(dev);

err_enable_device:
	kfree(device);
	return result;
}  
  
static void pcisimple_remove(struct pci_dev *dev)  
{  
	DPRINTF("device exit.\n");

	cdev_del(&device->cdev);
	unregister_chrdev_region(MKDEV(device_major, 0), 1);
	free_irq(dev->irq, dev);
	pci_iounmap(dev, device->io_base);
	pci_iounmap(dev, device->mem_base);
	pci_release_regions(dev);
	pci_disable_device(dev);
	kfree(device);
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
