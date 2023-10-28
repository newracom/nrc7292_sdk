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

#ifndef __NRC_ATCMD_H__
#define __NRC_ATCMD_H__
/**********************************************************************************************/

#include "build_ver.h"
#include "nrc_sdk.h"


#define SDK_VER_MAJOR			(VERSION_MAJOR)
#define SDK_VER_MINOR			(VERSION_MINOR)
#define SDK_VER_REVISION		(VERSION_REVISION)
#if defined(VERSION_DESCRIPTION)
#define SDK_VER_DESCRIPTION		(VERSION_DESCRIPTION)
#endif

#define ATCMD_VER_MAJOR			(1)
#define ATCMD_VER_MINOR			(25)
#define ATCMD_VER_REVISION		(0)

/**********************************************************************************************/

#if defined(CONFIG_SAE) && defined(CONFIG_OWE)
#define CONFIG_ATCMD_WPA3
/* #define CONFIG_ATCMD_SOFTAP_WPA3 */
#endif

#ifndef CONFIG_ATCMD_CLI
/* #define CONFIG_ATCMD_CLI */
#ifndef CONFIG_ATCMD_CLI_MINIMUM
/* #define CONFIG_ATCMD_CLI_MINIMUM */
#endif
#endif

#ifndef CONFIG_ATCMD_IPV6
/* #define CONFIG_ATCMD_IPV6 */
#endif

#ifndef CONFIG_ATCMD_USER
/* #define CONFIG_ATCMD_USER */
#endif

/* #define CONFIG_ATCMD_PREFIX_CHECK */
/* #define CONFIG_ATCMD_TRXBUF_STATIC */
/* #define CONFIG_ATCMD_LOWERCASE */
/* #define CONFIG_ATCMD_HISTORY */
/* #define CONFIG_ATCMD_PROMPT */
#define CONFIG_ATCMD_SOFTAP

#if CONFIG_ATCMD_TASK_PRIORITY == 0
#undef CONFIG_ATCMD_TASK_PRIORITY
#define CONFIG_ATCMD_TASK_PRIORITY		NRC_TASK_PRIORITY
#endif

#define CONFIG_ATCMD_DATA_LEN_MAX		(4 * 1024)

/**********************************************************************************************/

#define ATCMD_NETIF_INDEX				0

#define ATCMD_MSG_LEN_MIN				2
#define ATCMD_MSG_LEN_MAX				128
#define ATCMD_MSG_PARAM_MAX				10

#define ATCMD_TASK_PRIORITY				CONFIG_ATCMD_TASK_PRIORITY
#define ATCMD_DATA_LEN_MAX				CONFIG_ATCMD_DATA_LEN_MAX

#define ATCMD_RXBUF_SIZE				ATCMD_DATA_LEN_MAX /* uplink */
#define ATCMD_TXBUF_SIZE				ATCMD_DATA_LEN_MAX /* donwlink */

#define ATCMD_IP4_ADDR_LEN_MIN			7	/* x.x.x.x */
#define ATCMD_IP4_ADDR_LEN_MAX			15	/* lwip/ip4_addr.h, (IP4ADDR_STRLEN_MAX - 1) */

#if defined(CONFIG_ATCMD_IPV6)
#define ATCMD_IP6_ADDR_LEN_MIN			2	/* :: */
#define ATCMD_IP6_ADDR_LEN_MAX			45	/* lwip/ip6_addr.h, (IP6ADDR_STRLEN_MAX - 1) */

#define ATCMD_IPADDR_LEN_MIN			ATCMD_IP6_ADDR_LEN_MIN
#define ATCMD_IPADDR_LEN_MAX			ATCMD_IP6_ADDR_LEN_MAX
#else
#define ATCMD_IPADDR_LEN_MIN			ATCMD_IP4_ADDR_LEN_MIN
#define ATCMD_IPADDR_LEN_MAX			ATCMD_IP4_ADDR_LEN_MAX
#endif

#define ATCMD_STR_SIZE(len)				(len + 1)
#define ATCMD_STR_PARAM_SIZE(len)		(1 + len + 1 + 1)

/**********************************************************************************************/

