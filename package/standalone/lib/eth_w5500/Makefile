include $(CONFIG)

PKG_NAME=w5500

include eth_w5500.make

ETH_W5500_BASE	= $(CURDIR)

INCLUDE	+= -I$(ETH_W5500_BASE)

INCLUDE += -I$(TOPDIR)/lib/ethernet
INCLUDE += -I$(TOPDIR)/sdk/include

VPATH	+= $(ETH_W5500_BASE)

include $(TOPDIR)/make/build-lib.mk
