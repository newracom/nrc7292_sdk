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

DEFINE	+= -DSUPPORT_EPAPER

#
# port/
#
CSRCS += DEV_Config_nrc.c
CSRCS += EPD_2in66b_nrc.c
CSRCS += EPD_HINK_11in6_nrc.c
CSRCS += GUI_BmpImage.c

#
# src/lib/
#
CSRCS += font12.c font12CN.c font16.c font20.c font24.c font24CN.c font8.c
CSRCS += GUI_Paint.c
