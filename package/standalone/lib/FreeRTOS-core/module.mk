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
DEFINE += -DNRC_AWS

# Logging level control
#LOG_FLAGS	+= -DENABLE_IOT_DEBUG
#LOG_FLAGS	+= -DENABLE_IOT_TRACE
LOG_FLAGS	+= -DENABLE_IOT_INFO
LOG_FLAGS	+= -DENABLE_IOT_WARN
LOG_FLAGS	+= -DENABLE_IOT_ERROR

DEFINE	+= $(LOG_FLAGS)

# iot client sources
coreMQTT_SRCS += \
	core_mqtt.c \
	core_mqtt_serializer.c \
	core_mqtt_state.c

coreHTTP_SRCS += \
	core_http_client.c

coreJSON_SRCS += \
	core_json.c

#corePKCS11_SRCS += \
#	core_pkcs11.c \
#	core_pki_utils.c \
#	core_pkcs11_mbedtls.c

backoffAlgorithm_SRCS += \
	backoff_algorithm.c

#iot_logging_SRCS += \
#	iot_logging.c \
#	iot_logging_task_dynamic_buffers.c

# platform sources
PLATFORM_SRCS += \
	clock_freertos.c \
	sockets_freertos.c \
	plaintext_freertos.c \
	mbedtls_freertos.c

CSRCS += \
	$(coreMQTT_SRCS) \
	$(coreHTTP_SRCS) \
	$(HTTP_PARSER_SRCS) \
	$(coreJSON_SRCS) \
	$(corePKCS11_SRCS) \
	$(backoffAlgorithm_SRCS) \
	$(iot_logging_SRCS) \
	$(PLATFORM_SRCS)
