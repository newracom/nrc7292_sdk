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

#ifndef __NRC_TYPES_H__
#define __NRC_TYPES_H__

// Type Definition
typedef signed char                 int8_t;
typedef unsigned char               uint8_t;
typedef signed short                int16_t;
typedef unsigned short              uint16_t;
typedef signed long                 int32_t;
typedef unsigned long               uint32_t;
typedef signed long long            int64_t;
typedef unsigned long long          uint64_t;

typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef unsigned int u32;
typedef int64_t s64;
typedef uint64_t u64;

#define SCAN_RESULT_ITEMS_NUM			5
#define SCAN_RESULT_BUFFER_SIZE			512
#define MAX_SCAN_RESULTS				10
#define MAX_SSID_LENGTH 32
#define MAX_BSSID_LENGTH 17
#define MAX_CC_LENGTH 3
#define MAX_FREQ_NUM 50
#define MAX_STATIC_IP_LENGTH 16
#define MAX_PW_LENGTH 30
#define MAX_MAC_ADDR 17
#define MAX_WPA_RESPONSE_BUFFER_SIZE 1024
#define WIFI_CONNECTION_TIMEOUT 60 // sec

typedef enum {
	NRC_SUCCESS = 0,
	NRC_FAIL = -1
} nrc_err_t;

/** @brief Wi-Fi state */
typedef enum {
	WIFI_STATE_INIT = 0,
	WIFI_STATE_READY,
	WIFI_STATE_TRY_CONNECT,
	WIFI_STATE_CONNECTED,
	WIFI_STATE_TRY_GET_IP,
	WIFI_STATE_GET_IP,
	WIFI_STATE_TRY_DISCONNECT,
	WIFI_STATE_DISCONNECTED,
	WIFI_STATE_SOFTAP_CONF,
	WIFI_STATE_SOFTAP_START,
	WIFI_STATE_DHCPS_START,
	WIFI_STATE_TRY_DISASSOC,
	WIFI_STATE_SCAN,
	WIFI_STATE_SCAN_DONE,
	WIFI_STATE_MAX,
} tWIFI_STATE_ID;

/** @brief Wi-Fi event type */
typedef enum {
	WIFI_EVT_CONNECT_SUCCESS =0,
	WIFI_EVT_CONNECT_FAIL,
	WIFI_EVT_GET_IP,
	WIFI_EVT_GET_IP_FAIL,
	WIFI_EVT_DISCONNECT,
	WIFI_EVT_START_SOFT_AP,
	WIFI_EVT_SET_SOFT_AP_IP,
	WIFI_EVT_START_DHCP_SERVER,
	WIFI_EVT_SCAN,
	WIFI_EVT_SCAN_DONE,
	WIFI_EVT_VENDOR_IE,
	WIFI_EVT_MAX,
} tWIFI_EVENT_ID;

typedef void (*event_callback_fn)(tWIFI_EVENT_ID event, int data_len, char* data);

/** @brief Wi-Fi security mode */
typedef enum {
	WIFI_SEC_OPEN	= 0,
	WIFI_SEC_WPA2	= 1,
	WIFI_SEC_WPA3_OWE	= 2,
	WIFI_SEC_WPA3_SAE	= 3,
	WIFI_SEC_MAX,
} tWIFI_SECURITY;

/** @brief Wi-Fi API operation status	*/
typedef enum {
	WIFI_SUCCESS				= 0,
	WIFI_FAIL 				= -1,
	WIFI_NOMEM					= -2,
	WIFI_INVALID				= -3,

	WIFI_INIT_FAIL 				= (WIFI_INVALID - 1),
	WIFI_CONNECTION_FAIL		= (WIFI_INVALID - 2),
	WIFI_DHCP_FAIL				= (WIFI_INVALID - 3),
	WIFI_DHCP_TIMEOUT			= (WIFI_INVALID - 4),
	WIFI_SET_IP_FAIL			= (WIFI_INVALID - 5),
	WIFI_SOFTAP_FAIL			= (WIFI_INVALID - 6),
}tWIFI_STATUS;

