CSRCS += \
	hif.c \
	hif_fifo.c \
	atcmd_main.c \
	atcmd_core.c \
	atcmd_basic.c \
	atcmd_wifi.c \
	atcmd_param.c

ifneq ($(findstring ATCMD_HSPI,$(APP_NAME)), )
CSRCS += \
	hif_hspi.c
endif
ifneq ($(findstring ATCMD_UART,$(APP_NAME)), )
CSRCS += \
	hif_dma.c \
	hif_uart.c
endif

ifeq ($(CONFIG_ATCMD_CLI),y)
CSRCS += \
	atcmd_cli.c
endif

ifeq ($(CONFIG_ATCMD_HOST),y)
CSRCS += \
	lwip_dummy.c \
	atcmd_host.c
else
CSRCS += \
	lwip_ping.c \
	lwip_socket.c \
	atcmd_fota.c \
	atcmd_socket.c
endif

ifeq ($(CONFIG_ATCMD_USER),y)
CSRCS += \
	atcmd_user.c
endif

ifeq ($(CONFIG_ATCMD_TEST),y)
CSRCS += \
	atcmd_test.c
endif

