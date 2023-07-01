/*
 * MIT License
 *
 * Copyright (c) 2022 Newracom, Inc.
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

#ifndef __NRC_ATCMD_WIFI_H__
#define __NRC_ATCMD_WIFI_H__
/**********************************************************************************************/

#include "wifi_api.h"
#include "lwip_ping.h"


/*
 * String Length
 */
#define ATCMD_WIFI_MACADDR_LEN			17
#define ATCMD_WIFI_COUNTRY_LEN			2
#define ATCMD_WIFI_BSSID_LEN			ATCMD_WIFI_MACADDR_LEN

#define ATCMD_WIFI_SSID_LEN_MIN			1
#define ATCMD_WIFI_SSID_LEN_MAX			32

#define ATCMD_WIFI_SECURITY_LEN_MIN		4
#define ATCMD_WIFI_SECURITY_LEN_MAX		8

#define ATCMD_WIFI_PASSWORD_LEN_MIN		8
#define ATCMD_WIFI_PASSWORD_LEN_MAX		63

#define ATCMD_WIFI_IPADDR_LEN_MIN		ATCMD_IPADDR_LEN_MIN
#define ATCMD_WIFI_IPADDR_LEN_MAX		ATCMD_IPADDR_LEN_MAX

#define ATCMD_WIFI_SCAN_FLAGS_LEN_MAX	128


/*
 * Default Settings
 */
#define ATCMD_WIFI_INIT_COUNTRY			"US"
#define ATCMD_WIFI_INIT_TXPOWER			24
#define ATCMD_WIFI_INIT_TXPOWER_TYPE	24

#define ATCMD_WIFI_INIT_SSID			"halow"
#define ATCMD_WIFI_INIT_BSSID			"00:00:00:00:00:00"
#define ATCMD_WIFI_INIT_SECURITY		"open"
#define ATCMD_WIFI_INIT_PASSWORD		""

#define ATCMD_WIFI_INIT_PING_INTERVAL	1000
#define ATCMD_WIFI_INIT_PING_COUNT		5
#define ATCMD_WIFI_INIT_PING_SIZE		64

#define ATCMD_WIFI_BSS_MAX_IDLE_PERIOD		0
#define ATCMD_WIFI_BSS_MAX_IDLE_RETRY_MIN	3
#define ATCMD_WIFI_BSS_MAX_IDLE_RETRY_MAX	100


/*
 * Country Code
 */
#define ATCMD_WIFI_COUNTRY_ARRAY		{ "AU", "CN", "EU", "JP", "NZ", "TW", "US", "K0", "K1", "K2" }
#define ATCMD_WIFI_COUNTRY_STRING		"AU|CN|EU|JP|NZ|TW|US|K0|K1|K2"


/*
 * Task
 */
#define ATCMD_WIFI_DHCP_TASK_PRIORITY			ATCMD_TASK_PRIORITY
#define ATCMD_WIFI_DHCP_TASK_STACK_SIZE			((4 * 1024) / sizeof(StackType_t))

#define ATCMD_WIFI_ROAMING_TASK_PRIORITY		ATCMD_TASK_PRIORITY
#define ATCMD_WIFI_ROAMING_TASK_STACK_SIZE		((4 * 1024) / sizeof(StackType_t))

/*
 * RSSI
 */
#define ATCMD_WIFI_RSSI_MAX		0
#define ATCMD_WIFI_RSSI_MIN		-128

/**********************************************************************************************/

enum ATCMD_WIFI_SECURITY
{
	ATCMD_WIFI_SEC_OPEN = 0,
	ATCMD_WIFI_SEC_PSK,
	ATCMD_WIFI_SEC_OWE,
	ATCMD_WIFI_SEC_SAE,

	ATCMD_WIFI_SEC_MAX
};

enum ATCMD_WIFI_EVENT
{
	ATCMD_WIFI_EVT_SCAN_DONE = 0,
	ATCMD_WIFI_EVT_CONNECT_SUCCESS,
	ATCMD_WIFI_EVT_DISCONNECT,