/** @brief IP mode	*/
typedef enum {
	WIFI_STATIC_IP= 0,
	WIFI_DYNAMIC_IP
}tWIFI_IP_MODE;

/** @brief Wi-Fi country code 	*/
typedef enum {
	WIFI_CC_UNKNOWN = -1,
	WIFI_CC_JP = 0,
	WIFI_CC_KR,
	WIFI_CC_TW,
	WIFI_CC_US,
	WIFI_CC_EU,
	WIFI_CC_CN,
	WIFI_CC_NZ,
	WIFI_CC_AU,
	WIFI_CC_MAX
}tWIFI_COUNTRY_CODE;

/** @brief Wi-Fi bandwidth 	*/
typedef enum {
	WIFI_1M = 1,
	WIFI_2M = 2,
	WIFI_4M = 4
}tWIFI_BANDWIDTH;

/** @brief Wi-Fi device mode 	*/
typedef enum {
	WIFI_MODE_STATION = 0,
	WIFI_MODE_AP,
	WIFI_MODE_MESH_POINT
}tWIFI_DEVICE_MODE;

/** @brief vendor event 	*/
typedef enum {
	VEVENT_VENDOR_IE_CMD_0 = 0xF0,
	VEVENT_VENDOR_IE_CMD_1,
	VEVENT_VENDOR_IE_CMD_2,
	VEVENT_VENDOR_IE_CMD_3,
	VEVENT_VENDOR_IE_CMD_4,
} nrc_vendor_event_t;

#define VENDOR_IE_CMD_MAX ((VEVENT_VENDOR_IE_CMD_4 - VEVENT_VENDOR_IE_CMD_0) + 1)

/** @brief scan item */
typedef union
{
	char *items[SCAN_RESULT_ITEMS_NUM];
	struct {
		char *bssid;
		char *freq;
		char *sig_level;
		char *flags;
		char *ssid;
	};
} SCAN_RESULT;

/** @brief scan list */
typedef struct
{
	int n_result;
	SCAN_RESULT result[MAX_SCAN_RESULTS];
} SCAN_RESULTS;

/** @brief wifi info list */
typedef struct
{
	int vif_id;
        int tx_pwr;
	int frequency;
	int mcs;
	char device_mode[11];
	char mac_addr[18];
	char country[3];
	char bandwidth[3];
	char rate_control[5];
	char security[7];
	char rate_bw[24];
	char bssid[18];
	char aid[30];

} WIFI_INFO;

typedef struct _s1g_operation_channel_table{
	uint8_t     country[3];
	uint16_t    s1g_freq;
} s1g_operation_channel_mapping;

typedef enum {
	WLAN_ASYNC,
	WLAN_SYNC,
	WLAN_CONN_MAX,
} tWLAN_CONN_MODE;

typedef enum {
	WLAN_CMD,
	WLAN_EVENT,
	WLAN_MAX,
} tWLAN_MESSAGE_TYPE;

typedef enum {
	WLAN_CMD_INIT = 0,
	WLAN_CMD_REGI_CB,
	WLAN_CMD_ADD_NETWORK,
	WLAN_CMD_REMOVE_NETWORK,
	WLAN_CMD_SET_COUNTRY,
	WLAN_CMD_SET_SSID,
	WLAN_CMD_SET_BSSID,
	WLAN_CMD_SET_SECURITY,
	WLAN_CMD_SET_SOFTAP_CONF,
	WLAN_CMD_SET_S1G_CONF,
	WLAN_CMD_SET_TX_POWER,
	WLAN_CMD_SET_DISASSOC,
	WLAN_CMD_START_SOFTAP,
	WLAN_CMD_CONNECT,
	WLAN_CMD_DISCONNECT,
	WLAN_CMD_DHCPC,
	WLAN_CMD_DHCPS,
	WLAN_CMD_SCAN,
	WLAN_CMD_ABORT_SCAN,
	WLAN_CMD_SCAN_RESULTS,
	WLAN_CMD_SET_SCAN_FREQ,
	WLAN_CMD_GET_SCAN_FREQ,
	WLAN_CMD_WPS_PBC,
	WLAN_CMD_SET_BSS_MAX_IDLE,
	WLAN_CMD_MAX,
} tWLAN_CMD_ID;

