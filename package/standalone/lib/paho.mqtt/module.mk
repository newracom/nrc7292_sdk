MQTT_PKT_SRCS	= $(MQTT_BASE)/MQTTPacket/src
MQTT_CNT_SRCS	= $(MQTT_BASE)/MQTTClient-C/src

INCLUDE	+= -I$(MQTT_PKT_SRCS)
INCLUDE	+= -I$(MQTT_CNT_SRCS)

VPATH	+= $(MQTT_PKT_SRCS)
VPATH	+= $(MQTT_CNT_SRCS)

DEFINE	+= -DMQTT_TASK

CSRCS		+= \
	MQTTConnectClient.c \
	MQTTConnectServer.c \
	MQTTDeserializePublish.c \
	MQTTFormat.c \
	MQTTPacket.c \
	MQTTSerializePublish.c \
	MQTTSubscribeClient.c \
	MQTTSubscribeServer.c \
	MQTTUnsubscribeClient.c \
	MQTTUnsubscribeServer.c \
	MQTTNrcImpl.c \
	MQTTClient.c
