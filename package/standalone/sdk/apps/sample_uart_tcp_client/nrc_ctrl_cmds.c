/*
 * MIT License
 *
 * Copyright (c) 2023 Newracom, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "nrc_sdk.h"
#include "nrc_tcp_server.h"
#include "tlv.h"
#include "nrc_ctrl_func.h"
#include "nrc_fota.h"
#include "sample_uart_tcp_client_version.h"

#define MAX_VALUE_SIZE 1024

static void send_success(int sock, unsigned int tag, char *value, size_t size)
{
	char *response = NULL;

	response = tlv_encode(tag, size, value);
	if (response) {
		write_to_socket(sock, response, sizeof(int) + sizeof(int) + size);
		nrc_mem_free(response);
	}
}

static void send_failure(int sock, unsigned int tag)
{
	char *response = NULL;
	size_t size = strlen("NOK");
	response = tlv_encode(tag, size, "NOK");
	if (response) {
		write_to_socket(sock, response, sizeof(int) + sizeof(int) + size);
		nrc_mem_free(response);
	}
}

void handle_tlv_command(int sock, char *buffer, size_t received)
{
	unsigned int tag = 0;
	size_t size = 0;
	char value[MAX_VALUE_SIZE] = {0,};
	char* value_temp_ptr = NULL;
	int success = 1;

	char fw_upgrade_url[MAX_VALUE_SIZE] = {0,};
	size_t url_size = 0;
	int fw_upgrade = 0;

	int8_t i8_value = 0;
	uint8_t u8_value = 0;
	uint8_t u8_value1 = 0;
	uint16_t u16_value = 0;
	int i32_value = 0;
	unsigned int u32_value = 0;
	int32_t long_value = 0;
	uint32_t ulong_value = 0;
	char *token;

	tlv_decode(buffer, &tag, &size, value);

	nrc_usr_print("[%s] tag: %d. length : %d, value : %s\n", __func__, tag, size, value);
	for (int i = 0; i < size; i++) {
		nrc_usr_print("%c\n", value[i]);
	}
	switch (tag) {
	case TLV_CMD_GET_SSID:
		size = MAX_VALUE_SIZE;
		if (nrc_ctrl_get_ssid(value, &size) < 0) {
			success = 0;
		}
		break;
	case TLV_CMD_SET_SSID:
		if (nrc_ctrl_set_ssid(value) < 0) {
			success = 0;
		}
		break;
	case TLV_CMD_GET_SECURITY:
		if (nrc_ctrl_get_security(&u8_value) < 0) {
			success = 0;
			break;
		}
		sprintf(value, "%d", u8_value);
		size = strlen(value) + 1;
		break;
	case TLV_CMD_SET_SECURITY:
		u8_value = (uint8_t) atoi(value);
		if (nrc_ctrl_set_security(u8_value) < 0) {
			success = 0;
		}
		break;
	case TLV_CMD_GET_PASSWORD:
		size = MAX_VALUE_SIZE;
		if (nrc_ctrl_get_password(value, &size) < 0) {
			success = 0;
		}
		break;
	case TLV_CMD_SET_PASSWORD:
		if (nrc_ctrl_set_password(value) < 0) {
			success = 0;
		}
		break;
	case TLV_CMD_GET_COUNTRY:
		size = MAX_VALUE_SIZE;
		if (nrc_ctrl_get_country(value, &size) < 0) {
			success = 0;
		}
		break;
	case TLV_CMD_SET_COUNTRY:
		if (nrc_ctrl_set_country(value) < 0) {
			success = 0;
		}
		break;
	case TLV_CMD_GET_IPMODE:
		if (nrc_ctrl_get_ipmode(&u8_value) < 0) {
			success = 0;
			break;
		}
		sprintf(value, "%d", u8_value);
		size = strlen(value) + 1;
		break;
	case TLV_CMD_SET_IPMODE:
		u8_value = (uint8_t) atoi(value);
		if (nrc_ctrl_set_ipmode(u8_value) < 0) {
			success = 0;
		}
		break;
	case TLV_CMD_GET_CONN_TIMEOUT:
		if (nrc_ctrl_get_conn_timeout(&long_value) < 0) {
			success = 0;
			break;
		}
		sprintf(value, "%ld", long_value);
		size = strlen(value) + 1;
		break;
	case TLV_CMD_SET_CONN_TIMEOUT:
		i32_value = (int32_t) atol(value);
		if (nrc_ctrl_set_conn_timeout(i32_value) < 0) {
			success = 0;
		}
		break;
	case TLV_CMD_GET_DISCONN_TIMEOUT:
		if (nrc_ctrl_get_disconn_timeout(&long_value) < 0) {
			success = 0;
			break;
		}
		sprintf(value, "%ld", long_value);
		size = strlen(value) + 1;
		break;
	case TLV_CMD_SET_DISCONN_TIMEOUT:
		i32_value = (int32_t) atol(value);
		if (nrc_ctrl_set_disconn_timeout(i32_value) < 0) {
			success = 0;
		}
		break;
	case TLV_CMD_GET_RATE_CONTROL:
		if (nrc_ctrl_get_rc(&u8_value) < 0) {
			success = 0;
			break;
		}
		sprintf(value, "%d", u8_value);
		size = strlen(value) + 1;
		break;
	case TLV_CMD_SET_RATE_CONTROL:
		u8_value = (uint8_t) atoi(value);
		if (u8_value != 0 && u8_value != 1) {
			success = 0;
			break;
		}
		if (nrc_ctrl_set_rc(u8_value) < 0) {
			success = 0;
		}
		break;
	case TLV_CMD_GET_MCS:
		if (nrc_ctrl_get_mcs(&u8_value) < 0) {
			success = 0;
			break;
		}
		sprintf(value, "%d", u8_value);
		size = strlen(value) + 1;
		break;
	case TLV_CMD_SET_MCS:
		u8_value = (uint8_t) atoi(value);
		if (u8_value > 10) {
			success = 0;
			break;
		}
		if (nrc_ctrl_set_mcs(u8_value) < 0) {
			success = 0;
		}
		break;
	case TLV_CMD_GET_CCA_THRES:
		if (nrc_ctrl_get_cca_thres(&i8_value) < 0) {
			success = 0;
			break;
		}
		sprintf(value, "%d", i8_value);
		size = strlen(value) + 1;
		break;
	case TLV_CMD_SET_CCA_THRES:
		if (nrc_ctrl_set_cca_thres(i8_value) < 0) {
			success = 0;
		}
		break;
	case TLV_CMD_GET_STATICIP:
		size = MAX_VALUE_SIZE;
		if (nrc_ctrl_get_static_ip(value, &size) < 0) {
			success = 0;
		}
		break;
	case TLV_CMD_SET_STATICIP:
		if (nrc_ctrl_set_static_ip(value) < 0) {
			success = 0;
		}
		break;
	case TLV_CMD_GET_NETMASK:
		size = MAX_VALUE_SIZE;
		if (nrc_ctrl_get_netmask(value, &size) < 0) {
			success = 0;
		}
		break;
	case TLV_CMD_SET_NETMASK:
		if (nrc_ctrl_set_netmask(value) < 0) {
			success = 0;
		}
		break;
	case TLV_CMD_GET_GATEWAY:
		size = MAX_VALUE_SIZE;
		if (nrc_ctrl_get_gateway(value, &size) < 0) {
			success = 0;
		}
		break;
	case TLV_CMD_SET_GATEWAY:
		if (nrc_ctrl_set_gateway(value) < 0) {
			success = 0;
		}
		break;
	case TLV_CMD_SHOW_CONFIG:
		size = MAX_VALUE_SIZE;
		if (nrc_ctrl_show_config(value, &size) < 0) {
			success = 0;
		}
		break;
	case TLV_CMD_SHOW_SYSCONFIG:
		size = MAX_VALUE_SIZE;
		if (nrc_ctrl_show_sysconfig(value, &size) < 0) {
			success = 0;
		}
		break;
	case TLV_CMD_GET_APINFO:
		size = MAX_VALUE_SIZE;
		if (nrc_ctrl_show_apinfo(value, &size) < 0) {
			success = 0;
		}
		break;
	case TLV_CMD_GET_TXPOWER:
		if (nrc_ctrl_get_txpower(&u8_value, &u8_value1) < 0) {
			success = 0;
			break;
		}
		sprintf(value, "%d,%d", u8_value, u8_value1);
		size = strlen(value) + 1;
		break;
	case TLV_CMD_SET_TXPOWER:
		value_temp_ptr = malloc(10);
		memset(value_temp_ptr, 0x0, 10);
		memcpy(value_temp_ptr, value, strlen(value));
		token = strtok(value_temp_ptr, ",");
		u8_value = atoi(token);
		token = strtok(NULL, ",");
		u8_value1 = atoi(token);
		if (nrc_ctrl_set_txpower(u8_value, u8_value1) < 0) {
			success = 0;
		}
		break;
#ifdef INCLUDE_SCAN_BACKOFF
	case TLV_CMD_GET_SCAN_MAX_INTERVAL:
		u32_value = nrc_ctrl_get_scan_max_interval();
		sprintf(value, "%d", u32_value);
		size = strlen(value) + 1;
		break;
	case TLV_CMD_SET_SCAN_MAX_INTERVAL:
		u32_value = (uint32_t) atoi(value);
		if (u8_value < 60) {
			success = 0;
			break;
		}
		nrc_ctrl_set_scan_max_interval(u32_value);
		break;
	case TLV_CMD_GET_BACKOFF_START_COUNT:
		u32_value = nrc_ctrl_get_backoff_start_count();
		sprintf(value, "%d", u32_value);
		size = strlen(value) + 1;
		break;
	case TLV_CMD_SET_BACKOFF_START_COUNT:
		u32_value = (uint32_t) atoi(value);
		nrc_ctrl_set_backoff_start_count(u32_value);
		break;
#endif
	case TLV_CMD_GET_APPLICATION_VERSION:
		sprintf(value, "%d.%d.%d", SAMPLE_UART_TCP_CLIENT_MAJOR, SAMPLE_UART_TCP_CLIENT_MINOR, SAMPLE_UART_TCP_CLIENT_PATCH);
		size = strlen(value) + 1;
		break;
	case TLV_CMD_GET_SDK_VERSION:
		size = MAX_VALUE_SIZE;
		if (nrc_ctrl_get_version(value, &size) < 0) {
			success = 0;
		}
		break;
	case TLV_CMD_GET_MACADDR:
		if (nrc_wifi_get_mac_address(0, value) != WIFI_SUCCESS) {
			success = 0;
			break;
		}
		size = strlen(value) + 1;
		break;
	case TLV_CMD_GET_RSSI:
		if (nrc_wifi_get_rssi(0, &i8_value) != WIFI_SUCCESS) {
			success = 0;
			break;
		}
		sprintf(value, "%d", i8_value);
		size = strlen(value) + 1;
		break;
	case TLV_CMD_GET_SNR:
		if (nrc_wifi_get_snr(0, &u8_value) != WIFI_SUCCESS) {
			success = 0;
			break;
		}
		sprintf(value, "%d", u8_value);
		size = strlen(value) + 1;
		break;
	case TLV_CMD_FW_UPGRADE:
		memcpy(fw_upgrade_url, value, size);
		url_size = size;
		fw_upgrade = 1;
		sprintf(value, "Upgrade started\n");
		size = strlen(value) + 1;
		break;
	case TLV_CMD_REBOOT:
		nrc_ctrl_sys_reboot();
		sprintf(value, "sys_reboot");
		size = strlen(value) + 1;
		break;
	case TLV_CMD_NVS_RESET:
		if (nrc_ctrl_nvs_reset() < 0) {
			success = 0;
		}
		break;
	default:
		nrc_usr_print("invalid tag: %d\n", tag);
		success = 0;
		break;
	}

	if (success) {
		send_success(sock, tag, value, size);
	} else {
		send_failure(sock, tag);
	}
	if (fw_upgrade) {
		nrc_usr_print("Starting Firmware Upgrade...\n");
		_delay_ms(2000);
		if (nrc_fota_start(fw_upgrade_url, url_size) < 0) {
			nrc_usr_print("FOTA failed\n");
		}
	}
}
