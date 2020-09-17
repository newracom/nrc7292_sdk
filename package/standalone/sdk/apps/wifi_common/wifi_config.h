/*
 * MIT License
 *
 * Copyright (c) 2020 Newracom, Inc.
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

#ifndef __WIFI_CONFIG_H__
#define __WIFI_CONFIG_H__

#define STR_SSID "halow_demo"
#define COUNTRY_CODE "KR" /* KR:Korea, JP: Japan, US:United States, TW: Taiwan, EU: EURO */

#define NRC_WIFI_SECURE  MODE_WPA2 /* MODE_OPEN,  MODE_WPA2 */
#define NRC_WIFI_DHCP_ENABLE 0

#define NRC_REMOTE_ADDRESS "192.168.200.1"

#define TX_POWER 17 /* 17 dBm */

#define NRC_WIFI_TEST_COUNT 100
#define NRC_WIFI_TEST_INTERVAL 1000 /* ms */

#if defined(NRC_WIFI_SECURE) && (NRC_WIFI_SECURE == MODE_WPA2)
#define NRC_WIFI_PASSWORD  "12345678"
#endif

#if (NRC_WIFI_DHCP_ENABLE != 1)
#define NRC_STATIC_IP "192.168.200.13"
#endif

#define NRC_WIFI_SOFTAP_DHCP_SERVER 1
#define NRC_AP_IP "192.168.200.1"

#endif /* __WIFI_CONFIG_H__ */
