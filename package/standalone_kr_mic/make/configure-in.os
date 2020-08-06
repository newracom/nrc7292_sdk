#// --------------------------------------------------
#// LIB
#// FreeRTOS
#// --------------------------------------------------
FreeRTOS_BASE	= $(LIB_PATH)/FreeRTOS
FreeRTOS_CONFIG_PATH	= $(FreeRTOS_BASE)/Config
FreeRTOS_SOURCE_PATH	= $(FreeRTOS_BASE)/Source
FreeRTOS_SYSTEM_PATH	= $(FreeRTOS_CONFIG_PATH)/$(SYSTEM)
FreeRTOS_PORTABLE_PATH	= $(FreeRTOS_SOURCE_PATH)/portable
FreeRTOS_INCLUDE_PATH	= $(FreeRTOS_SOURCE_PATH)/include
INCLUDE	+= -I$(FreeRTOS_INCLUDE_PATH)
INCLUDE	+= -I$(FreeRTOS_SYSTEM_PATH)
INCLUDE	+= -I$(FreeRTOS_SOURCE_PATH)
INCLUDE	+= -I$(FreeRTOS_PORTABLE_PATH)/GCC/$(CPU)
INCLUDE	+= -I$(FreeRTOS_PORTABLE_PATH)/MemMang
VPATH	+= $(FreeRTOS_INCLUDE_PATH)
VPATH	+= $(FreeRTOS_SYSTEM_PATH)
VPATH	+= $(FreeRTOS_SOURCE_PATH)
VPATH	+= $(FreeRTOS_PORTABLE_PATH)/GCC/$(CPU)
VPATH	+= $(FreeRTOS_PORTABLE_PATH)/MemMang

ifeq ($(CONFIG_FREERTOS), y)
SRCS_OS	 := tasks.c
SRCS_OS	+= list.c
SRCS_OS	+= queue.c
SRCS_OS	+= port.c
SRCS_OS	+= heap_4.c
SRCS_OS	+= timers.c
SRCS_OS	+= event_groups.c
CSRCS	+= $(SRCS_OS)
endif #CONFIG_FREERTOS