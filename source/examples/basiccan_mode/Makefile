
DESTDIR = /home/andy/Projects/GSoC2013/Linux/rootfs/home/can_pci/basic

CFLAGS    = -O2 -Wall 


PROGRAMS = send receive

all: $(PROGRAMS)

clean:
	rm -f $(PROGRAMS) *.o

install:
	cp -f $(PROGRAMS) $(DESTDIR)/

distclean:
	rm -f $(PROGRAMS) *.o *~


