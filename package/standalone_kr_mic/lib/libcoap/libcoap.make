CSRCS += \
	address.c \
	async.c \
	block.c \
	coap_event.c \
	coap_hashkey.c \
	coap_gnutls.c \
	coap_io.c \
	coap_notls.c \
	coap_openssl.c \
	coap_session.c \
	coap_time.c \
	coap_tinydtls.c \
	coap_debug.c \
	encode.c \
	coap_mem.c \
	net.c \
	option.c \
	pdu.c \
	resource.c \
	str.c \
	subscribe.c \
	uri.c

ifeq ($(CONFIG_MBEDTLS), y)
CSRCS += \
	coap_mbedtls.c
endif