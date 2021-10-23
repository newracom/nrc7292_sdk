TARGET	:= 	built-in.o
SUBDIRS	:=
CURRDIR := $(NRCDIR)/lib/hostap/src/drivers
WPASDIR := $(NRCDIR)/lib/hostap

##################################################################
TARGET	:= 	built-in.o
INCLUDE += -I$(CURRDIR) -I$(WPASDIR)/src/drivers -I$(WPASDIR)/src -I$(WPASDIR)/src/utils
SUBDIRS	:=
SRC_MOD := \
			test/wpas_scan_test.c \
			driver_nrc_scan.c
#################################################################

#$(info [orig] [$(SRC_MOD)])
OBJS 	:= $(patsubst %.c,$(OBJSDIR)/%.o,$(SRC_MOD))

include $(TOPDIR)/Config.mk
all: compile $(OBJS)
include $(TOPDIR)/Rules.mk
