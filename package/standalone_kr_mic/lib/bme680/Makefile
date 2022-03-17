include $(CONFIG)

PKG_NAME=bme680

CSRCS += \
	bme680.c

BME680_BASE = $(CURDIR)

INCLUDE	+= -I$(BME680_BASE)

VPATH	+= $(BME680_BASE)

DEFINE += -DSUPPORT_BME680

include $(TOPDIR)/make/build-lib.mk
