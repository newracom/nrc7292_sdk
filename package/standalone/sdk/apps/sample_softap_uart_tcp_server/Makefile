DEFINE += -DCONFIG_ATCMD_UART
DEFINE += -DWIFI_CONFIG_NVS_UPDATE=1

CSRCS += \
	softap_uart_tcp_server.c \
	hif.c \
	hif_dma.c \
	hif_fifo.c \
	hif_uart.c \
	nrc_tcp_server.c \
	nrc_ctrl_cmds.c \
	nrc_ctrl_func.c \
	tlv.c \
	nrc_fota.c

include $(SDK_WIFI_COMMON)/module.mk
