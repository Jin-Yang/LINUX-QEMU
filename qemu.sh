#! /bin/sh

WORKSPACE=`pwd`
PREFIX=/opt/qemu/bin


# Other options: -vnc :1 == -vnc 127.0.0.1:1 vncviewer 127.0.0.1:5901

# A minimal one.
#$PREFIX/qemu-system-i386 -s -m 128 -kernel bzImage  -hda rootfs.img -append "root=/dev/sda init=/sbin/init"

# Save the output information.
#$PREFIX/qemu-system-i386 -s -m 128 -kernel bzImage -hda rootfs.img -serial file:./linux-start.log -append "root=/dev/sda init=/sbin/init console=ttyS0"

# Start with NFS and output redirect to terminal.
#$PREFIX/qemu-system-i386 -s -m 128 -vnc :1 -kernel bzImage -hda fat:rw:/mnt -serial stdio \
#	-append "root=/dev/nfs nfsroot=192.168.9.33:$WORKSPACE/rootfs rw \ 	
#	ip=192.168.9.88:192.168.9.33:192.168.9.33:255.255.255.0::eth0:auto \
#    init=/sbin/init console=ttyS0" \
#	-net nic,model=e1000,vlan=0,macaddr=00:cd:ef:00:02:01 \
#	-net tap,vlan=0,ifname=tap0,script=$WORKSPACE/ifup-qemu

# Start with NFS normally, start log saved in "linux-start-normal.log".
#$PREFIX/qemu-system-i386 -s -m 128 -kernel bzImage -hda fat:rw:/mnt \
#	-append "notsc clocksource=acpi_pm root=/dev/nfs nfsroot=192.168.9.33:$WORKSPACE/rootfs rw \ 	
#	ip=192.168.9.88:192.168.9.33:192.168.9.33:255.255.255.0::eth0:auto \
#    init=/sbin/init console=ttyS0" \
#	-net nic,model=e1000,vlan=0,macaddr=00:cd:ef:00:02:01 \
#	-net tap,vlan=0,ifname=tap0,script=$WORKSPACE/ifup-qemu \
#	-serial file:./linux-start.log


# pci-serial test.
#$PREFIX/qemu-system-i386 -s -m 128 -kernel bzImage -hda fat:rw:hda \
#	-append "notsc clocksource=acpi_pm root=/dev/nfs nfsroot=192.168.9.33:$WORKSPACE/rootfs rw \ 	
#	ip=192.168.9.88:192.168.9.33:192.168.9.33:255.255.255.0::eth0:auto \
#    init=/sbin/init" \
#	-net nic,model=e1000,vlan=0,macaddr=00:cd:ef:00:02:01 \
#	-net tap,vlan=0,ifname=tap0,script=$WORKSPACE/ifup-qemu \
#	-chardev vc,id=PCIserial \
#	-device pci-can,chardev=PCIserial \



$PREFIX/qemu-system-i386 -s -m 128 -kernel bzImage \
	-append "notsc clocksource=acpi_pm root=/dev/nfs nfsroot=192.168.9.33:$WORKSPACE/rootfs rw \ 	
	ip=192.168.9.88:192.168.9.33:192.168.9.33:255.255.255.0::eth0:auto \
    init=/sbin/init" \
	-net nic,model=e1000,vlan=0,macaddr=00:cd:ef:00:02:01 \
	-net tap,vlan=0,ifname=tap0,script=$WORKSPACE/ifup-qemu \
	-serial none \
	-chardev vc,id=PCIserial \
	-device pci-serial,chardev=PCIserial \
	-chardev can,id=sja1000 \
	-device pci-can,chardev=sja1000



#	-chardev can,id=pci0,port=vcan0 \
#	-device pci-can,chardev=pci0



#	-serial none \
#	-chardev can,id=PCIcan,port=vcan0 \
#	-device pci-can,chardev=PCIcan


#	-device pci-serial,chardev=PCIcan \
# Use QMP(QEMU Monitor Protocol).
#	-qmp unix:/tmp/server.sock,server,nowait \

# For serial device.
#	-chardev serial,id=isa0,path=none -serial vc:800x600 -serial stdio \
#	-serial file:./linux-start.log \
#	-device pci-serial,chardev=isa0

### ISA-Serial OK
#	-chardev serial,id=isa0,path=/dev/ttyS0 \
#	-device isa-serial,chardev=isa0,iobase=0x3f8,irq=4,index=0

#	-device pci-serial
#   -device isa-serial,iobase=IOADDR,irq=IRQ,index=IDX



# http://git.qemu.org/qemu.git  hw/misc/pci-testdev.c
