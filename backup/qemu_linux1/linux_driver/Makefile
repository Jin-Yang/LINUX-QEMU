#device-objs := 8250.o 
obj-m+=qemu_test_pci.o

CURRENT:=$(shell pwd)

#KERNEL_PATH:=/lib/modules/$(shell uname -r)/build
KERNEL_PATH:=/home/andy/Projects/GSoC2013/Tips/linux-3.4.48
      
all:  
	make -C $(KERNEL_PATH) M=$(CURRENT) modules  
clean:  
	make -C $(KERNEL_PATH) M=$(CURRENT) clean  
