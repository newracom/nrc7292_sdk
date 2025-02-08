/* W5500 Bridge mode Example

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

#include "web_service.h"
#include "wifi_service.h"
#include "wifi_connect_common.h"
#include "api_system.h"
#include "nvs.h"
#include "cJSON.h"

#define WIRELESS_BRIDGE_FW_VERSION "1.0.0"
#define WEB_SERVER_DEFAULT_IP "192.168.50.1"
#define DHCP_LEASE_TIME 12*60 // min
#ifdef NRC7292
#define TX_LED_GPIO 8
#define RX_LED_GPIO 11
#else
#define TX_LED_GPIO 10
#define RX_LED_GPIO 11
#endif
#define DHCP_TIMEOUT  10 // sec

static WIFI_CONFIG wifi_config;

#define USE_FLASH_ETHERNET_MAC_ADDR 1
#if defined(USE_FLASH_ETHERNET_MAC_ADDR) && (USE_FLASH_ETHERNET_MAC_ADDR == 1)
typedef struct {
	uint8_t model_name[16];
	uint8_t serial_number[16];
	uint8_t eth_mac[6];
} user_factory_t;

user_factory_t user_factory_info;
#define USER_FACTORY_SIZE 512

#define USE_USER_FACTORY_JSON_DATA_FORMAT 1
#if USE_USER_FACTORY_JSON_DATA_FORMAT
int get_json_str_value(cJSON *cjson, char *key, char **value)
{
	cJSON *cjson_obj = NULL;

	cjson_obj = cJSON_GetObjectItem(cjson, key);
	if (cjson_obj && cjson_obj->valuestring) {
		*value = strdup(cjson_obj->valuestring);
		return 1;
	} else {
		nrc_usr_print("[%s] %s not found in json\n", __func__, key);
		return 0;
	}
}

void parse_user_factory(char *input, user_factory_t* data)
{
    cJSON *cjson = NULL;
    cjson = cJSON_Parse(input);

    if (cjson) {
        cJSON *model_json = cJSON_GetObjectItem(cjson, "model");
        if (model_json && model_json->valuestring) {
            strncpy((char*)data->model_name, model_json->valuestring, sizeof(data->model_name));
            data->model_name[sizeof(data->model_name) - 1] = '\0';
            nrc_usr_print("[%s] model : %s\n", __func__, data->model_name);
        } else {
            nrc_usr_print("[%s] Error: Missing or invalid 'model' field\n", __func__);
            goto exit;
        }

        cJSON *sn_json = cJSON_GetObjectItem(cjson, "sn");
        if (sn_json && sn_json->valuestring) {
            strncpy((char*)data->serial_number, sn_json->valuestring, sizeof(data->serial_number));
            data->serial_number[sizeof(data->serial_number) - 1] = '\0';
            nrc_usr_print("[%s] sn : %s\n", __func__, data->serial_number);
        } else {
            nrc_usr_print("[%s] Error: Missing or invalid 'sn' field\n", __func__);
            goto exit;
        }

        cJSON *eth_mac_json = cJSON_GetObjectItem(cjson, "eth_mac");
        if (eth_mac_json && eth_mac_json->valuestring) {
            int num_matched = sscanf(eth_mac_json->valuestring, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                                      &data->eth_mac[0], &data->eth_mac[1], &data->eth_mac[2],
                                      &data->eth_mac[3], &data->eth_mac[4], &data->eth_mac[5]);
            if (num_matched != 6) {
                nrc_usr_print("[%s] Error: Invalid 'eth_mac' format\n", __func__);
                goto exit;
            }
            nrc_usr_print("[%s] eth_mac : %02x:%02x:%02x:%02x:%02x:%02x\n", __func__,
                          data->eth_mac[0], data->eth_mac[1], data->eth_mac[2],
                          data->eth_mac[3], data->eth_mac[4], data->eth_mac[5]);
        } else {
            nrc_usr_print("[%s] Error: Missing or invalid 'eth_mac' field\n", __func__);
            goto exit;
        }
    } else {
        nrc_usr_print("[%s] JSON parse error\n", __func__);
    }

exit:
    if (cjson) {
        cJSON_Delete(cjson);
    }
}
#endif /* USE_USER_FACTORY_JSON_DATA_FORMAT */

