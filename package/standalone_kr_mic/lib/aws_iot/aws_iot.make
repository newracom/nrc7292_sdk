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
