include $(CONFIG)

PKG_NAME=eth

include ethernet.make

ETHERNET_BASE	= $(CURDIR)

INCLUDE	+= -I$(ETHERNET_BASE)

INCLUDE += -I$(TOPDIR)/sdk/include

VPATH	+= $(ETHERNET_BASE)

include $(TOPDIR)/make/build-lib.mk
