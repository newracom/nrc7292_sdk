include $(CONFIG)

PKG_NAME=ssd1306

CSRCS += \
	fonts.c\
	oled96.c

SSD1306_BASE = $(CURDIR)

INCLUDE	+= -I$(SSD1306_BASE)
INCLUDE += -I$(TOPDIR)/sdk/include

VPATH	+= $(SSD1306_BASE)

DEFINE += -DSUPPORT_SSD1306

include $(TOPDIR)/make/build-lib.mk