	ATCMD_WIFI_EVT_STA_CONNECT,
	ATCMD_WIFI_EVT_STA_DISCONNECT,

	ATCMD_WIFI_EVT_MAX
};

/**********************************************************************************************/

typedef uint32_t atcmd_wifi_event_t;
typedef uint8_t atcmd_wifi_power_t;

typedef char atcmd_wifi_ipaddr_t[ATCMD_WIFI_IPADDR_LEN_MAX + 1];
typedef char atcmd_wifi_macaddr_t[ATCMD_WIFI_MACADDR_LEN + 1];
typedef char atcmd_wifi_country_t[ATCMD_WIFI_COUNTRY_LEN + 1];
typedef char atcmd_wifi_ssid_t[ATCMD_WIFI_SSID_LEN_MAX + 1];
typedef char atcmd_wifi_bssid_t[ATCMD_WIFI_BSSID_LEN + 1];
typedef char atcmd_wifi_security_t[ATCMD_WIFI_SECURITY_LEN_MAX + 1];
typedef char atcmd_wifi_password_t[ATCMD_WIFI_PASSWORD_LEN_MAX + 1];

typedef struct
{
#define ATCMD_WIFI_CHANNELS_MAX		50

	int n_freq;
	uint16_t freq[ATCMD_WIFI_CHANNELS_MAX];
} atcmd_wifi_channels_t;

typedef struct
{
#define ATCMD_WIFI_SCAN_SET_PARAM_MAX	15

	bool scanning;
	SCAN_RESULTS results;
} atcmd_wifi_scan_t;

typedef struct
{
	bool connected;
	bool connecting;
	bool disconnecting;

	atcmd_wifi_ssid_t ssid;
	atcmd_wifi_bssid_t bssid;
	atcmd_wifi_security_t security;
	atcmd_wifi_password_t password;
} atcmd_wifi_connect_t;

typedef struct
{
	int scan_delay;
	int rssi_threshold;
	int rssi_level;
} atcmd_wifi_roaming_params_t;

typedef struct
{
	bool enable;
	atcmd_wifi_roaming_params_t params;

	TaskHandle_t task;
} atcmd_wifi_roaming_t;

typedef struct
{
#ifdef CONFIG_ATCMD_IPV6
	_lwip_ping_params_t params[2];
#else
	_lwip_ping_params_t params[1];
#endif
} atcmd_wifi_ping_t;

#ifdef CONFIG_ATCMD_SOFTAP
typedef struct
{
	bool active;
	bool dhcp_server;

	uint16_t channel_bw;
	uint16_t channel_freq;

	struct
	{
		int period;
		int retry;
	} bss_max_idle;

	atcmd_wifi_ssid_t ssid;
	atcmd_wifi_security_t security;
	atcmd_wifi_password_t password;
} atcmd_wifi_softap_t;
#endif

typedef struct
{
	SemaphoreHandle_t lock;

	atcmd_wifi_event_t event;

	atcmd_wifi_country_t country;
	atcmd_wifi_channels_t channels;
	atcmd_wifi_power_t txpower;

	atcmd_wifi_scan_t scan;
	atcmd_wifi_connect_t connect;
	atcmd_wifi_ping_t ping;
	atcmd_wifi_roaming_t roaming;
#ifdef CONFIG_ATCMD_SOFTAP
	atcmd_wifi_softap_t softap;
#endif
} atcmd_wifi_info_t;

/**********************************************************************************************/

#define ATCMD_WIFI_LOCK()				ASSERT(atcmd_wifi_lock())
#define ATCMD_WIFI_UNLOCK()				ASSERT(atcmd_wifi_unlock())

#define ATCMD_MSG_WEVENT(fmt, ...)		ATCMD_MSG_EVENT("WEVENT", fmt, ##__VA_ARGS__)

extern int atcmd_wifi_enable (void);
extern void atcmd_wifi_disable (void);

extern bool atcmd_wifi_lock (void);
extern bool atcmd_wifi_unlock (void);

extern void atcmd_wifi_deep_sleep_send_event (void);

/**********************************************************************************************/
#endif /* #ifndef __NRC_ATCMD_WIFI_H__ */

