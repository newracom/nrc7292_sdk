include $(CONFIG)

PKG_NAME=sgp30

CSRCS += \
	sgp30.c

SGP30_BASE = $(CURDIR)

INCLUDE	+= -I$(SGP30_BASE)

VPATH	+= $(SGP30_BASE)

DEFINE += -DSGP30_LOOKUP_TABLE

include $(TOPDIR)/make/build-lib.mk
