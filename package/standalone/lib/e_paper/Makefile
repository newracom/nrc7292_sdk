include $(CONFIG)

PKG_NAME=epaper

include e_paper.make

EPAPER_BASE	= $(CURDIR)

INCLUDE += -I$(EPAPER_BASE)/port
INCLUDE += -I$(EPAPER_BASE)/port/Config
INCLUDE += -I$(EPAPER_BASE)/port/e-Paper
INCLUDE += -I$(EPAPER_BASE)/port/GUI
INCLUDE += -I$(EPAPER_BASE)/src/lib/Fonts 
INCLUDE += -I$(EPAPER_BASE)/src/lib/GUI

VPATH   += $(EPAPER_BASE)/port/Config
VPATH   += $(EPAPER_BASE)/port/e-Paper
VPATH   += $(EPAPER_BASE)/port/GUI
VPATH   += $(EPAPER_BASE)/src/lib/Fonts
VPATH   += $(EPAPER_BASE)/src/lib/GUI

DEFINE += -DSUPPORT_EPAPER

include $(TOPDIR)/make/build-lib.mk