static nrc_err_t get_user_factory_data(user_factory_t* data)
{
	nrc_err_t ret = NRC_FAIL;

	char data_fcatory[USER_FACTORY_SIZE]={0,};
	ret = nrc_get_user_factory(data_fcatory, USER_FACTORY_SIZE);
	if(ret == NRC_SUCCESS){
#if USE_USER_FACTORY_JSON_DATA_FORMAT
		parse_user_factory(data_fcatory, data);
#else
		memcpy(data, data_fcatory, sizeof(user_factory_t));
#endif
		nrc_usr_print("[%s] model_name : %s\n", __func__, data->model_name);
		nrc_usr_print("[%s] serial_number : %s\n", __func__, data->serial_number);
		nrc_usr_print("[%s] eth_mac : %02x:%02x:%02x:%02x:%02x:%02x\n", __func__,
			data->eth_mac[0], data->eth_mac[1], data->eth_mac[2],
			data->eth_mac[3], data->eth_mac[4], data->eth_mac[5]);
	}
	return ret;
}
#endif

static int str_to_argc_argv(const char* str, int* argc, char*** argv) {
	int i, j, len, n;
	char** res, * p;

	/* count the number of arguments */
	len = strlen(str);
	for (i = n = 0; i < len; ) {
		while (i < len && str[i] == ' ')
			i++;
		if (i == len)
			break;
		n++;
		while (i < len && str[i] != ' ')
			i++;
	}

	/* allocate space for argv */
	res = (char**)nrc_mem_malloc((n + 1) * sizeof(char*));
	if (!res)
		return -1;

	/* fill argv with pointers to arguments */
	p = (char*)nrc_mem_malloc(len + 1);
	if (!p) {
		nrc_mem_free(res);
		return -1;
	}
	strcpy(p, str);
	for (i = j = 0; i < len; ) {
		while (i < len && p[i] == ' ')
			p[i++] = '\0';
		if (i == len)
			break;
		res[j++] = p + i;
		while (i < len && p[i] != ' ')
			i++;
	}
	res[j] = NULL;

	/* set argc and argv */
	*argc = n;
	*argv = res;

	return 0;
}

#define NVS_APP_VERSION "app_version"
static nrc_err_t set_app_version_to_nvs(char* data)
{
	nvs_err_t err = NVS_OK;
	nvs_handle_t nvs_handle = 0;
	int32_t nvs_signed_int = 0;

	err = nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle);
	if (NVS_OK != err) {
		nrc_usr_print("[%s] Failed to open NVS\n", __func__);
		return NRC_FAIL;
	}

	nvs_set_str(nvs_handle, NVS_APP_VERSION, (char*)data);
	nrc_usr_print("[%s] Update app_version : %s\n", __func__, data);

	if (nvs_handle)
		nvs_close(nvs_handle);

	return NRC_SUCCESS;
}

static nrc_err_t get_app_version_from_nvs(char* data, int buf_len)
{
	nvs_err_t err = NVS_OK;
	nvs_handle_t nvs_handle = 0;
	int32_t nvs_signed_int = 0;
	size_t length = buf_len;

	err = nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle);
	if (NVS_OK != err) {
		nrc_usr_print("[%s] Failed to open NVS\n", __func__);
		return NRC_FAIL;
	}

	nvs_get_str(nvs_handle, NVS_APP_VERSION, (char*)data, &length);
	nrc_usr_print("[%s] app_version in nvs : %s\n", __func__, data);

	if (nvs_handle)
		nvs_close(nvs_handle);

	return NRC_SUCCESS;
}

