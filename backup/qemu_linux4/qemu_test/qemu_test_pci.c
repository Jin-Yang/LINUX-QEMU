#include "can_pci.h"



static int device_major 	= DEVICE_MAJOR;
struct can_dev *device;



static irqreturn_t irq_handler(int irq, void *dev)
{
	DPRINTF("irq\n");

   	iowrite8(0x33, device->mem_base + 127); // Clear the interrupt.

	return IRQ_HANDLED;
}


int device_open( struct inode *node, struct file *filp )
{
	uint8_t			temp, count = 0;

	DPRINTF("device open.\n");

	memset(device->tx_buf, 0, sizeof(struct can_frame) * BUFFERSIZE);
	filp->private_data = device;

	// Try to go to reset mode.
	temp = ioread8(device->mem_base + SJA_MODE_REG); //
	while (((temp & RESET_BIT) != RESET_BIT) && (count < 3)) {
		temp |= RESET_BIT;
		iowrite8(temp, device->mem_base + SJA_MODE_REG);
		mdelay(10);
		count++;
		temp = ioread8(device->mem_base + SJA_MODE_REG);
	}
	if ((temp & RESET_BIT) != RESET_BIT)
		return -EFAULT;
	
	/* Do some initialising thing */


	// Try to go to operation mode.
	temp = ioread8(device->mem_base + SJA_MODE_REG); //
	while (((temp & RESET_BIT) == RESET_BIT) && (count < 3)) {
		temp &= ~RESET_BIT;
		iowrite8(temp, device->mem_base + SJA_MODE_REG);
		mdelay(10);
		count++;
		temp = ioread8(device->mem_base + SJA_MODE_REG);
	}
	if ((temp & RESET_BIT) == RESET_BIT)
		return -EFAULT;

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

/*	if (copy_to_user(buf, (void *)(dev->mem + *fpos), count)) {
		ret = -EFAULT;
	} else {
//		*fpos += count;
		ret = count;
	}*/

	return ret;
}

ssize_t device_write( struct file *filp, const char __user *buf, size_t count, loff_t *fpos )
{
	int ret = 0, i;
	unsigned char temp, index;
	struct can_dev *dev = filp->private_data;

	DPRINTF("write %d fpos %d.\n", count, (int)*fpos);

	if (count > BUFFERSIZE * sizeof(struct can_dev) - dev->tx_count)
		return -EFBIG;  // There is no tx buffer to store current message.
	if (count != sizeof(struct can_frame)) 
		return -EINVAL; // We only accept the 'struct can_dev'.

	if (copy_from_user(&(dev->tx_buf[dev->tx_in]), buf, count)) {
		ret = -EFAULT;
	}

	temp = ioread8(dev->mem_base + SJA_STATUS_REG); //
	DPRINTF("status register...... %x\n", temp);
	if ((temp & TBS) != TBS) { // Put the message to buffer.
		dev->tx_count += sizeof(struct can_dev);
		dev->tx_in++;
		if (dev->tx_in == BUFFERSIZE)
			dev->tx_in = 0;
		return count;
	}

	temp = 0; index = TX_FRAME_INFO_REG;
	temp |= DLC(dev->tx_buf[dev->tx_in].can_dlc);
	if (dev->tx_buf[dev->tx_in].can_id & (1 << 30)) // RTR
		temp |= RTR;
	if (dev->tx_buf[dev->tx_in].can_id & (1 << 31)) { // EFF
		temp |= EFF;
		iowrite8(temp, dev->mem_base + index); // Write to TX frame information.
		index++;
		
		temp = (dev->tx_buf[dev->tx_in].can_id >> 21) & 0xff;
		iowrite8(temp, dev->mem_base + index);
		index++;
		temp = (dev->tx_buf[dev->tx_in].can_id >> 13) & 0xff;
		iowrite8(temp, dev->mem_base + index);
		index++;
		temp = (dev->tx_buf[dev->tx_in].can_id >> 5) & 0xff;
		iowrite8(temp, dev->mem_base + index);
		index++;
		temp = (dev->tx_buf[dev->tx_in].can_id & 0x1f) << 0x03;
		iowrite8(temp, dev->mem_base + index);
		index++;

		for(i = 0; i < dev->tx_buf[dev->tx_in].can_dlc; i++, index++)
			iowrite8(dev->tx_buf[dev->tx_in].data[i], dev->mem_base + index);
	} else {
		iowrite8(temp, dev->mem_base + index); // Write to TX frame information.
		index++;

		temp = (dev->tx_buf[dev->tx_in].can_id >> 3) & 0xff;
		iowrite8(temp, dev->mem_base + index);
		index++;
		
		temp = (dev->tx_buf[dev->tx_in].can_id & 0x03) << 0x05;
		iowrite8(temp, dev->mem_base + index);
		index++;
		
		for(i = 0; i < dev->tx_buf[dev->tx_in].can_dlc; i++, index++)
			iowrite8(dev->tx_buf[dev->tx_in].data[i], dev->mem_base + index);
	}
	
   	iowrite8(0x01, dev->mem_base + SJA_COMMAND_REG);// Send transmission request.

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