typedef struct  {
	uint16_t type;
	uint16_t len;
	void *value;
} WLAN_MESSAGE;
#define WLAN_MESSAGE_SIZE	sizeof (WLAN_MESSAGE)

typedef struct  {
	uint16_t ret_len;
	char  ret[MAX_WPA_RESPONSE_BUFFER_SIZE];
} WLAN_SUPPLICANT_RET;
#define WLAN_SUP_RET_SIZE	sizeof (WLAN_SUPPLICANT_RET)

typedef struct  {
	int id;
	uint8_t* data;
	uint16_t len;
} MSG_ID_FORMAT;
#define WLAN_ID_FM_SIZE		sizeof (MSG_ID_FORMAT)

typedef struct  {
	int id;
	int nd;
} MSG_NETWORK_ID_FORMAT;
#define WLAN_NETWORK_ID_SIZE	sizeof (MSG_NETWORK_ID_FORMAT)

typedef struct  {
	int id;
	int nd;
	int tx_power;
} MSG_TXPOWER_FORMAT;
#define WLAN_TXPOWER_SIZE	sizeof (MSG_TXPOWER_FORMAT)

typedef struct  {
	int id;
	int nd;
	bool ndp_preq;
	int mode;
} MSG_CONNECT_FORMAT;
#define WLAN_CONNECT_FM_SIZE	sizeof (MSG_CONNECT_FORMAT)


typedef struct  {
	int id;
	int nd;
	uint8_t ssid[MAX_SSID_LENGTH + 1];
} MSG_SSID_FORMAT;
#define WLAN_SSID_FM_SIZE	sizeof (MSG_SSID_FORMAT)


typedef struct  {
	int id;
	int nd;
	uint8_t bssid[MAX_BSSID_LENGTH + 1];
} MSG_BSSID_FORMAT;
#define WLAN_BSSID_FM_SIZE	sizeof (MSG_BSSID_FORMAT)


typedef struct  {
	int id;
	int nd;
	uint8_t country[MAX_CC_LENGTH];
} MSG_COUNTRY_CODE_FORMAT;
#define WLAN_CC_FM_SIZE	sizeof (MSG_COUNTRY_CODE_FORMAT)


typedef struct  {
	int id;
	int nd;
	int num_scan_freq;
	uint16_t *scan_freq_list;
} MSG_FREQ_FORMAT;
#define WLAN_FREQ_FM_SIZE	sizeof (MSG_FREQ_FORMAT)

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

typedef struct  {
	int id;
	int nd;
	int mode;
	uint8_t mac_addr[MAX_MAC_ADDR + 1];
} MSG_DISASSOC_FORMAT;
#define WLAN_DISASSOC_FM_SIZE	sizeof (MSG_DISASSOC_FORMAT)

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
	uint16_t s1g_channel;
} MSG_S1G_CONF_FORMAT;
#define WLAN_S1G_CONF_SIZE	sizeof (MSG_S1G_CONF_FORMAT)

typedef struct  {
	int id;
	int nd;
	int period;
	int retry_cnt;
} MSG_SOFTAP_BSS_MAX_IDLE_FORMAT;
#define WLAN_SOFTAP_BSS_MAX_IDLE_SIZE	sizeof (MSG_SOFTAP_BSS_MAX_IDLE_FORMAT)

typedef void (*intr_handler_fn)(int vector);

typedef void (*vendor_ie_event_callback_fn)(int data_len, char* data);

typedef struct _vendor_ie {
	vendor_ie_event_callback_fn func;
} vendor_ie_t;

#endif /* __NRC_TYPES_H__ */
