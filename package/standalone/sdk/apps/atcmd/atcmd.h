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

#define MACSW_VER_MAJOR			(VERSION_MAJOR)
#define MACSW_VER_MINOR			(VERSION_MINOR)
#define MACSW_VER_REVISION		(VERSION_REVISION)

#define ATCMD_VER_MAJOR			(1)
#define ATCMD_VER_MINOR			(11)
#define ATCMD_VER_REVISION		(0)

/**********************************************************************************************/

/*
 *	RX: target from host
 *	 TX: target to host
 */

#ifndef CONFIG_ATCMD_DEBUG
/* #define CONFIG_ATCMD_DEBUG */
#endif

/* #define CONFIG_ATCMD_HOST_MODE */		/* if enabled, atcmd_host.c file is needed. */
/* #define CONFIG_ATCMD_HOST_PROMPT */

/* #define CONFIG_ATCMD_WELCOME_MESSAGE */

/* #define CONFIG_ATCMD_PREFIX_CHECK */

/* #define CONFIG_ATCMD_TRXBUF_STATIC */

#define CONFIG_ATCMD_DATA_LEN_MAX			(4 * 1024)

/**********************************************************************************************/

#define _atcmd_malloc					pvPortMalloc
#define _atcmd_free						vPortFree
#define _atcmd_printf					nrc_uart_printf

#define _atcmd_log(fmt, ...)			_atcmd_printf("[ATCMD] " fmt, ##__VA_ARGS__)

#define _atcmd_debug(fmt, ...)			_atcmd_log(fmt, ##__VA_ARGS__)
#define _atcmd_trace()					_atcmd_log("%s::%d\r\n", __func__, __LINE__)

#define _atcmd_info(fmt, ...)			_atcmd_log(fmt, ##__VA_ARGS__)
#define _atcmd_error(fmt, ...)			_atcmd_log("%s: " fmt, __func__, ##__VA_ARGS__)

/**********************************************************************************************/

#define ATCMD_LEN_MIN					2
#define ATCMD_LEN_MAX					128
#define ATCMD_PARAMS_MAX				10

#define ATCMD_DATA_LEN_MAX				CONFIG_ATCMD_DATA_LEN_MAX

#define ATCMD_RXBUF_SIZE				ATCMD_DATA_LEN_MAX /* uplink */
#define ATCMD_TXBUF_SIZE				ATCMD_DATA_LEN_MAX /* donwlink */

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

	ATCMD_CFG_TERMINAL = ATCMD_CFG_MIN,
	ATCMD_CFG_HOST,
	ATCMD_CFG_PROMPT,
	ATCMD_CFG_ECHO,
	ATCMD_CFG_HISTORY,
	ATCMD_CFG_LOWERCASE,
	ATCMD_CFG_LINEFEED,

	ATCMD_CFG_NUM,
	ATCMD_CFG_MAX = ATCMD_CFG_NUM - 1,

	ATCMD_CFG_LIMIT = (32 - 1)
};

enum ATCMD_MODE
{
#define ATCMD_MODE_NONE		-1

	ATCMD_MODE_NORMAL = 0,
	ATCMD_MODE_TERMINAL,
#ifdef CONFIG_ATCMD_HOST_MODE
	ATCMD_MODE_HOST,
#endif

	ATCMD_MODE_NUM
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
	ATCMD_GROUP_TEST,		/* AT+T	*/
	ATCMD_GROUP_USER		/* AT+U	*/
};

enum ATCMD_ID
{
/* 	ATCMD_GROUP_BASIC
 *************************/
	ATCMD_BASIC_LOG = 0,
	ATCMD_BASIC_MODE,
	ATCMD_BASIC_VERSION,
	ATCMD_BASIC_UART,
	ATCMD_BASIC_GPIOCFG,
	ATCMD_BASIC_GPIOVAL,
	ATCMD_BASIC_ADC,
	ATCMD_BASIC_SLEEP,
	ATCMD_BASIC_TIMEOUT,

/* 	ATCMD_GROUP_WIFI
 *************************/
	ATCMD_WIFI_MACADDR,
	ATCMD_WIFI_COUNTRY,
	ATCMD_WIFI_TXPOWER,
	ATCMD_WIFI_RXSIGNAL,
	ATCMD_WIFI_RATECTRL,
	ATCMD_WIFI_MCS,
	ATCMD_WIFI_TSF,
	ATCMD_WIFI_IPADDR,
	ATCMD_WIFI_DHCP,
	ATCMD_WIFI_DHCPS,
	ATCMD_WIFI_SCAN,
	ATCMD_WIFI_CONNECT,
	ATCMD_WIFI_DISCONNECT,
	ATCMD_WIFI_PING,
	ATCMD_WIFI_SOFTAP,
	ATCMD_WIFI_TIMEOUT,

/* 	ATCMD_GROUP_SOCKET
 *************************/
	ATCMD_SOCKET_OPEN,
	ATCMD_SOCKET_CLOSE,
	ATCMD_SOCKET_LIST,
	ATCMD_SOCKET_SEND,
	ATCMD_SOCKET_SEND_MODE,
	ATCMD_SOCKET_RECV,
	ATCMD_SOCKET_RECV_MODE,
	ATCMD_SOCKET_RECV_LOG,
	ATCMD_SOCKET_RECV_AVAIL,
	ATCMD_SOCKET_TIMEOUT,
	ATCMD_SOCKET_PERF,

/* 	ATCMD_GROUP_TEST
 *************************/
	ATCMD_TEST_IPERF,
	ATCMD_TEST_ADDBA,
	ATCMD_TEST_DELBA,
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
	char cmd[ATCMD_LEN_MAX + 1];
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

#define ATCMD_LOG_TYPE_MAX		6
#define ATCMD_LOG_TYPE_NUM		ATCMD_LOG_TYPE_MAX

#define ATCMD_LOG_TYPE_CHAR		{ 'R', 'H', 'T', 'I', 'E', 'D' }
#define ATCMD_LOG_TYPE_STR		{ "RETURN", "HELP", "TRACE", "INFO", "EVENT", "DEBUG" }

enum ATCMD_LOG_TYPE
{
	ATCMD_LOG_TYPE_RETURN = (1 << 0),
	ATCMD_LOG_TYPE_HELP = (1 << 1),
	ATCMD_LOG_TYPE_TRACE = (1 << 2),
	ATCMD_LOG_TYPE_INFO = (1 << 3),
	ATCMD_LOG_TYPE_EVENT = (1 << 4),
	ATCMD_LOG_TYPE_DEBUG = (1 << 5),

