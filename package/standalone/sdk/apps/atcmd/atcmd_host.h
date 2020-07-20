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

#ifndef __NRC_ATCMD_HOST_H__
#define __NRC_ATCMD_HOST_H__
/**********************************************************************************************/

#define ATCMD_DATA_LEN_MAX		CONFIG_ATCMD_DATA_LEN_MAX

#define ATCMD_PKT_START			"STX["
#define ATCMD_PKT_END			"]ETX"

#define ATCMD_PKT_START_LEN		4
#define ATCMD_PKT_END_LEN		4

#define ATCMD_PKT_LEN_MIN		(ATCMD_PKT_START_LEN + sizeof(atcmd_packet_header_t) + ATCMD_PKT_END_LEN)
#define ATCMD_PKT_LEN_MAX		(ATCMD_PKT_LEN_MIN + sizeof(atcmd_socket_t) + ATCMD_DATA_LEN_MAX)


enum ATCMD_PKT_TYPE
{
	ATCMD_PKT_CMD = 0,
	ATCMD_PKT_RET,
	ATCMD_PKT_INFO,
	ATCMD_PKT_EVENT,
	ATCMD_PKT_DATA
};

typedef struct
{
	uint16_t type:15; /* enum ATCMD_PKT_TYPE			*/
	uint16_t async:1; /* 1: no return value (OK/FAIL)	*/
	uint16_t length;
} atcmd_packet_header_t;

typedef struct
{
	char start[ATCMD_PKT_START_LEN];

	atcmd_packet_header_t header;

	union
	{
		char payload[0];

		char cmd[0];
		char resp[0];

		struct
		{
			atcmd_socket_t socket;
			char data[0];
		};
	}; /* (24 + ATCMD_DATA_LEN_MAX)-byte */

	/* char end[ATCMD_PKT_END_LEN]; */
} atcmd_packet_t; /* (4 + 4 + 24 + ATCMD_DATA_LEN_MAX + 4)-byte */

extern int atcmd_host_receive (char *buf, int len);
extern int atcmd_host_transmit_response (int type, char *resp, int len);
extern int atcmd_host_transmit_data (atcmd_socket_t *socket, char *data, int len);

/**********************************************************************************************/
#endif /* #ifndef __NRC_ATCMD_HOST_H__ */

