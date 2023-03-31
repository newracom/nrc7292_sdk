include $(CONFIG)

PKG_NAME=pahomqtt

include mqtt.make

MQTT_BASE		= $(CURDIR)
MQTT_PKT_SRCS	= $(MQTT_BASE)/MQTTPacket/src
MQTT_CNT_SRCS	= $(MQTT_BASE)/MQTTClient-C/src

INCLUDE	+= -I$(MQTT_PKT_SRCS)
INCLUDE	+= -I$(MQTT_CNT_SRCS)

# LWIP header
INCLUDE += -I$(TOPDIR)/lib/lwip/port/include
INCLUDE += -I$(TOPDIR)/lib/lwip/lwip/src/include

# MBEDTLS headers
INCLUDE += -I$(TOPDIR)/lib/mbedtls/mbedtls/include
INCLUDE += -I$(TOPDIR)/lib/mbedtls/port/include

INCLUDE += -I$(TOPDIR)/lib/ssd1306

VPATH	+= $(MQTT_PKT_SRCS)
VPATH	+= $(MQTT_CNT_SRCS)

DEFINE	+= -DMQTT_TASK
DEFINE	+= -DSUPPORT_MBEDTLS

DEFINE += -DMBEDTLS_CONFIG_FILE=\"config-nrc-basic.h\"

include $(TOPDIR)/make/build-lib.mk