enum ATCMD_ERROR
{
	ATCMD_NO_RETURN = -1,

	ATCMD_SUCCESS = 0,

	ATCMD_ERROR_INVAL,
	ATCMD_ERROR_BUSY,
	ATCMD_ERROR_FAIL,
	ATCMD_ERROR_TIMEOUT,

	ATCMD_ERROR_NUM,

	ATCMD_ERROR_NOTSUPP = ATCMD_ERROR_NUM + 1
};

enum ATCMD_CFG
{
	ATCMD_CFG_MIN = 0,

	ATCMD_CFG_ECHO = ATCMD_CFG_MIN,
	ATCMD_CFG_LINEFEED,
	ATCMD_CFG_PROMPT,
#ifdef CONFIG_ATCMD_LOWERCASE
	ATCMD_CFG_LOWERCASE,
#endif
#ifdef CONFIG_ATCMD_HISTORY
	ATCMD_CFG_HISTORY,
#endif

	ATCMD_CFG_NUM,
	ATCMD_CFG_MAX = ATCMD_CFG_NUM - 1,

	ATCMD_CFG_LIMIT = (32 - 1)
};

enum ATCMD_TYPE
{
	ATCMD_TYPE_NONE = -1,

	ATCMD_TYPE_RUN = 0,		/* AT+CMD				*/
	ATCMD_TYPE_GET,			/* AT+CMD?				*/
	ATCMD_TYPE_GET_PARAM,	/* AT+CMD?=<parameters> */
	ATCMD_TYPE_SET,			/* AT+CMD=<parameters>  */
	ATCMD_TYPE_SET_HELP,	/* AT+CMD=?				*/

	ATCMD_TYPE_NUM

};

enum ATCMD_HANDLER
{
	ATCMD_HANDLER_NONE = -1,

	ATCMD_HANDLER_RUN = 0,	/* AT+CMD	*/
	ATCMD_HANDLER_GET,		/* AT+CMD?	*/
	ATCMD_HANDLER_SET,		/* AT+CMD=	*/

	ATCMD_HANDLER_NUM
};

enum ATCMD_GROUP_ID
{
	ATCMD_GROUP_BASIC = 0,	/* AT+	*/
	ATCMD_GROUP_WIFI,		/* AT+W	*/
	ATCMD_GROUP_SOCKET,		/* AT+S	*/
	ATCMD_GROUP_USER		/* AT+U	*/
};

enum ATCMD_ID
{
/* 	ATCMD_GROUP_BASIC
 *************************/
	ATCMD_BASIC_VERSION = 0,
	ATCMD_BASIC_HEAP,
	ATCMD_BASIC_UART,
	ATCMD_BASIC_GPIOCFG,
	ATCMD_BASIC_GPIOVAL,
	ATCMD_BASIC_ADC,
	ATCMD_BASIC_FW_UPDATE,
	ATCMD_BASIC_FW_DOWNLOAD,
	ATCMD_BASIC_TIMEOUT,

/* 	ATCMD_GROUP_WIFI
 *************************/
	ATCMD_WIFI_MACADDR = 100,
	ATCMD_WIFI_MACADDR0,
	ATCMD_WIFI_MACADDR1,
	ATCMD_WIFI_COUNTRY,
	ATCMD_WIFI_TXPOWER,
	ATCMD_WIFI_RXSIGNAL,
	ATCMD_WIFI_RATECTRL,
	ATCMD_WIFI_MCS,
	ATCMD_WIFI_DUTY_CYCLE,
	ATCMD_WIFI_CCA_THRESHOLD,
	ATCMD_WIFI_TX_TIME,
	ATCMD_WIFI_TSF,
	ATCMD_WIFI_BEACON_INTERVAL,
	ATCMD_WIFI_LISTEN_INTERVAL,
	ATCMD_WIFI_SCAN,
	ATCMD_WIFI_SCAN_SSID,
	ATCMD_WIFI_CONNECT,
	ATCMD_WIFI_DISCONNECT,
	ATCMD_WIFI_DHCP,
	ATCMD_WIFI_DHCPS,
	ATCMD_WIFI_IPADDR,
	ATCMD_WIFI_PING,
#ifdef CONFIG_ATCMD_IPV6
	ATCMD_WIFI_IPADDR6,
	ATCMD_WIFI_PING6,
#endif
	ATCMD_WIFI_DNS,
	ATCMD_WIFI_FOTA,
	ATCMD_WIFI_DEEP_SLEEP,
	ATCMD_WIFI_SOFTAP,
	ATCMD_WIFI_SOFTAP_SSID,
	ATCMD_WIFI_BSS_MAX_IDLE,
	ATCMD_WIFI_STA_INFO,
	ATCMD_WIFI_MAX_STA,
	ATCMD_WIFI_TIMEOUT,

