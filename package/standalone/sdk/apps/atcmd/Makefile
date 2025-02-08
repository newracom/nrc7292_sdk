
CSRCS += \
	hif.c \
	hif_fifo.c

ifneq ($(findstring ATCMD_HSPI,$(APP_NAME)), )
CSRCS += \
	hif_hspi.c
endif

ifneq ($(findstring ATCMD_UART,$(APP_NAME)), )
CSRCS += \
	hif_dma.c \
	hif_uart.c
endif

#######################################################################

CSRCS += \
	wifi_api.c \
	lwip_socket.c \
	lwip_ping.c \
	atcmd_main.c \
	atcmd_core.c \
	atcmd_param.c \
	atcmd_basic.c \
	atcmd_wifi.c \
	atcmd_socket.c

ifeq ($(CONFIG_ATCMD_FOTA),y)
CSRCS += \
	atcmd_fota.c
endif

ifeq ($(CONFIG_ATCMD_USER),y)
CSRCS += \
	atcmd_user.c
endif

ifeq ($(CONFIG_ATCMD_INTERNAL),y)
CSRCS += \
	atcmd_internal.c
endif

#######################################################################

$(info )
$(info [ ATCMD Build Configurations ])
$(info ------------------------------)
$(info - CONFIG_FLASH_SIZE=$(CONFIG_FLASH_SIZE))
$(info - CONFIG_FLASH_PROFILE=$(CONFIG_FLASH_PROFILE))
$(info ------------------------------)
$(info - CONFIG_DEBUG=$(CONFIG_DEBUG))
$(info - CONFIG_LOG=$(CONFIG_LOG))
$(info - CONFIG_LOG_MIMINUM=$(CONFIG_LOG_MINIMUM))
$(info - CONFIG_CLI=$(CONFIG_CLI))
$(info - CONFIG_CLI_MINIMUM=$(CONFIG_CLI_MINIMUM))
$(info ------------------------------)
$(info - CONFIG_IPV6=$(CONFIG_IPV6))
$(info - CONFIG_MQTT=$(CONFIG_MQTT))
$(info - CONFIG_COAP=$(CONFIG_COAP))
$(info - CONFIG_CJSON=$(CONFIG_CJSON))
$(info - CONFIG_NVS_FLASH=$(CONFIG_NVS_FLASH))
$(info - CONFIG_HTTP_CLIENT=$(CONFIG_HTTP_CLIENT))
$(info - CONFIG_MANUAL_CONT_TX_SUPPORT=$(CONFIG_MANUAL_CONT_TX_SUPPORT))
$(info ------------------------------)
$(info - CONFIG_ATCMD_CLI=$(CONFIG_ATCMD_CLI))
$(info - CONFIG_ATCMD_IPV6=$(CONFIG_ATCMD_IPV6))
$(info - CONFIG_ATCMD_FAST_RECOVERY=$(CONFIG_ATCMD_FAST_RECOVERY))
$(info ------------------------------)
$(info - CONFIG_ATCMD_FWUPDATE=$(CONFIG_ATCMD_FWUPDATE))
$(info - CONFIG_ATCMD_SFUSER=$(CONFIG_ATCMD_SFUSER))
$(info - CONFIG_ATCMD_SFSYSUSER=$(CONFIG_ATCMD_SFSYSUSER))
$(info ------------------------------)
$(info - CONFIG_ATCMD_WPS=$(CONFIG_ATCMD_WPS))
$(info - CONFIG_ATCMD_BGSCAN=$(CONFIG_ATCMD_BGSCAN))
$(info - CONFIG_ATCMD_RELAY=$(CONFIG_ATCMD_RELAY))
$(info - CONFIG_ATCMD_FOTA=$(CONFIG_ATCMD_FOTA))
$(info - CONFIG_ATCMD_FOTA_HTTPS=$(CONFIG_ATCMD_FOTA_HTTPS))
$(info ------------------------------)
$(info - CONFIG_ATCMD_USER=$(CONFIG_ATCMD_USER))
$(info - CONFIG_ATCMD_INTERNAL=$(CONFIG_ATCMD_INTERNAL))
$(info ------------------------------)
$(info )

