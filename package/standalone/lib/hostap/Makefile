include $(CONFIG)

PKG_NAME=hostap

WPA_SUPP_ROOT=$(CURDIR)
WPA_SUPP_PATH	= $(shell find $(WPA_SUPP_ROOT) -type d \
	-not -path $(WPA_SUPP_ROOT)/hostapd -not -path $(WPA_SUPP_ROOT)/wlantest)

INCLUDE	+= $(addprefix -I,$(WPA_SUPP_PATH))

# LWIP headers
INCLUDE += -I$(TOPDIR)/lib/lwip/port/include
INCLUDE += -I$(TOPDIR)/lib/lwip/lwip/src/include

# MBEDTLS headers
INCLUDE += -I$(TOPDIR)/lib/mbedtls/mbedtls/include
INCLUDE += -I$(TOPDIR)/lib/mbedtls/port/include

VPATH += $(WPA_SUPP_ROOT)/src/utils
VPATH += $(WPA_SUPP_ROOT)/src/common
VPATH += $(WPA_SUPP_ROOT)/src/rsn_supp
VPATH += $(WPA_SUPP_ROOT)/src/crypto
VPATH += $(WPA_SUPP_ROOT)/src/l2_packet
VPATH += $(WPA_SUPP_ROOT)/src/wps
VPATH += $(WPA_SUPP_ROOT)/src/eapol_supp
VPATH += $(WPA_SUPP_ROOT)/src/eap_peer
VPATH += $(WPA_SUPP_ROOT)/src/eapol_auth
VPATH += $(WPA_SUPP_ROOT)/src/eap_server
VPATH += $(WPA_SUPP_ROOT)/src/eap_common
VPATH += $(WPA_SUPP_ROOT)/src/drivers
VPATH += $(WPA_SUPP_ROOT)/src/ap
VPATH += $(WPA_SUPP_ROOT)/wpa_supplicant

CONFIG_WPA_SUPP_CRYPTO=mbedtls
CONFIG_SOFT_AP=y

include wpa_supplicant/FreeRTOS.config
include wpa_supplicant/FreeRTOS.make

CSRCS = $(notdir $(WPA_SUPP_CSRCS))
SRCS = $(notdir $(WPA_SUPP_CPPSRCS))

MAC_ADDR_SEED	= $(shell git config user.name | sed -e 's/ /_/g')
DEFINE += -DINCLUDE_WPA_SUPP -DMAC_ADDR_SEED=\"$(MAC_ADDR_SEED)\"

# MBEDTLS definitions
DEFINE	+= -DSUPPORT_MBEDTLS
DEFINE	+= -DMBEDTLS_CONFIG_FILE=\"config-nrc-basic.h\"

include $(TOPDIR)/make/build-lib.mk