	ATCMD_WIFI_CONTINUOUS_TX,

	/* Command for internal */
#if defined(CONFIG_ATCMD_WIFI_INTERNAL)
	ATCMD_WIFI_RF_CAL,
	ATCMD_WIFI_LBT,
	ATCMD_WIFI_MIC_SCAN,
	ATCMD_WIFI_BMT,
#endif	

/* 	ATCMD_GROUP_SOCKET
 *************************/
	ATCMD_SOCKET_OPEN = 200,
#ifdef CONFIG_ATCMD_IPV6
	ATCMD_SOCKET_OPEN6,
#endif
	ATCMD_SOCKET_CLOSE,
	ATCMD_SOCKET_LIST,
	ATCMD_SOCKET_SEND,
	ATCMD_SOCKET_RECV,
	ATCMD_SOCKET_RECV_MODE,
	ATCMD_SOCKET_RECV_INFO,
	ATCMD_SOCKET_RECV_LOG,
	ATCMD_SOCKET_ADDR_INFO,
	ATCMD_SOCKET_TCP_KEEPALIVE,
	ATCMD_SOCKET_TCP_NODELAY,
	ATCMD_SOCKET_TIMEOUT,

	/* Command for internal */
#if defined(CONFIG_ATCMD_SOCKET_INTERNAL)
	ATCMD_SOCKET_SEND_MODE,
	ATCMD_SOCKET_SEND_DONE,
	ATCMD_SOCKET_SEND_EXIT,
#endif	

/* 	ATCMD_GROUP_USER
 *************************/
	ATCMD_USER_MIN = 300,
	ATCMD_USER_MAX = ATCMD_USER_MIN + (100 - 1),
};

enum ATCMD_KEY
{
	ATCMD_KEY_RESET = 0,
	ATCMD_KEY_IGNORE,
	ATCMD_KEY_PRINT,

	ATCMD_KEY_BS,
	ATCMD_KEY_CR,
	ATCMD_KEY_LF,
	ATCMD_KEY_ESC,

	ATCMD_KEY_UP,
	ATCMD_KEY_DOWN,
	ATCMD_KEY_RIGHT,
	ATCMD_KEY_LEFT,

	ATCMD_KEY_HOME,
	ATCMD_KEY_INS,
	ATCMD_KEY_DEL,
	ATCMD_KEY_END,
	ATCMD_KEY_PGUP,
	ATCMD_KEY_PGDN,
};

/**********************************************************************************************/

typedef struct
{
	int cnt;
	char cmd[ATCMD_MSG_LEN_MAX + 1];
} atcmd_buf_t;

typedef struct atcmd_list
{
	struct atcmd_list *next, *prev;
} atcmd_list_t;

typedef struct
{
	atcmd_list_t list;

	const char *name;
	enum ATCMD_GROUP_ID id;

	const char *cmd_prefix;
	int cmd_prefix_size;

	atcmd_list_t cmd_list_head;
} atcmd_group_t;

typedef int (*atcmd_handler_t) (int argc, char *argv[]);

typedef struct
{
	atcmd_list_t list;

	enum ATCMD_GROUP_ID group;

	int id;
	const char *cmd;

	atcmd_handler_t handler[ATCMD_HANDLER_NUM];
} atcmd_info_t;

/**********************************************************************************************/

enum ATCMD_MSG_TYPE
{
	ATCMD_MSG_TYPE_RETURN = 0,
	ATCMD_MSG_TYPE_INFO,
	ATCMD_MSG_TYPE_EVENT,
	ATCMD_MSG_TYPE_DEBUG,
	ATCMD_MSG_TYPE_HELP,

