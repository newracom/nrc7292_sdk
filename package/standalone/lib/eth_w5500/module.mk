INCLUDE += -I$(ETH_W5500_BASE)
VPATH   += $(ETH_W5500_BASE)
DEFINE	+= -DETH_DRIVER_W5500

CSRCS += \
	eth_mac_w5500.c \
	eth_phy_w5500.c

ifeq ($(CONFIG_SPI_DMA), y)
CSRCS +=  \
	nrc_spi_dma.c \
	nrc_dma.c

DEFINE += -DSUPPORT_ETHERNET_SPI_DMA
endif
