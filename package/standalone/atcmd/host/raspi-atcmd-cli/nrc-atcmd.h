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

#include "common.h"

/**********************************************************************************************/

#define ATCMD_MSG_LEN_MIN		4			/* 'AT\r\n' */
#define ATCMD_MSG_LEN_MAX		128

#define ATCMD_DATA_LEN_MAX		(4 * 1024) 	/* f/w atcmd.h ATCMD_DATA_LEN_MAX */

#define ATCMD_IPADDR_LEN_MIN	STR_IP6ADDR_LEN_MIN
#define ATCMD_IPADDR_LEN_MAX	STR_IP6ADDR_LEN_MAX

/**********************************************************************************************/

enum ATCMD_RET_TYPE
{
	ATCMD_RET_ERROR = -1,
	ATCMD_RET_OK = 0,
	ATCMD_RET_NONE
};

enum ATCMD_INFO
{
	ATCMD_INFO_START = 0,

	ATCMD_INFO_END,

	ATCMD_INFO_NUM = ATCMD_INFO_END
};

enum ATCMD_EVENT
{
	ATCMD_EVENT_START = 0,

	ATCMD_BEVENT_FWBINDL_IDLE = ATCMD_EVENT_START,
	ATCMD_BEVENT_FWBINDL_DROP,
	ATCMD_BEVENT_FWBINDL_DONE,

	/* Wi-Fi Events */
	ATCMD_WEVENT_CONNECT_SUCCESS,
	ATCMD_WEVENT_DISCONNECT,

	ATCMD_WEVENT_DHCP_START,
	ATCMD_WEVENT_DHCP_STOP,
	ATCMD_WEVENT_DHCP_BUSY,
	ATCMD_WEVENT_DHCP_FAIL,
	ATCMD_WEVENT_DHCP_SUCCESS,
	ATCMD_WEVENT_DHCP_TIMEOUT,

	ATCMD_WEVENT_FOTA_VERSION,
	ATCMD_WEVENT_FOTA_BINARY,
	ATCMD_WEVENT_FOTA_DOWNLOAD,
	ATCMD_WEVENT_FOTA_UPDATE,
	ATCMD_WEVENT_FOTA_FAIL,

	ATCMD_WEVENT_DEEPSLEEP_WAKEUP,

	/* Socket Events */
	ATCMD_SEVENT_CONNECT,
	ATCMD_SEVENT_CLOSE,
	ATCMD_SEVENT_SEND_IDLE,
	ATCMD_SEVENT_SEND_DROP,
	ATCMD_SEVENT_SEND_EXIT,
	ATCMD_SEVENT_SEND_ERROR,
	ATCMD_SEVENT_RECV_READY,
	ATCMD_SEVENT_RECV_ERROR,

	ATCMD_EVENT_END,
	ATCMD_EVENT_NUM = ATCMD_EVENT_END,
};

enum ATCMD_CB_TYPE
{
	ATCMD_CB_INFO = 0,
	ATCMD_CB_EVENT,
	ATCMD_CB_RXD
};

typedef struct
{
	bool verbose;

	int id;
	int len;
	char remote_addr[ATCMD_IPADDR_LEN_MAX + 1];
	int remote_port;
} atcmd_rxd_t;

typedef int (*atcmd_info_cb_t) (enum ATCMD_INFO info, int argc, char *argv[]);
typedef int (*atcmd_event_cb_t) (enum ATCMD_EVENT event, int argc, char *argv[]);
typedef void (*atcmd_rxd_cb_t) (atcmd_rxd_t *rxd, char *data);

/**********************************************************************************************/

extern char *nrc_atcmd_param_to_str (const char *param, char *str, int len);

extern int nrc_atcmd_send (char *buf, int len);
extern int nrc_atcmd_send_cmd (const char *fmt, ...);
extern int nrc_atcmd_send_data (char *data, int len);
extern void nrc_atcmd_recv (char *buf, int len);

extern int nrc_atcmd_register_callback (int type, void *func);
extern int nrc_atcmd_unregister_callback (int type);

extern void nrc_atcmd_log_on (void);
extern void nrc_atcmd_log_off (void);
extern bool nrc_atcmd_log_is_on (void);

extern void nrc_atcmd_data_info (uint64_t *send, uint64_t *recv);
extern void nrc_atcmd_data_reset (void);

extern int nrc_atcmd_firmware_download (char *bin_data, int bin_size, uint32_t bin_crc32);

/**********************************************************************************************/
#endif /* #ifndef __NRC_H__ */


