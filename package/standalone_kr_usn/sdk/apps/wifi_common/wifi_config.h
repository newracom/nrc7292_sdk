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

#ifndef __WIFI_CONFIG_H__
#define __WIFI_CONFIG_H__

/* SSID (Service Set Identifier) */
#ifndef STR_SSID
#define STR_SSID "halow_demo"
#endif /* STR_SSID */

/* KR:Korea, JP: Japan, US:United States, TW: Taiwan, EU: EURO */
#ifndef COUNTRY_CODE
#define COUNTRY_CODE "US"
#endif /* COUNTRY_CODE */

/* WIFI_SEC_OPEN, WIFI_SEC_WPA2, WIFI_SEC_WPA3_SAE, WIFI_SEC_WPA3_SAE */
#ifndef NRC_WIFI_SECURE
#define NRC_WIFI_SECURE  WIFI_SEC_OPEN
#endif /* NRC_WIFI_SECURE */

/* WIFI_STATIC_IP, WIFI_DYNAMIC_IP*/
#ifndef NRC_WIFI_IP_MODE
#define NRC_WIFI_IP_MODE	WIFI_DYNAMIC_IP
#endif /* NRC_WIFI_IP_MODE */

/* Remote IP address to connect */
#ifndef NRC_REMOTE_ADDRESS
#define NRC_REMOTE_ADDRESS "192.168.200.1"
#endif /* NRC_REMOTE_ADDRESS */

/* Remote Port number to connect */
#ifndef NRC_REMOTE_PORT
#define NRC_REMOTE_PORT		8099
#endif /* NRC_REMOTE_PORT */

/* Scan APs on specfied frequencies */
#ifndef NRC_WIFI_SCAN_LIST
#define NRC_WIFI_SCAN_LIST 0
#endif /* NRC_WIFI_SCAN_LIST */

/* Tx power value (dBm) */
#ifndef TX_POWER
#define TX_POWER 17
#endif /* TX_POWER */

/* Test number */
#ifndef NRC_WIFI_TEST_COUNT
#define NRC_WIFI_TEST_COUNT 20
#endif /* NRC_WIFI_TEST_COUNT */

/* Test interval (ms)*/
#ifndef NRC_WIFI_TEST_INTERVAL
#define NRC_WIFI_TEST_INTERVAL 1000
#endif /* NRC_WIFI_TEST_INTERVAL */

/* Test duration (ms)*/
#ifndef NRC_WIFI_TEST_DURATION
#define NRC_WIFI_TEST_DURATION 10000 /* ms */
#endif /* NRC_WIFI_TEST_DURATION */

#if defined(NRC_WIFI_SECURE) && (NRC_WIFI_SECURE == WIFI_SEC_WPA2)
/* password */
#ifndef NRC_WIFI_PASSWORD
#define NRC_WIFI_PASSWORD  "12345678"
#endif /* NRC_WIFI_PASSWORD */
#endif /* defined(NRC_WIFI_SECURE) && (NRC_WIFI_SECURE == WIFI_SEC_WPA2) */

#if (NRC_WIFI_IP_MODE == WIFI_STATIC_IP)
/* Static IP address */
#ifndef NRC_STATIC_IP
#define NRC_STATIC_IP "192.168.200.13"
#endif /* NRC_STATIC_IP */
#endif /* (NRC_WIFI_IP_MODE == WIFI_STATIC_IP) */

#ifdef CONFIG_IPV6
/* Static IPv6 address to use. */
/* In order to override this configure it using NVS key "wlan0_ip6" */
#ifndef NRC_STATIC_IP6
#define NRC_STATIC_IP6 "fc00::c0a8:c80d"
#endif /* NRC_STATIC_IP6 */
#endif /* CONFIG_IPV6 */

#if NRC_WIFI_SCAN_LIST
/* Scan channels and lists */
#if defined(INCLUDE_KR_MIC_CHANNEL)
#ifndef NRC_WIFI_SCAN_FREQ_NUM
#define NRC_WIFI_SCAN_FREQ_NUM 3
uint16_t nrc_scan_freq_list[NRC_WIFI_SCAN_FREQ_NUM]={9255,9275,9270};
#endif /* NRC_WIFI_SCAN_FREQ_NUM */
#else
#ifndef NRC_WIFI_SCAN_FREQ_NUM
#define NRC_WIFI_SCAN_FREQ_NUM 5
uint16_t nrc_scan_freq_list[NRC_WIFI_SCAN_FREQ_NUM]={9055,9175,9090,9250,9220};
#endif /* NRC_WIFI_SCAN_FREQ_NUM */
#endif /* !INCLUDE_KR_MIC_CHANNEL */
#endif /* NRC_WIFI_SCAN_LIST */

/* DHCP server is enable or disable in softAP */
#ifndef NRC_WIFI_SOFTAP_DHCP_SERVER
#define NRC_WIFI_SOFTAP_DHCP_SERVER 1
#endif /* NRC_WIFI_SOFTAP_DHCP_SERVER */

/* IP address for softAP */
#ifndef NRC_AP_IP
#define NRC_AP_IP "192.168.200.1"
#endif /* NRC_AP_IP */

/* Default CH(MHz) : KR:920.0(KR MIC:927.5) JP:918.0 US:925.0 TW:839.5 EU:864.0 */
#ifndef NRC_AP_SET_CHANNEL
#define NRC_AP_SET_CHANNEL 9225
#endif /* NRC_AP_SET_CHANNEL */

#if defined(INCLUDE_SCAN_MODE)
/* Scan method */
#ifndef WIFI_SCAN_MODE
#define WIFI_SCAN_MODE	WIFI_SCAN_NORMAL
#endif /* WIFI_SCAN_MODE */
#endif /* INCLUDE_SCAN_MODE */

#endif /* __WIFI_CONFIG_H__ */
