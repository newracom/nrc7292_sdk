include $(CONFIG)

PKG_NAME=FreeRTOSCore

include module.mk

FREERTOS_CORE_BASE	= $(CURDIR)
coreMQTT_DIR	= $(FREERTOS_CORE_BASE)/libraries/standard/coreMQTT/source
coreHTTP_DIR	= $(FREERTOS_CORE_BASE)/libraries/standard/coreHTTP/source
coreJSON_DIR	= $(FREERTOS_CORE_BASE)/libraries/standard/coreJSON/source
corePKCS11_DIR	= $(FREERTOS_CORE_BASE)/libraries/standard/corePKCS11/source
backoffAlgorithm_DIR	= $(FREERTOS_CORE_BASE)/libraries/standard/backoffAlgorithm/source
PLATFORM_DIR	= $(FREERTOS_CORE_BASE)/platform

INCLUDE	+= -I$(coreMQTT_DIR)/include
INCLUDE	+= -I$(coreMQTT_DIR)/interface
INCLUDE	+= -I$(coreHTTP_DIR)/include
INCLUDE	+= -I$(coreJSON_DIR)/include
INCLUDE	+= -I$(backoffAlgorithm_DIR)/include
INCLUDE	+= -I$(PLATFORM_DIR)/include
INCLUDE	+= -I$(PLATFORM_DIR)/freertos
INCLUDE	+= -I$(PLATFORM_DIR)/freertos/transport/include
INCLUDE	+= -I$(PLATFORM_DIR)/freertos/include

# LWIP headers
INCLUDE += -I$(TOPDIR)/lib/lwip/port/include
INCLUDE += -I$(TOPDIR)/lib/lwip/lwip/src/include

# http parser headers
INCLUDE += -I$(TOPDIR)/lib/http_parser

# mbedtls headers
INCLUDE += -I$(TOPDIR)/lib/mbedtls/mbedtls/include
INCLUDE += -I$(TOPDIR)/lib/mbedtls/port/include

# api_uart.h
INCLUDE += -I$(TOPDIR)/sdk/include

VPATH	+= $(coreMQTT_DIR)
VPATH	+= $(coreHTTP_DIR)
VPATH	+= $(coreJSON_DIR)
VPATH	+= $(corePKCS11_DIR)
VPATH	+= $(corePKCS11_DIR)/portable/mbedtls
VPATH	+= $(backoffAlgorithm_DIR)
VPATH	+= $(PLATFORM_DIR)/freertos
VPATH	+= $(PLATFORM_DIR)/freertos/transport/src

DEFINE += -DMQTT_DO_NOT_USE_CUSTOM_CONFIG
DEFINE += -DHTTP_DO_NOT_USE_CUSTOM_CONFIG
DEFINE += -DNRC_FREERTOS_CORE
DEFINE += -DNRC_MBEDTLS
DEFINE += -DMBEDTLS_CONFIG_FILE=\"config-nrc-basic.h\"

include $(TOPDIR)/make/build-lib.mk