	ATCMD_LOG_TYPE_ALL = ~0,

	ATCMD_LOG_TYPE_TERMINAL = ATCMD_LOG_TYPE_ALL,
	ATCMD_LOG_TYPE_HOST = (ATCMD_LOG_TYPE_RETURN | ATCMD_LOG_TYPE_INFO | ATCMD_LOG_TYPE_EVENT),
	ATCMD_LOG_TYPE_NORMAL = (ATCMD_LOG_TYPE_HOST | ATCMD_LOG_TYPE_HELP),
};

extern bool ATCMD_LOG_VALID (int type);
extern void ATCMD_LOG_RESET (void);
extern void ATCMD_LOG_ENABLE (int type);
extern void ATCMD_LOG_DISABLE (int type);
extern int ATCMD_LOG_STATUS (int type);
extern int ATCMD_LOG_PRINT (int type, const char *fmt1, const char *fmt2, ...);

#define ATCMD_LOG_RETURN(cmd, ret)	atcmd_transmit_return(cmd, ret)

#define ATCMD_LOG_HELP(fmt, ...)	\
		ATCMD_LOG_PRINT(ATCMD_LOG_TYPE_HELP, fmt, fmt, ##__VA_ARGS__)

#define ATCMD_LOG_TRACE(name, fmt1, fmt2, ...)	\
		ATCMD_LOG_PRINT(ATCMD_LOG_TYPE_TRACE, name ":" fmt1, name ": " fmt2, ##__VA_ARGS__)

#define ATCMD_LOG_INFO(name, fmt1, fmt2, ...)	\
		ATCMD_LOG_PRINT(ATCMD_LOG_TYPE_INFO, name ":" fmt1, name ": " fmt2, ##__VA_ARGS__)

#define ATCMD_LOG_EVENT(name, fmt1, fmt2, ...)	\
		ATCMD_LOG_PRINT(ATCMD_LOG_TYPE_EVENT, name ":" fmt1, name ": " fmt2, ##__VA_ARGS__)

#define ATCMD_LOG_DEBUG(fmt, ...)	\
		ATCMD_LOG_PRINT(ATCMD_LOG_TYPE_DEBUG, fmt, fmt, ##__VA_ARGS__)

/**********************************************************************************************/

#include "atcmd_trace.h"
#include "atcmd_basic.h"
#include "atcmd_wifi.h"
#include "atcmd_socket.h"
#include "atcmd_test.h"

#ifdef CONFIG_ATCMD_HOST_MODE
#include "atcmd_host.h"
#endif

/**********************************************************************************************/

extern const char *atcmd_strerror (int err);

extern int atcmd_param_to_long (char *param, long *val);
extern int atcmd_param_to_ulong (char *param, unsigned long *val);
extern char *atcmd_param_to_str (const char *param, char *str, int len);
extern char *atcmd_str_to_param (const char *str, char *param, int len);

extern enum ATCMD_MODE atcmd_mode_get (void);
extern int atcmd_mode_set (enum ATCMD_MODE mode);

extern void ATCMD_DATA_MODE_ENABLE (atcmd_socket_t *socket, uint32_t len,
									uint32_t timeout, bool sync);
extern void ATCMD_DATA_MODE_DISABLE (void);

extern atcmd_group_t *atcmd_group_search (enum ATCMD_GROUP_ID id);
extern int atcmd_group_register (atcmd_group_t *group);
extern int atcmd_group_unregister (enum ATCMD_GROUP_ID id);

extern atcmd_info_t *atcmd_search (atcmd_group_t *group, enum ATCMD_ID id);
extern int atcmd_info_register (enum ATCMD_GROUP_ID gid, atcmd_info_t *info);
extern void atcmd_info_unregister (enum ATCMD_GROUP_ID gid, enum ATCMD_ID id);

extern void atcmd_receive (char *data, int size);
extern int atcmd_receive_command (char *data, int size);

extern int atcmd_transmit (char *data, int size);
extern int atcmd_transmit_data (atcmd_socket_t *socket, char *data, int len);
extern int atcmd_transmit_return (char *cmd, int ret);

extern int atcmd_enable (_hif_info_t *info);
extern void atcmd_disable (void);

extern int atcmd_user_register (const char *cmd, int id, atcmd_handler_t handler[]);
extern int atcmd_user_unregister (int id);

extern void atcmd_dump_hex_print (const void *buf, size_t len, bool ascii);
extern void atcmd_dump_hex_print_terminal (const void *buf, size_t len, bool ascii);

/**********************************************************************************************/
#endif /* #ifndef __NRC_ATCMD_H__ */
