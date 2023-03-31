include $(CONFIG)

PKG_NAME=http_parser

include httpparser.make

HTTP_PARSER_BASE = $(CURDIR)

INCLUDE	+= -I$(HTTP_PARSER_BASE)

VPATH	+= $(HTTP_PARSER_BASE)

include $(TOPDIR)/make/build-lib.mk
