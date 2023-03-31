$(info lib/Makefile CONFIG $(CONFIG))
$(info lib/Makefile ALIAS $(ALIAS))
include $(CONFIG)

SUBDIRS:=$(TOPDIR)/lib/FreeRTOS
SUBDIRS+=$(TOPDIR)/lib/hostap
SUBDIRS+=$(TOPDIR)/lib/lwip
SUBDIRS+=$(TOPDIR)/lib/mbedtls

ifeq ($(CONFIG_NVS_FLASH), y)
	SUBDIRS+=$(TOPDIR)/lib/nvs_flash
endif

ifeq ($(CONFIG_CJSON), y)
	SUBDIRS+=$(TOPDIR)/lib/cJSON
endif

ifeq ($(CONFIG_HTTP_PARSER), y)
	SUBDIRS+=$(TOPDIR)/lib/http_parser
endif

ifeq ($(CONFIG_HTTP_SERVER), y)
	SUBDIRS+=$(TOPDIR)/lib/http_server
endif

ifeq ($(CONFIG_MXML), y)
	SUBDIRS+=$(TOPDIR)/lib/mxml
endif

ifeq ($(CONFIG_AWS), y)
	SUBDIRS+=$(TOPDIR)/lib/aws_iot
endif

ifeq ($(CONFIG_COAP), y)
	SUBDIRS+=$(TOPDIR)/lib/coap
endif

ifeq ($(CONFIG_MQTT), y)
	SUBDIRS+=$(TOPDIR)/lib/paho.mqtt
endif

ifeq ($(CONFIG_ETHERNET), y)
	SUBDIRS+=$(TOPDIR)/lib/ethernet
endif

ifeq ($(CONFIG_ETH_ENC28J60), y)
	SUBDIRS+=$(TOPDIR)/lib/eth_enc28j60
endif

ifeq ($(CONFIG_ETH_W5500), y)
	SUBDIRS+=$(TOPDIR)/lib/eth_w5500
endif

ifeq ($(CONFIG_EPAPER), y)
	SUBDIRS+=$(TOPDIR)/lib/e_paper
endif

ifeq ($(CONFIG_BME680), y)
	SUBDIRS+=$(TOPDIR)/lib/bme680
endif

ifeq ($(CONFIG_SSD1306), y)
	SUBDIRS+=$(TOPDIR)/lib/ssd1306
endif

.PHONY: all $(SUBDIRS)

all: libs

libs: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -j $(JOBS) -C $@

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
