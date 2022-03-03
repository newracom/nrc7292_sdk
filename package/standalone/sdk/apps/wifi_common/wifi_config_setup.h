/*
 * MIT License
 *
 * Copyright (c) 2021 Newracom, Inc.
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

#ifndef __WIFI_CONFIG_SETUP_H__
#define __WIFI_CONFIG_SETUP_H__

#include "nrc_types.h"

typedef struct  {
	uint8_t ssid[MAX_SSID_LENGTH + 1];
	uint8_t bssid[MAX_BSSID_LENGTH + 1];
	uint8_t country[MAX_CC_LENGTH + 1];
	uint8_t security_mode;
	uint8_t password[MAX_PW_LENGTH + 1];
	uint16_t scan_freq_list[MAX_FREQ_NUM];
	uint8_t scan_freq_num;
	uint32_t channel;
	uint8_t count;
	int interval;
	int short_bcn_interval;
	int duration;
	uint8_t ip_mode;
	uint8_t static_ip[MAX_STATIC_IP_LENGTH];
	uint8_t ap_ip[MAX_STATIC_IP_LENGTH];
	uint8_t remote_addr[MAX_STATIC_IP_LENGTH];
	uint16_t remote_port;
	uint16_t s1g_channel;
	int tx_power;
	uint8_t dhcp_server;
	int test_running;
}WIFI_CONFIG;
#define WIFI_CONFIG_SIZE	sizeof (WIFI_CONFIG)

void set_wifi_config(WIFI_CONFIG *param);
void set_wifi_softap_config(WIFI_CONFIG *param);

#endif /* __WIFI_CONFIG_SETUP_H__ */
