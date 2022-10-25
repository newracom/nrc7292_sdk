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

#ifndef __NRC_ATCMD_WIFI_H__
#define __NRC_ATCMD_WIFI_H__
/**********************************************************************************************/

#ifndef CONFIG_ATCMD_WITHOUT_LWIP
#include "lwip_ping.h"
#endif

/*
 * String Length
 */
#define ATCMD_WIFI_IPADDR_LEN_MAX		15
#define ATCMD_WIFI_MACADDR_LEN			17
#define ATCMD_WIFI_COUNTRY_LEN			2
#define ATCMD_WIFI_BSSID_LEN			ATCMD_WIFI_MACADDR_LEN
#define ATCMD_WIFI_SSID_LEN_MAX			32
#define ATCMD_WIFI_SECURITY_LEN_MAX		8
#define ATCMD_WIFI_PASSWORD_LEN_MIN		8
#define ATCMD_WIFI_PASSWORD_LEN_MAX		63
#define ATCMD_WIFI_SCAN_FLAGS_LEN_MAX	128


/*
 * Default Settings
 */
#define ATCMD_WIFI_INIT_COUNTRY			"KR"
#define ATCMD_WIFI_INIT_TXPOWER			17

#define ATCMD_WIFI_INIT_SSID			"halow"
#define ATCMD_WIFI_INIT_BSSID			"00:00:00:00:00:00"
#define ATCMD_WIFI_INIT_SECURITY		"open"
#define ATCMD_WIFI_INIT_PASSWORD		""

#define ATCMD_WIFI_INIT_PING_INTERVAL	1000
#define ATCMD_WIFI_INIT_PING_COUNT		5
#define ATCMD_WIFI_INIT_PING_SIZE		64

#define ATCMD_WIFI_BSS_MAX_IDLE_PERIOD	0
#define ATCMD_WIFI_BSS_MAX_IDLE_RETRY	0


/*
 * Country Code
 */
#define ATCMD_WIFI_COUNTRY_NUMBER		8
#define ATCMD_WIFI_COUNTRY_ARRAY		{ "AU", "CN", "EU", "JP", "NZ", "TW", "US", "KR" }
#define ATCMD_WIFI_COUNTRY_STRING		"AU|CN|EU|JP|NZ|TW|US|KR"


/*
 * Roaming Task
 */
#define ATCMD_WIFI_ROAMING_TASK_PRIORITY		ATCMD_TASK_PRIORITY
#define ATCMD_WIFI_ROAMING_TASK_STACK_SIZE		((4 * 1024) / sizeof(StackType_t))

/*
 * RSSI
 */
#define ATCMD_WIFI_RSSI_MAX		0
#define ATCMD_WIFI_RSSI_MIN		-128

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
	bool requesting;
	bool responsed;
} atcmd_wifi_dhcp_t;

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

#ifndef CONFIG_ATCMD_WITHOUT_LWIP
typedef _lwip_ping_params_t atcmd_wifi_ping_t;
#endif

typedef struct
{
	bool active;
	bool dhcp_server;

	int net_id;

	struct
	{
		int number;
		int freq;
	} channel; /* S1G */

	struct
	{
		int period;
		int retry;
	} bss_max_idle;

	atcmd_wifi_ssid_t ssid;
	atcmd_wifi_security_t security;
	atcmd_wifi_password_t password;

	union
	{
		char *ipaddr[3];

		struct
		{
			char *addr;
			char *netmask;
			char *gateway;
		} ip;
	};
} atcmd_wifi_softap_t;

typedef struct
{
	atcmd_wifi_event_t event;

	atcmd_wifi_country_t country;
	atcmd_wifi_channels_t channels;
	atcmd_wifi_power_t txpower;

	atcmd_wifi_scan_t scan;
	atcmd_wifi_connect_t connect;
	atcmd_wifi_dhcp_t dhcp;
	atcmd_wifi_roaming_t roaming;
#ifndef CONFIG_ATCMD_WITHOUT_LWIP
	atcmd_wifi_ping_t ping;
#endif
	atcmd_wifi_softap_t softap;

	SemaphoreHandle_t mutex;
} atcmd_wifi_info_t;

/**********************************************************************************************/

#define ATCMD_MSG_WEVENT(fmt, ...)	\
		ATCMD_MSG_EVENT("WEVENT", fmt, ##__VA_ARGS__)

extern int atcmd_wifi_enable (void);
extern void atcmd_wifi_disable (void);

extern bool atcmd_wifi_lock (void);
extern bool atcmd_wifi_unlock (void);

extern void atcmd_wifi_sleep_send_event (void);

/**********************************************************************************************/
#endif /* #ifndef __NRC_ATCMD_WIFI_H__ */

