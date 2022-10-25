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

#ifndef __NRC_ATCMD_H__
#define __NRC_ATCMD_H__
/**********************************************************************************************/

#include "build_ver.h"
#include "nrc_sdk.h"
#include "hif.h"

#define SDK_VER_MAJOR			(VERSION_MAJOR)
#define SDK_VER_MINOR			(VERSION_MINOR)
#define SDK_VER_REVISION		(VERSION_REVISION)

#define ATCMD_VER_MAJOR			(1)
#define ATCMD_VER_MINOR			(22)
#define ATCMD_VER_REVISION		(7)

/**********************************************************************************************/
#if defined(CONFIG_SAE) && defined(CONFIG_OWE)
#define CONFIG_ATCMD_WPA3
#endif

#ifndef CONFIG_ATCMD_USER
/* #define CONFIG_ATCMD_USER */
#endif

#ifndef CONFIG_ATCMD_DEBUG
/* #define CONFIG_ATCMD_DEBUG */
#endif

/* #define CONFIG_ATCMD_PREFIX_CHECK */
/* #define CONFIG_ATCMD_TRXBUF_STATIC */

#define CONFIG_ATCMD_XPUT_IMPROVEMENT
#ifdef CONFIG_ATCMD_XPUT_IMPROVEMENT
/* #define CONFIG_ATCMD_SOCKET_EVENT_SEND */
#endif

#if CONFIG_ATCMD_TASK_PRIORITY == 0
#undef CONFIG_ATCMD_TASK_PRIORITY
#define CONFIG_ATCMD_TASK_PRIORITY		NRC_TASK_PRIORITY
#endif

#define CONFIG_ATCMD_DATA_LEN_MAX		(4 * 1024)

/**********************************************************************************************/

#define _atcmd_malloc					pvPortMalloc
#define _atcmd_free						vPortFree
#define _atcmd_printf					hal_uart_printf

#define _atcmd_log(fmt, ...)			_atcmd_printf("[ATCMD] " fmt, ##__VA_ARGS__)

#define _atcmd_debug(fmt, ...)			_atcmd_log(fmt, ##__VA_ARGS__)
#define _atcmd_trace()					_atcmd_log("%s::%d\r\n", __func__, __LINE__)

#define _atcmd_info(fmt, ...)			_atcmd_log(fmt, ##__VA_ARGS__)
#define _atcmd_error(fmt, ...)			_atcmd_log("%s,%d: " fmt, __func__, __LINE__, ##__VA_ARGS__)

/**********************************************************************************************/

#define ATCMD_MSG_LEN_MIN				2
#define ATCMD_MSG_LEN_MAX				128
#define ATCMD_MSG_PARAM_MAX				10

#define ATCMD_TASK_PRIORITY				CONFIG_ATCMD_TASK_PRIORITY
#define ATCMD_DATA_LEN_MAX				CONFIG_ATCMD_DATA_LEN_MAX

#define ATCMD_RXBUF_SIZE				ATCMD_DATA_LEN_MAX /* uplink */
#if defined(NRC7292)
#define ATCMD_TXBUF_SIZE				ATCMD_DATA_LEN_MAX /* donwlink */
#else
#define ATCMD_TXBUF_SIZE				(ATCMD_DATA_LEN_MAX / 2) /* donwlink */
#endif

#define ATCMD_IP4_ADDR_LEN_MIN			7
#define ATCMD_IP4_ADDR_LEN_MAX			15

#define ATCMD_STR_SIZE(len)				(len + 1)
#define ATCMD_STR_PARAM_SIZE(len)		(1 + len + 1 + 1)

/**********************************************************************************************/

enum ATCMD_ERROR
{
	ATCMD_SEND_DATA = -1,

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

