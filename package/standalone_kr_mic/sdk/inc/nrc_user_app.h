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

#ifndef __NRC_USER_APP_H__
#define __NRC_USER_APP_H__

typedef enum {
	WLAN_ASYNC,
	WLAN_SYNC,
	WLAN_CONN_MAX,
} WLAN_CONN_MODE;

typedef enum {
	WLAN_CMD,
	WLAN_EVENT,
	WLAN_MAX,
} WLAN_MESSAGE_TYPE;

typedef enum {
	WLAN_CMD_INIT = 0,
	WLAN_CMD_REGI_CB,
	WLAN_CMD_GET_ND,
	WLAN_CMD_SET_COUNTRY,
	WLAN_CMD_SET_SSID,
	WLAN_CMD_SET_SECURITY,
	WLAN_CMD_SET_SOFTAP_CONF,
	WLAN_CMD_SET_S1G_CONF,
	WLAN_CMD_SET_TX_POWER,
	WLAN_CMD_START_SOFTAP,
	WLAN_CMD_CONNECT,
	WLAN_CMD_DISCONNECT,
	WLAN_CMD_DHCPC,
	WLAN_CMD_DHCPS,
	WLAN_CMD_SCAN,
	WLAN_CMD_SCAN_RESULTS,
	WLAN_CMD_MAX,
} WLAN_CMD_ID;

typedef enum {
	WLAN_EVT_CONNECT_SUCCESS =0,
	WLAN_EVT_CONNECT_FAIL,
	WLAN_EVT_GET_IP,
	WLAN_EVT_GET_IP_FAIL,
	WLAN_EVT_DISCONNECT,
	WLAN_EVT_START_SOFT_AP,
	WLAN_EVT_SET_SOFT_AP_IP,
	WLAN_EVT_START_DHCP_SERVER,
	WLAN_EVT_SCAN_DONE,
	WLAN_EVT_MAX,
} WLAN_EVENT_ID;

typedef enum {
	WLAN_STATE_INIT = 0,
	WLAN_STATE_READY,
	WLAN_STATE_TRY_CONNECT,
	WLAN_STATE_CONNECTED,
	WLAN_STATE_TRY_GET_IP,
	WLAN_STATE_GET_IP,
	WLAN_STATE_TRY_DISCONNECT,
	WLAN_STATE_DISCONNECTED,
	WLAN_STATE_SOFTAP_CONF,
	WLAN_STATE_SOFTAP_START,
	WLAN_STATE_DHCPS_START,
	WLAN_STATE_MAX,
} WLAN_STATE_ID;

typedef struct  {
	uint16_t type;
	uint16_t len;
	void *value;
} WLAN_MESSAGE;
#define WLAN_MESSAGE_SIZE	sizeof (WLAN_MESSAGE)

typedef struct  {
	uint16_t ret_len;
	char  ret[512];
} WLAN_SUPPLICANT_RET;
#define WLAN_SUP_RET_SIZE	sizeof (WLAN_SUPPLICANT_RET)

typedef struct  {
	int id;
} MSG_ID_FORMAT;
#define WLAN_ID_FM_SIZE		sizeof (MSG_ID_FORMAT)

#define MAX_SSID_LENGTH 20
#define MAX_PW_LENGTH 30
typedef struct  {
	int id;
	int nd;
	uint8_t ssid[MAX_SSID_LENGTH];
	int s1g_freq;
	int sec_mode;
	uint8_t password[MAX_PW_LENGTH];
} MSG_SOFTAP_CONF_FORMAT;
#define WLAN_SOFTAP_CONF_SIZE	sizeof (MSG_SOFTAP_CONF_FORMAT)

typedef struct  {
	int id;
	int nd;
	int tx_power;
} MSG_TXPOWER_FORMAT;
#define WLAN_TXPOWER_SIZE	sizeof (MSG_TXPOWER_FORMAT)

typedef struct  {
	int id;
	int nd;
	int mode;
} MSG_CONNECT_FORMAT;
#define WLAN_CONNECT_FM_SIZE	sizeof (MSG_CONNECT_FORMAT)

typedef struct  {
	int id;
	int nd;
	uint8_t ssid[MAX_SSID_LENGTH];
} MSG_SSID_FORMAT;
#define WLAN_SSID_FM_SIZE	sizeof (MSG_SSID_FORMAT)

#define MAX_CC_LENGTH 3
typedef struct  {
	int id;
	int nd;
	uint8_t country[MAX_CC_LENGTH];
} MSG_COUNTRY_CODE_FORMAT;
#define WLAN_CC_FM_SIZE	sizeof (MSG_COUNTRY_CODE_FORMAT)

typedef void (*event_callback_fn)(WLAN_EVENT_ID event);
typedef struct  {
	int id;
	event_callback_fn function;
} MSG_CB_FUNCTION_FORMAT;
#define WLAN_CB_FUNC_FM_SIZE	sizeof (MSG_CB_FUNCTION_FORMAT)

typedef struct  {
	int id;
	int nd;
	int mode;
	uint8_t password[MAX_PW_LENGTH];
} MSG_SECURITY_FORMAT;
#define WLAN_SECURITY_FM_SIZE	sizeof (MSG_SECURITY_FORMAT)

#define MAX_STATIC_IP_LENGTH 16
typedef struct  {
	uint8_t ssid[MAX_SSID_LENGTH];
	uint8_t country[MAX_CC_LENGTH];
	uint8_t security_mode;
	uint8_t password[MAX_PW_LENGTH];
	uint8_t count;
	int interval;
	uint8_t dhcp_enable;
	uint8_t static_ip[MAX_STATIC_IP_LENGTH];
	uint8_t ap_ip[MAX_STATIC_IP_LENGTH];
	uint8_t remote_addr[MAX_STATIC_IP_LENGTH];
	uint8_t s1g_channel;
	int tx_power;
	uint8_t dhcp_server;
	int test_running;
}WIFI_CONFIG;
#define WIFI_CONFIG_SIZE	sizeof (WIFI_CONFIG)

typedef struct _s1g_operation_channel_table{
	uint8_t     country[MAX_CC_LENGTH];
	uint16_t    s1g_freq;
} s1g_operation_channel_mapping;

#define RUN_FAIL -1
#define RUN_SUCCESS 0

extern WLAN_SUPPLICANT_RET x_sup_ret;
#define SUP_RET				x_sup_ret.ret
#define SUP_RET_LEN			x_sup_ret.ret_len
#define CMD_BUFFER_LEN 512
int nrc_user_app_main();

#endif
