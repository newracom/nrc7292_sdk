include $(CONFIG)

PKG_NAME = FreeRTOS

INCLUDE += -ISource/include
INCLUDE += -IConfig/nrc7292
INCLUDE += -ISource/portable/GCC/ARM_CM3

VPATH=${CURDIR}/Source
VPATH+=Source/portable/GCC/ARM_CM3
VPATH+=Source/portable/MemMang

CSRCS := tasks.c
CSRCS += list.c
CSRCS += queue.c
CSRCS += timers.c
CSRCS += event_groups.c
CSRCS += port.c
CSRCS += heap_4.c

include $(TOPDIR)/make/build-lib.mk
