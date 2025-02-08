HTTP_CLIENT_SRC     = $(HTTP_CLIENT_BASE)/src

INCLUDE += -I$(HTTP_CLIENT_BASE)/include

VPATH   += $(HTTP_CLIENT_SRC)

CSRCS += \
	nrc_http_client.c \
	nrc_http_client_debug.c
