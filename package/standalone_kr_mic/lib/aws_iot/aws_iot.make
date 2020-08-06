# iot client sources
IOT_CLIENT_SRCS += \
    aws_iot_jobs_interface.c \
    aws_iot_jobs_json.c \
    aws_iot_jobs_topics.c \
    aws_iot_jobs_types.c \
    aws_iot_json_utils.c \
    aws_iot_mqtt_client.c \
    aws_iot_mqtt_client_common_internal.c \
    aws_iot_mqtt_client_connect.c \
    aws_iot_mqtt_client_publish.c \
    aws_iot_mqtt_client_subscribe.c \
    aws_iot_mqtt_client_unsubscribe.c \
    aws_iot_mqtt_client_yield.c \
    aws_iot_shadow.c \
    aws_iot_shadow_actions.c \
    aws_iot_shadow_json.c \
    aws_iot_shadow_records.c

# iot jsmn sources
IOT_CLIENT_JSMN_SRCS += jsmn.c

# platform sources
PLATFORM_SRCS +=\
	aws_iot_timer.c \
	network_mbedtls_wrapper.c

CSRCS += \
	$(PLATFORM_SRCS) \
	$(IOT_CLIENT_SRCS) \
	$(IOT_CLIENT_JSMN_SRCS)
