#include "can_pci.h"


static int device_major 	= DEVICE_MAJOR;
struct can_dev *device;

static void send_message(void __iomem *base, struct can_frame *can)
{
	uint8_t temp, index, i;

	index = 10;
	temp = (can->can_id >> 3) & 0xff;
	iowrite8(temp, base + index);
	index++;

	temp = (can->can_id << 5) & 0xe0;
	if (can->can_id & (1 << 30)) // RTR
		temp |= (1 << 4);
	temp |= can->can_dlc & 0x0f;
	iowrite8(temp, base + index);
	index++;

	for(i = 0; i < can->can_dlc; i++, index++)
		iowrite8(can->data[i], base + index);

   	iowrite8(0x01, base + SJA_COMMAND_REG);// Send transmission request.
}


static void receive_message(void __iomem *base, struct can_frame *can)
{
	uint8_t temp, index, i;
	index = TX_FRAME_INFO_REG;

	memset(can, 0, sizeof(struct can_frame));
	index = 20;
	temp = ioread8(base + index++);
	printk(" %02x", temp);
	can->can_id = (temp << 3) & (0xff << 3);
	temp = ioread8(base + index++);
	printk(" %02x", temp);
	can->can_id |= (temp >> 5) & 0x07;
	if(temp & 0x10)
		can->can_id |= (1 << 30);

	can->can_dlc = temp & 0x0f;
	for(i = 0; i < can->can_dlc; i++) {
		can->data[i] = ioread8(base + index++);
		printk(" %02x", can->data[i]);
	}
	printk("\n");
   	iowrite8(0x04, base + SJA_COMMAND_REG);// Release receive buffer.
}


static irqreturn_t irq_handler(int irq, void *dev)
{
	uint8_t temp;

	temp = ioread8(device->mem_base + SJA_INT_REG); //
	if (temp & TI) {
		DPRINTF("IRQ Transmit int\n");
		if (device->tx_count > 0) {
			send_message(device->mem_base, &(device->tx_buf[device->tx_out]));
			device->tx_out++;
			if (device->tx_out == BUFFERSIZE)
				device->tx_out = 0;
			device->tx_count--;
		}
	}

	if (temp & RI) {
		DPRINTF("IRQ Receive int in(%d) out(%d) cnt(%d)\n", 
					device->rx_in, device->rx_out, device->rx_count);		
		if(device->rx_count >= BUFFERSIZE) {
   			iowrite8(0x04, device->mem_base + SJA_COMMAND_REG);// Release receive buffer.
			return IRQ_HANDLED; // No buffer, ignore this one.
		}

		device->rx_count++;
		receive_message(device->mem_base, &(device->rx_buf[device->rx_in]));

		device->rx_in++;
		if (device->rx_in >= BUFFERSIZE)
			device->rx_in = 0;
	}
	
	if (temp & DOI) {
		DPRINTF("IRQ Overrun int\n");
		iowrite8(CDO, device->mem_base + SJA_COMMAND_REG); // Clear data overrun.
	}


	return IRQ_HANDLED;
}


int device_open( struct inode *node, struct file *filp )
{
	uint8_t			temp, count;

	DPRINTF("device open.\n");


	// Try to go to reset mode.
	temp = ioread8(device->mem_base + SJA_MODE_REG); //
	count = 0;
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

	// BasicCAN Mode
	temp = ioread8(device->mem_base + SJA_CLOCK_REG);
	iowrite8(temp & (~PELI), device->mem_base + SJA_CLOCK_REG);

	// Filter set
	iowrite8(0xff, device->mem_base + SJA_BASIC_AMR_REG); // All data

	// Enable transmit and receive interrupt
	temp = ioread8(device->mem_base + SJA_CONTROL_REG);
	iowrite8(temp | (3 << 1), device->mem_base + SJA_CONTROL_REG);

	// Try to go to operation mode.
	temp = ioread8(device->mem_base + SJA_MODE_REG); //
	count = 0;
	while (((temp & RESET_BIT) == RESET_BIT) && (count < 3)) {
		temp &= ~RESET_BIT;
		iowrite8(temp, device->mem_base + SJA_MODE_REG);
		mdelay(10);
		count++;
		temp = ioread8(device->mem_base + SJA_MODE_REG);
	}

	if ((temp & RESET_BIT) == RESET_BIT)
		return -EFAULT;


	memset(device->tx_buf, 0, BUFFERSIZE * sizeof(struct can_frame));
	device->tx_count = 0;
	device->tx_in = 0;
	device->tx_out = 0;
	memset(device->rx_buf, 0, BUFFERSIZE * sizeof(struct can_frame));
	device->rx_count = 0;
	device->rx_in = 0;
	device->rx_out = 0;

	filp->private_data = device;

	return 0;
}

int device_release( struct inode *node, struct file *filp )
{
	struct can_dev *dev = filp->private_data;

	DPRINTF("device release.\n");
	iowrite8(0x01, dev->mem_base + SJA_MODE_REG);
	
	return 0;
}

ssize_t device_read( struct file *filp, char __user *buf, size_t count, loff_t *fpos )
{
	int ret = 0;
	struct can_dev *dev = filp->private_data;

	if (count != sizeof(struct can_frame)) 
		return -EINVAL; // We only accept the sizeof 'struct can_dev'.
	if (dev->rx_count == 0)
		return 0;  // There is no CAN message now.

	if (copy_to_user(buf, &(dev->rx_buf[dev->rx_out]), count)) {
		ret = -EFAULT;
	} else {
		dev->rx_out++;
		if (device->rx_out == BUFFERSIZE)
			device->rx_out = 0;
		dev->rx_count--;
		ret = count;
	}

	return ret;
}

ssize_t device_write( struct file *filp, const char __user *buf, size_t count, loff_t *fpos )
{
	int ret = count;
	unsigned char temp;
	struct can_dev *dev = filp->private_data;

	DPRINTF("write %d fpos %d.\n", count, (int)*fpos);


	if (count != sizeof(struct can_frame)) 
		return -EINVAL; // We only accept the sizeof 'struct can_dev'.
	if (dev->tx_count == BUFFERSIZE)
		return -EFBIG;  // There is no tx buffer to store current message.

	if (copy_from_user(&(dev->tx_buf[dev->tx_in]), buf, count)) {
		ret = -EFAULT;
	}

	temp = ioread8(dev->mem_base + SJA_STATUS_REG); //
	DPRINTF("status register...... %x\n", temp);
	if ((temp & TBS) != TBS) { // Put the message to buffer.
		dev->tx_count++;
		dev->tx_in++;
		if (dev->tx_in == BUFFERSIZE)
			dev->tx_in = 0;
		return count;
	}

	send_message(dev->mem_base, &(dev->tx_buf[dev->tx_in]));

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
	if (!device->mem_base) {
		dev_err(&dev->dev, "pci_iomap FAILED %d", result);
		goto err_iomap; 
  	}

    DPRINTF("interrupt %d\n", dev->irq);
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