void check_fw_version(void)
{
	char app_version_nvs[24];
	memset(app_version_nvs, 0x0, sizeof(app_version_nvs));
	nrc_usr_print("[%s] F/W version : %s\n", __func__, WIRELESS_BRIDGE_FW_VERSION);
	get_app_version_from_nvs(app_version_nvs, sizeof(app_version_nvs));
	if(strcmp(WIRELESS_BRIDGE_FW_VERSION, app_version_nvs)!=0){
		set_app_version_to_nvs(WIRELESS_BRIDGE_FW_VERSION);
	}
}

void user_init(void)
{
	spi_device_t w5500_spi;
	int gpio_int_pin;
	httpd_handle_t httpd;
	char value[32];
	uint8_t mac[6] = {0,};
	uint8_t *addr = NULL;
	uint8_t device_mode = 0;
	uint8_t network_mode = 0;

	char str[200];
	int argc;
	char** argv;

	nrc_uart_console_enable(true);

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

	nrc_led_trx_init(TX_LED_GPIO, RX_LED_GPIO, 500, false);

	run_http_server(&wifi_config);

	nrc_wifi_set_use_4address(true);

	nrc_wifi_set_config(&wifi_config);

	/* Set Country Code */
	if(nrc_wifi_set_country(0, nrc_wifi_country_from_string((char *)wifi_config.country)) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set Country\n", __func__);
		return;
	}

#if defined(USE_FLASH_ETHERNET_MAC_ADDR) && (USE_FLASH_ETHERNET_MAC_ADDR == 1)
	if(get_user_factory_data(&user_factory_info) == NRC_SUCCESS){
		addr = user_factory_info.eth_mac;
		if ((memcmp(addr, "\xFF\xFF\xFF\xFF\xFF\xFF", 6) == 0) ||
			(memcmp(addr, "\x00\x00\x00\x00\x00\x00", 6) == 0)) {
			addr = NULL;
		}
	}
#endif

	nrc_eth_set_ethernet_mode(wifi_config.device_mode); /* 0:STA, 1:AP */
	nrc_eth_set_network_mode(wifi_config.network_mode); /*0:bridge, 1:NAT */

	if (ethernet_init(&w5500_spi, addr, gpio_int_pin) != NRC_SUCCESS) {
		nrc_usr_print("[%s] Error initializing ethernet...\n", __func__);
		return;
	}

	if (wifi_config.network_mode == WIFI_NETWORK_MODE_BRIDGE) {
		/* Set IP address */
		if (wifi_config.ip_mode ==  WIFI_STATIC_IP) {
			nrc_eth_set_ip_mode(NRC_ETH_IP_MODE_STATIC);
			memset(str, 0x0, sizeof(str));
			sprintf(str, "br %s -n %s -g %s", wifi_config.static_ip,
				wifi_config.netmask,  wifi_config.gateway);
			nrc_usr_print("wifi_config : %s\n", str);
			if (str_to_argc_argv(str, &argc, &argv) == -1) {
				nrc_usr_print("Failed to convert string to argc and argv\n");
				return;
			}
			wifi_ifconfig(argc, argv);
			nrc_usr_print("The bridge IP address set to : %s\n", wifi_config.static_ip);

			nrc_mem_free(argv[0]);
			nrc_mem_free(argv);
		} else {
			nrc_eth_set_ip_mode(NRC_ETH_IP_MODE_DHCP);
			nrc_usr_print("Starting DHCP client on the bridge...\n");
			/* If bridge ip is not set, start DHCP client */
			/* Call wifi_station_dhcpc_start with vif set to 0 */
			/* vif will be ignored by below call if the network mode is bridge */
			/* API name is set to _station_, but it is applicable for AP as well */
			wifi_station_dhcpc_start(0);
		}

		/* set DHCP server */
		if (wifi_config.dhcp_server == 1 && wifi_config.device_mode == WIFI_MODE_AP) {
			nrc_usr_print("The DHCP server on bridge interface\n");
			memset(str, 0x0, sizeof(str));
			sprintf(str, "-i br -lt %d", DHCP_LEASE_TIME);
			nrc_usr_print("dhcps : %s\n", str);
			if (str_to_argc_argv(str, &argc, &argv) == -1) {
				nrc_usr_print("Failed to convert string to argc and argv\n");
				return;
			}
			wifi_dhcps(argc, argv);
			nrc_mem_free(argv[0]);
			nrc_mem_free(argv);
		}
	} else {
		nrc_usr_print("Setting eth address to %s...\n", WEB_SERVER_DEFAULT_IP);
		memset(str, 0x0, sizeof(str));
		sprintf(str, "eth %s", WEB_SERVER_DEFAULT_IP);
		nrc_usr_print("wifi_config : %s\n", str);
		if (str_to_argc_argv(str, &argc, &argv) == -1) {
			nrc_usr_print("Failed to convert string to argc and argv\n");
			return;
		}
		wifi_ifconfig(argc, argv);
		nrc_mem_free(argv[0]);
		nrc_mem_free(argv);

		nrc_usr_print("The DHCP server on ethernet interface\n");
		memset(str, 0x0, sizeof(str));
		sprintf(str, "-i eth -lt %d", 5);
		nrc_usr_print("dhcps : %s\n", str);
		if (str_to_argc_argv(str, &argc, &argv) == -1) {
			nrc_usr_print("Failed to convert string to argc and argv\n");
			return;
		}
		wifi_dhcps(argc, argv);
		nrc_mem_free(argv[0]);
		nrc_mem_free(argv);

		if (wifi_config.ip_mode ==  WIFI_STATIC_IP) {
			nrc_usr_print("The wlan0 IP address has been set : %s\n", wifi_config.static_ip);
			memset(str, 0x0, sizeof(str));
			sprintf(str, "wlan0 %s -n %s -g %s", wifi_config.static_ip,
					wifi_config.netmask,  wifi_config.gateway);
			nrc_usr_print("wifi_config : %s\n", str);
			if (str_to_argc_argv(str, &argc, &argv) == -1) {
				nrc_usr_print("Failed to convert string to argc and argv\n");
				return;
			}
			wifi_ifconfig(argc, argv);
			nrc_mem_free(argv[0]);
			nrc_mem_free(argv);
		}
	}

	/* Run STA or AP */
	if (wifi_config.device_mode == WIFI_MODE_STATION) {
		/* 4 address support only necessary for Bridge with multiple ethernet devices */
		if (wifi_config.network_mode != WIFI_NETWORK_MODE_BRIDGE) {
			nrc_wifi_set_use_4address(false);
		}
		nrc_usr_print("[%s] Device in Station mode...\n", __func__);
		nrc_usr_print("[%s] Connecting to \"%s\"...\n", __func__, wifi_config.ssid);
		if (connect_to_ap(&wifi_config) == NRC_SUCCESS) {
			nrc_usr_print("Connection to AP successful...\n");
		} else {
			nrc_usr_print("Error connecting to AP...\n");
		}
	} else {
		nrc_usr_print("[%s] Device in Access Point mode...\n", __func__);
		nrc_usr_print("[%s] Starting AP with ssid \"%s\"...\n", __func__, wifi_config.ssid);
		if (start_softap(&wifi_config) == NRC_SUCCESS) {
			nrc_usr_print("Soft AP Started...\n");
			nrc_usr_print ("\033[31m [%s] dhcp_server start? %d \033[39m\n",
				__func__, wifi_config.dhcp_server);
		} else {
			nrc_usr_print("Error Starting Soft AP...\n");
		}
		nrc_usr_print("[%s] ssid %s security %d channel %d\n", __func__,
			wifi_config.ssid, wifi_config.security_mode, wifi_config.channel);
	}
}
