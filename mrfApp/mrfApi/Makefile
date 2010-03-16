
CFLAGS += -fPIC
CC=gcc
OBJECTS = egapi.o irqtest.o irq_monitor.o factest.o
HEADERS = egapi.h erapi.h
PKG_DIR = pci_mrfapi
PKG_FILES = $(PKG_DIR)/COPYING \
        $(PKG_DIR)/egapi.c \
        $(PKG_DIR)/egapi.h \
        $(PKG_DIR)/egcpci.h \
        $(PKG_DIR)/erapi.c \
        $(PKG_DIR)/erapi.h \
        $(PKG_DIR)/evan_monitor.c \
        $(PKG_DIR)/evrpresc.c \
        $(PKG_DIR)/irq_monitor.c \
        $(PKG_DIR)/irqtest.c \
        $(PKG_DIR)/Makefile \
        $(PKG_DIR)/mmap_test.c \
        $(PKG_DIR)/factest.c \
        $(PKG_DIR)/fwdump.c

PKG_DATE = $(shell date +"%y%m%d")
PKG_NAME = pci_mrfapi.$(PKG_DATE).tar.gz

all: evan_monitor evrpresc mmap_test irqtest irq_monitor factest

package: $(PKG_NAME)

$(PKG_NAME):
	cd ..; tar zcvf $@ $(PKG_FILES)

egapi.o: egapi.c egapi.h
	$(CC) $(CFLAGS) -c egapi.c

erapi.o: erapi.c erapi.h
	$(CC) $(CFLAGS) -c erapi.c

evan_monitor.o: evan_monitor.c egapi.h
	$(CC) $(CFLAGS) -c evan_monitor.c

evan_monitor: evan_monitor.o egapi.o
	$(CC) $(LDFLAGS) -o $@ evan_monitor.o egapi.o

evrpresc.o: evrpresc.c erapi.h
	$(CC) $(CFLAGS) -c evrpresc.c

evrpresc: evrpresc.o erapi.o
	$(CC) $(LDFLAGS) -o $@ evrpresc.o erapi.o

mmap_test: mmap_test.c
	$(CC) -o $@ $<

irqtest.o: irqtest.c egapi.h erapi.h
	$(CC) $(CFLAGS) -c irqtest.c

irqtest: irqtest.o egapi.o erapi.o
	$(CC) $(LDFLAGS) -o $@ irqtest.o egapi.o erapi.o

irq_monitor.o: irq_monitor.c egapi.h erapi.h
	$(CC) $(CFLAGS) -c irq_monitor.c

irq_monitor: irq_monitor.o egapi.o erapi.o
	$(CC) $(LDFLAGS) -o $@ irq_monitor.o egapi.o erapi.o

factest.o: factest.c egapi.h erapi.h
	$(CC) $(CFLAGS) -c factest.c

factest: factest.o egapi.o erapi.o
	$(CC) $(LDFLAGS) -o $@ factest.o egapi.o erapi.o

clean:
	rm -rf *.o *~ evan_monitor evrpresc mmap_test irqtest irq_monitor factest
