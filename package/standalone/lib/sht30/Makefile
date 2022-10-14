include $(CONFIG)

PKG_NAME=sht30

CSRCS += \
	sht30.c

SHT30_BASE = $(CURDIR)

INCLUDE	+= -I$(SHT30_BASE)

VPATH	+= $(SHT30_BASE)

DEFINE += -DSUPPORT_SHT30

include $(TOPDIR)/make/build-lib.mk
