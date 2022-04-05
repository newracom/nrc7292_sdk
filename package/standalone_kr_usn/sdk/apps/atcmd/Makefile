CSRCS += \
	hif.c \
	hif_fifo.c \
	atcmd_main.c \
	atcmd_core.c \
	atcmd_basic.c \
	atcmd_wifi.c \
	atcmd_socket.c \
	atcmd_fota.c \
	atcmd_param.c \
	lwip_socket.c \
	lwip_ping.c

ifneq ($(findstring ATCMD_HSPI,$(APP_NAME)), )
CSRCS += \
	hif_hspi.c
endif
ifneq ($(findstring ATCMD_UART,$(APP_NAME)), )
CSRCS += \
	hif_dma.c \
	hif_uart.c
endif

ifeq ($(CONFIG_ATCMD_USER),y)
CSRCS += \
	atcmd_user.c
endif

