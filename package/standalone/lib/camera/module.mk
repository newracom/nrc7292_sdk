CAMERA2_BASE = lib/camera
INCLUDE += -I$(CAMERA2_BASE)
INCLUDE += -I$(CAMERA2_BASE)/driver
INCLUDE += -I$(CAMERA2_BASE)/driver/include
INCLUDE += -I$(CAMERA2_BASE)/driver/private_include
INCLUDE += -I$(CAMERA2_BASE)/sensors/private_include

VPATH   += $(CAMERA2_BASE)/driver
VPATH	+= $(CAMERA2_BASE)/sensors

ifeq ($(CONFIG_OV2640), y)
DEFINE += -DCONFIG_OV2640_SUPPORT=1
CSRCS += ov2640.c
endif

ifeq ($(CONFIG_OV5640), y)
DEFINE += -DCONFIG_OV5640_SUPPORT=1
# if CPU clock running on 32MHz
CSRCS += ov5640.c
endif

CSRCS +=  \
	sccb.c \
	sensor.c \
	nrc_camera.c \
	xclk.c
