include $(CONFIG)

PKG_NAME=http_server

include httpserver.make

HTTP_SERVER_BASE = $(CURDIR)
HTTP_SERVER_SRC  = $(HTTP_SERVER_BASE)/src
HTTP_SERVER_PORT = $(HTTP_SERVER_SRC)/port
HTTP_SERVER_UTIL = $(HTTP_SERVER_SRC)/util

INCLUDE += -I$(HTTP_SERVER_BASE)/include
INCLUDE += -I$(HTTP_SERVER_SRC)/port/nrc7292
INCLUDE += -I$(HTTP_SERVER_UTIL)

# LWIP header
INCLUDE += -I$(TOPDIR)/lib/lwip/port/include
INCLUDE += -I$(TOPDIR)/lib/lwip/lwip/src/include

# http parser headers
INCLUDE += -I$(TOPDIR)/lib/http_parser

VPATH   += $(HTTP_SERVER_SRC)
VPATH   += $(HTTP_SERVER_PORT)
VPATH   += $(HTTP_SERVER_UTIL)

include $(TOPDIR)/make/build-lib.mk
