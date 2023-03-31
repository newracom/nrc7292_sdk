include $(CONFIG)

PKG_NAME=sht30

CSRCS += \
	sht30.c

SHT30_BASE = $(CURDIR)

INCLUDE	+= -I$(SHT30_BASE)

VPATH	+= $(SHT30_BASE)

DEFINE += -DSHT30_LOOKUP_TABLE

include $(TOPDIR)/make/build-lib.mk