	ATCMD_CFG_PROMPT = ATCMD_CFG_MIN,
	ATCMD_CFG_ECHO,
	ATCMD_CFG_HISTORY,
	ATCMD_CFG_LOWERCASE,
	ATCMD_CFG_LINEFEED,

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
	ATCMD_BASIC_UART,
	ATCMD_BASIC_GPIOCFG,
	ATCMD_BASIC_GPIOVAL,
	ATCMD_BASIC_ADC,
	ATCMD_BASIC_TIMEOUT,

/* 	ATCMD_GROUP_WIFI
 *************************/
	ATCMD_WIFI_MACADDR,
	ATCMD_WIFI_COUNTRY,
	ATCMD_WIFI_TXPOWER,
	ATCMD_WIFI_RXSIGNAL,
	ATCMD_WIFI_RATECTRL,
	ATCMD_WIFI_MCS,
	ATCMD_WIFI_DUTY_CYCLE,
	ATCMD_WIFI_CCA_THRESHOLD,
	ATCMD_WIFI_TX_TIME,
	ATCMD_WIFI_TSF,
	ATCMD_WIFI_LBT,
	ATCMD_WIFI_IPADDR,
	ATCMD_WIFI_DHCP,
	ATCMD_WIFI_SCAN,
	ATCMD_WIFI_CONNECT,
	ATCMD_WIFI_DISCONNECT,
	ATCMD_WIFI_ROAMING,
	ATCMD_WIFI_PING,
	ATCMD_WIFI_FOTA,
	ATCMD_WIFI_SLEEP,
	ATCMD_WIFI_TIMEOUT,

	ATCMD_WIFI_SOFTAP,
	ATCMD_WIFI_BSS_MAX_IDLE,
	ATCMD_WIFI_STAINFO,
	ATCMD_WIFI_DHCPS,

/* 	ATCMD_GROUP_SOCKET
 *************************/
	ATCMD_SOCKET_OPEN,
	ATCMD_SOCKET_CLOSE,
	ATCMD_SOCKET_LIST,
	ATCMD_SOCKET_SEND,
	ATCMD_SOCKET_RECV_LOG,
	ATCMD_SOCKET_TIMEOUT,

/* 	ATCMD_GROUP_HOST
 *************************/
	ATCMD_HOST_SEND,
	ATCMD_HOST_TIMEOUT,

/* 	ATCMD_GROUP_USER
 *************************/
	ATCMD_USER_MIN,
	ATCMD_USER_MAX = ATCMD_USER_MIN + 100,
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

typedef struct
{
	struct
	{
		int msg;
		int data;
	} len;

	struct
	{
		char msg[ATCMD_MSG_LEN_MAX];
		char data[ATCMD_TXBUF_SIZE];
	} buf;
} atcmd_rxd_t;

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

#include "atcmd_basic.h"
#include "atcmd_wifi.h"
#include "atcmd_socket.h"
#include "atcmd_user.h"

/**********************************************************************************************/

inline uint32_t atcmd_sys_now (void)
{
	return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

extern const char *atcmd_strerror (int err);

extern int atcmd_param_to_int8 (const char *param, int8_t *val);
extern int atcmd_param_to_int16 (const char *param, int16_t *val);
extern int atcmd_param_to_int32 (const char *param, int32_t *val);
extern int atcmd_param_to_uint8 (const char *param, uint8_t *val);
extern int atcmd_param_to_uint16 (const char *param, uint16_t *val);
extern int atcmd_param_to_uint32 (const char *param, uint32_t *val);
extern float atcmd_param_to_float (const char *param, float *val);
extern int atcmd_param_to_hex (const char *param, uint32_t *val);
extern char *atcmd_param_to_str (const char *param, char *str, int len);
extern char *atcmd_str_to_param (const char *str, char *param, int len);

#ifndef CONFIG_ATCMD_WITHOUT_LWIP
extern int atcmd_data_mode_enable (atcmd_socket_t *socket, int32_t len, uint32_t timeout);
#else
extern int atcmd_data_mode_enable (int32_t len, uint32_t timeout);
#endif
extern void atcmd_data_mode_disable (void);

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

#ifndef CONFIG_ATCMD_TRACE
#define atcmd_trace_init()	0
#define atcmd_trace_exit()

#define atcmd_trace_task_loop(id)
#define atcmd_trace_task_suspend(id)
#define atcmd_trace_task_resume(id)

#define atcmd_trace_mutex_take(id)
#define atcmd_trace_mutex_give(id)

#define atcmd_trace_func_call(id)
#define atcmd_trace_func_return(id)
#endif

/**********************************************************************************************/
#endif /* #ifndef __NRC_ATCMD_H__ */
