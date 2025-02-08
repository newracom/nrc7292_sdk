/* W5500 NAT Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

#include "eth.h"
#include "nrc_sdk.h"

#include "lwip/sys.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/mem.h"
#include "lwip/snmp.h"

#include "netif/ethernet.h"
#include "netif/etharp.h"

#include "nrc_lwip.h"
#include "nrc_eth_if.h"

#include "wifi_config_setup.h"

#include "wifi_service.h"
#include "wifi_connect_common.h"

extern struct netif eth_netif;
extern struct netif* nrc_netif[MAX_IF];

#define NAT 1

void ethernet_start(void)
{
	spi_device_t w5500_spi;
	int gpio_int_pin;
	char value[32];
	size_t length = sizeof(value);

	uint8_t mac[6] = {0x86, 0x25, 0x3f, 0xc7, 0x9b, 0x29};        // TODO: need real mac address
	uint8_t *addr = NULL;
	char *ifconfig_param[2];

#ifdef NRC7394
	gpio_int_pin = GPIO_30;
	w5500_spi.pin_miso = 29;
	w5500_spi.pin_mosi = 6;
	w5500_spi.pin_cs = 28;
	w5500_spi.pin_sclk = 7;
#else
	gpio_int_pin = GPIO_10;
	w5500_spi.pin_miso = 12;
	w5500_spi.pin_mosi = 13;
	w5500_spi.pin_cs = 14;
	w5500_spi.pin_sclk = 15;
#endif
	w5500_spi.frame_bits = SPI_BIT8;
	w5500_spi.clock = 16000000;
	w5500_spi.mode = SPI_MODE0;
	w5500_spi.controller = SPI_CONTROLLER_SPI0;
	w5500_spi.irq_save_flag = 0;
	w5500_spi.isr_handler = NULL;

	addr = mac;

	nrc_eth_set_ethernet_mode(NRC_ETH_MODE_STA);

	nrc_eth_set_network_mode(NRC_NETWORK_MODE_NAT);

	if (ethernet_init(&w5500_spi, addr, gpio_int_pin) != NRC_SUCCESS) {
		nrc_usr_print("[%s] Error initializing ethernet...\n", __func__);
		return;
	}

	if (NAT) {
		strcpy(value, "192.168.2.1");
		nrc_usr_print("Setting eth address to %s...\n", value);
		ifconfig_param[0] = strdup("eth");
		ifconfig_param[1] = strdup(value);
		wifi_ifconfig(2, ifconfig_param);
		free(ifconfig_param[0]);
		free(ifconfig_param[1]);

		// wlan0 address is set during wps_pbc connection

		start_dhcps_on_if(&eth_netif, 120);            // NAT only
	}
}

void user_init(void)
{
	WIFI_CONFIG param;

	ethernet_start();

	nrc_wifi_set_config(&param);
		/* Set Country Code */
	if(nrc_wifi_set_country(0, nrc_wifi_country_from_string((char *)param.country)) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set Country\n", __func__);
		return;
	}

	if (connect_to_ap(&param) == NRC_SUCCESS) {
		nrc_usr_print("Connection to AP successful...\n");
	} else {
		nrc_usr_print("Error connecting to AP...\n");
	}

}