	ATCMD_MSG_TYPE_MAX,
};

extern int atcmd_msg_print (int type, const char *fmt, ...);
extern int atcmd_msg_snprint (int type, char *buf, int len, const char *fmt, ...);
extern int atcmd_msg_vsnprint (int type, char *buf, int len, const char *fmt, va_list ap);

#define ATCMD_MSG_RETURN(cmd, ret)	atcmd_transmit_return(cmd, ret)

#define ATCMD_MSG_HELP(fmt, ...)	\
		atcmd_msg_print(ATCMD_MSG_TYPE_HELP, fmt, ##__VA_ARGS__)

#define ATCMD_MSG_INFO(cmd, fmt, ...)	\
		atcmd_msg_print(ATCMD_MSG_TYPE_INFO, cmd ":" fmt, ##__VA_ARGS__)

#define ATCMD_MSG_EVENT(event, fmt, ...)	\
		atcmd_msg_print(ATCMD_MSG_TYPE_EVENT, event ":" fmt, ##__VA_ARGS__)

/**********************************************************************************************/

#include "hif.h"

#include "atcmd_basic.h"
#include "atcmd_wifi.h"
#include "atcmd_socket.h"
#include "atcmd_user.h"

/**********************************************************************************************/

inline uint32_t atcmd_sys_now (void)
{
	return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

extern int atcmd_param_to_int8 (const char *param, int8_t *val);
extern int atcmd_param_to_int16 (const char *param, int16_t *val);
extern int atcmd_param_to_int32 (const char *param, int32_t *val);
extern int atcmd_param_to_uint8 (const char *param, uint8_t *val);
extern int atcmd_param_to_uint16 (const char *param, uint16_t *val);
extern int atcmd_param_to_uint32 (const char *param, uint32_t *val);
extern int atcmd_param_to_float (const char *param, float *val);
extern int atcmd_param_to_hex (const char *param, uint32_t *val);
extern char *atcmd_param_to_str (const char *param, char *str, int len);
extern char *atcmd_str_to_param (const char *str, char *param, int len);

extern int atcmd_data_mode_enable (atcmd_socket_t *socket, int32_t len, bool done_event, uint32_t timeout, char *exit_cmd);
extern int atcmd_firmware_download_enable (int32_t len, uint32_t timeout);

extern void atcmd_group_print (void);
extern atcmd_group_t *atcmd_group_search (enum ATCMD_GROUP_ID id);
extern int atcmd_group_register (atcmd_group_t *group);
extern int atcmd_group_unregister (enum ATCMD_GROUP_ID id);

extern void atcmd_info_print (atcmd_group_t *group);
extern atcmd_info_t *atcmd_search (atcmd_group_t *group, enum ATCMD_ID id);
extern int atcmd_info_register (enum ATCMD_GROUP_ID gid, atcmd_info_t *info);
extern void atcmd_info_unregister (enum ATCMD_GROUP_ID gid, enum ATCMD_ID id);

extern void atcmd_receive (char *data, int size);
extern int atcmd_receive_command (char *data, int size);

extern int atcmd_transmit (char *data, int size);
extern int atcmd_transmit_return (char *cmd, int ret);

extern int atcmd_enable (_hif_info_t *info);
extern void atcmd_disable (void);

extern int atcmd_user_register (const char *cmd, int id, atcmd_handler_t handler[]);
extern int atcmd_user_unregister (int id);

/**********************************************************************************************/

#define _atcmd_malloc		pvPortMalloc
#define _atcmd_free			vPortFree
#define _atcmd_printf		hal_uart_printf

extern int atcmd_log (const char *fmt, ...);

#define _atcmd_info(fmt, ...)		atcmd_log(fmt, ##__VA_ARGS__)
#define _atcmd_error(fmt, ...)		atcmd_log("(%s,%d) " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define _atcmd_debug(fmt, ...)		atcmd_log(fmt, ##__VA_ARGS__)
#define _atcmd_trace()				atcmd_log("%s,%d", __func__, __LINE__)

/**********************************************************************************************/
#endif /* #ifndef __NRC_ATCMD_H__ */
