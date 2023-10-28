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

#define MAX_SCAN_RESULTS				(30)
#define MAX_SSID_LENGTH					(32)
#define MAX_BSSID_LENGTH				(17)
#define MAX_CC_LENGTH					(3)
#define MAX_STATIC_IP_LENGTH			(48)
#define MAX_PW_LENGTH					(64)
#define MAX_PMK_LENGTH                  (64)
#define MAX_MAC_ADDR					(17)

#ifdef MAX_STA
#define MAX_STA_CONN_NUM				MAX_STA
#else
#define MAX_STA_CONN_NUM				10
#endif

typedef enum {
	NRC_SUCCESS = 0,
	NRC_FAIL = -1
} nrc_err_t;

/** @brief Wi-Fi state */
typedef enum {
	WIFI_STATE_UNKNOWN = -1,

	WIFI_STATE_INIT = 0,
	WIFI_STATE_CONNECTED,
	WIFI_STATE_DISCONNECTED,
	WIFI_STATE_TRY_CONNECT,
	WIFI_STATE_TRY_DISCONNECT,
	WIFI_STATE_SOFTAP_CONFIGURED,
	WIFI_STATE_SOFTAP_TRY_START,
	WIFI_STATE_SOFTAP_START,
	WIFI_STATE_DHCPS_START,

	WIFI_STATE_MAX,
} tWIFI_STATE_ID;

/** @brief Wi-Fi event type */
typedef enum {
	WIFI_EVT_SCAN = 0,
	WIFI_EVT_SCAN_DONE,
	WIFI_EVT_CONNECT_SUCCESS,
	WIFI_EVT_DISCONNECT,
	WIFI_EVT_AP_STARTED,
	WIFI_EVT_VENDOR_IE,
	WIFI_EVT_AP_STA_CONNECTED,
	WIFI_EVT_AP_STA_DISCONNECTED,
	WIFI_EVT_ASSOC_REJECT,

	WIFI_EVT_MAX,
} tWIFI_EVENT_ID;

/** @brief Network address status */
typedef enum {
	NET_ADDR_NOT_SET = 0,
	NET_ADDR_DHCP_STARTED,
	NET_ADDR_SET,
} tNET_ADDR_STATUS;

typedef void (*event_callback_fn)(int vif, tWIFI_EVENT_ID event, int data_len, void *data);

typedef struct event_callback_list {
	event_callback_fn fn;
	struct event_callback_list* next;
}event_callback_list_t;

/** @brief Wi-Fi security mode */
typedef enum {
	WIFI_SEC_OPEN = 0,
	WIFI_SEC_WPA2,
	WIFI_SEC_WPA3_OWE,
	WIFI_SEC_WPA3_SAE, /* SoftAP cannot support WPA3-SAE */

	WIFI_SEC_MAX,
} tWIFI_SECURITY;

/** @brief Wi-Fi API operation status	*/
typedef enum {
	WIFI_SUCCESS			= 0,
	WIFI_NOMEM				= -1,

	WIFI_INVALID			= -5,
	WIFI_INVALID_STATE		= (WIFI_INVALID - 1),

	WIFI_TIMEOUT			= -10,
	WIFI_TIMEOUT_DHCP		= (WIFI_TIMEOUT - 1),

	WIFI_FAIL 				= -15,
	WIFI_FAIL_INIT 			= (WIFI_FAIL - 1),
	WIFI_FAIL_CONNECT		= (WIFI_FAIL - 2),
	WIFI_FAIL_DHCP			= (WIFI_FAIL - 3),
	WIFI_FAIL_SET_IP		= (WIFI_FAIL - 5),
	WIFI_FAIL_SOFTAP		= (WIFI_FAIL - 6),
	WIFI_FAIL_SOFTAP_NOSTA		= (WIFI_FAIL - 7),
}tWIFI_STATUS;

