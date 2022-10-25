CC := gcc
AR := ar
LIB_TARGET = wpa_unittest.a
WPA_PATH := $(abspath ../../..)
WPA_UTILS := $(WPA_PATH)/src/utils
WPA_COMMON := $(WPA_PATH)/src/common

LIB_CFLAGS := -Wall -g

LIB_INCLUDE = \
	-I$(WPA_PATH) \
	-I$(WPA_PATH)/src \
	-I$(WPA_UTILS) \
	-I$(WPA_COMMON)

VPATH = $(WPA_UTILS) $(WPA_COMMON)
		
LIB_OBJS = \
	base64.o \
	bitfield.o \
	common.o \
	ip_addr.o \
	radiotap.o \
	trace.o \
	uuid.o \
	wpa_debug.o \
	wpabuf.o \
	os_unix.o \
	ieee802_11_common.o

$(warning $(LIB_OBJS))

.PHONY: all

all: $(LIB_TARGET)

clean:
	rm -f *~ *.o *.d *.a

$(LIB_TARGET): $(LIB_OBJS)
	$(AR) crT $@ $(notdir $?)

%.o: %.c
	@echo compile $(notdir $<)
	@$(CC) $(LIB_CFLAGS) $(LIB_INCLUDE) -c $< -o $(notdir $@)
