include $(CONFIG)

PKG_NAME=mbedtls

# mbedtls.make wants CONFIG_LWIP to be set to include
# TLS support
CONFIG_LWIP=y

include mbedtls.make

MBEDTLS_BASE		= $(CURDIR)
MBEDTLS_PORT 		= $(MBEDTLS_BASE)/port
MBEDTLS_PORT_SHA 	= $(MBEDTLS_PORT)/sha
MBEDTLS_LIB			= $(MBEDTLS_BASE)/mbedtls
MBEDTLS_LIB_SRC		= $(MBEDTLS_LIB)/library

MBEDTLS_PORT_INC	= $(MBEDTLS_PORT)/include
MBEDTLS_LIB_INC		= $(MBEDTLS_LIB)/include
MBEDTLS_BASE_INC	= $(MBEDTLS_BASE)/include

INCLUDE	+= -I$(MBEDTLS_PORT_INC)
INCLUDE	+= -I$(MBEDTLS_LIB_INC)
INCLUDE	+= -I$(MBEDTLS_BASE_INC)

# LWIP header
INCLUDE += -I$(TOPDIR)/lib/lwip/port/include
INCLUDE += -I$(TOPDIR)/lib/lwip/lwip/src/include

INCLUDE += -I$(TOPDIR)/lib/hostap/src/utils
INCLUDE += -I$(TOPDIR)/lib/hostap/src/crypto

VPATH	+= $(MBEDTLS_BASE)
VPATH	+= $(MBEDTLS_PORT)
VPATH	+= $(MBEDTLS_PORT_SHA)
VPATH	+= $(MBEDTLS_LIB_SRC)

DEFINE += -DMBEDTLS_CONFIG_FILE=\"config-nrc-basic.h\"
DEFINE += -DNRC_MBEDTLS

# LWIP definition
DEFINE += -DNRC_LWIP

include $(TOPDIR)/make/build-lib.mk