/** @brief IP mode	*/
typedef enum {
	WIFI_STATIC_IP= 0,
	WIFI_DYNAMIC_IP
}tWIFI_IP_MODE;

/** @brief Tx Power type	*/
typedef enum {
	WIFI_TXPOWER_AUTO= 0,
	WIFI_TXPOWER_LIMIT,
	WIFI_TXPOWER_FIXED
}tWIFI_TXPOWER_TYPE;

/** @brief Wi-Fi country code 	*/
typedef enum {
	WIFI_CC_UNKNOWN = -1,

	WIFI_CC_US = 0,
	WIFI_CC_JP,
	WIFI_CC_K1,
	WIFI_CC_TW,
	WIFI_CC_EU,
	WIFI_CC_CN,
	WIFI_CC_NZ,
	WIFI_CC_AU,
	WIFI_CC_K2,

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

	WIFI_MODE_MAX
}tWIFI_DEVICE_MODE;

typedef enum {
	WIFI_NETWORK_MODE_BRIDGE = 0,
	WIFI_NETWORK_MODE_NAT,

	WIFI_NETWORK_MODE_MAX
}tWIFI_NETWORK_MODE;

/** @brief Wi-Fi scan mode */
typedef enum {
	WIFI_SCAN_NORMAL = 0,
	WIFI_SCAN_PASSIVE,
	WIFI_SCAN_FAST,
	WIFI_SCAN_FAST_PASSIVE,

	WIFI_SCAN_MAX,
} tWIFI_SCAN;

/** @brief Wi-Fi STA state 	*/
typedef enum {
	WIFI_STA_INVALID = 0xFF,
	WIFI_STA_AUTH = 0,
	WIFI_STA_ASSOC = 1,
} tWIFI_STA_STATE;

/** @brief Wi-Fi TID (Traffic Identifier) */
typedef enum {
	WIFI_TID_BE,
	WIFI_TID_BK,
	WIFI_TID_VI,
	WIFI_TID_VO,
} tWIFI_TID;

/** @brief Wi-Fi Guard interval type */
typedef enum {
	WIFI_GI_UNKNOWN,
	WIFI_GI_LONG,
	WIFI_GI_SHORT,
} tWIFI_GI;

/** @brief Wi-Fi ignore broadcast SSID */
typedef enum {
	WIFI_IGNORE_BROADCAST_SSID_FULL,
	WIFI_IGNORE_BROADCAST_SSID_EMPTY,
	WIFI_IGNORE_BROADCAST_SSID_CLEAR,
} tWIFI_IGNORE_BROADCAST_SSID;

/** @brief scan item */
typedef union {
	char *items[5];
	struct {
		char *bssid;
		char *freq;
		char *sig_level;
		char *flags;
		char *ssid;
		tWIFI_SECURITY security;
	};
} SCAN_RESULT;

/** @brief scan list */
typedef struct {
	int n_result;
	SCAN_RESULT result[MAX_SCAN_RESULTS];
} SCAN_RESULTS;

/** @brief ap info for STA */
typedef struct {
	uint8_t bssid[6];
	uint8_t ssid[32];
	uint8_t ssid_len;
	uint8_t cc[2];
	uint16_t ch;
	uint16_t freq;
	tWIFI_BANDWIDTH bw;
	tWIFI_SECURITY security;
} AP_INFO;

/** @brief sta info for AP */
typedef struct {
	tWIFI_STA_STATE state;
	int8_t rssi;
	uint8_t snr;
	uint8_t tx_mcs;
	uint8_t rx_mcs;
	uint16_t aid;
	uint8_t addr[6];
} STA_INFO;

typedef struct {
	uint16_t total_num;
	STA_INFO sta[MAX_STA_CONN_NUM];
} STA_LIST;

typedef struct {
	uint8_t major;
	uint8_t minor;
	uint8_t patch;
} VERSION_T;

typedef void (*intr_handler_fn)(int vector);

#endif /* __NRC_TYPES_H__ */
