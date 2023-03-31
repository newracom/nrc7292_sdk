include $(CONFIG)

PKG_NAME=nvs_flash

include nvs_flash.make

NVS_BASE = $(CURDIR)

INCLUDE	+= -I$(NVS_BASE)/src
INCLUDE	+= -I$(NVS_BASE)/include
INCLUDE	+= -I$(TOPDIR)/lib/http_server/include
INCLUDE += -I$(TOPDIR)/sdk/include

VPATH	+= $(NVS_BASE)/src

DEFINE += -DSUPPORT_NVS_FLASH

include $(TOPDIR)/make/build-lib.mk
