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

enum ATCMD_HOST_EVENT
{
	ATCMD_HOST_EVENT_SEND_IDLE = 0,
	ATCMD_HOST_EVENT_SEND_DROP,
	ATCMD_HOST_EVENT_SEND_EXIT,
	ATCMD_HOST_EVENT_SEND_ERROR,
};

typedef struct
{
#define ATCMD_PACKET_START				"SOP+"
#define ATCMD_PACKET_END				"+EOP"

#define ATCMD_PACKET_START_SIZE			4
#define ATCMD_PACKET_HEADER_SIZE		4
#define ATCMD_PACKET_PAYLOAD_LEN_MAX	ATCMD_DATA_LEN_MAX
#define ATCMD_PACKET_END_SIZE			4

#define ATCMD_PACKET_LEN_MIN	(ATCMD_PACKET_START_SIZE + ATCMD_PACKET_HEADER_SIZE + ATCMD_PACKET_END_SIZE)
#define ATCMD_PACKET_LEN_MAX	(ATCMD_PACKET_LEN_MIN + ATCMD_PACKET_PAYLOAD_LEN_MAX)

	char start[ATCMD_PACKET_START_SIZE];

	struct
	{
		uint16_t seq;
		uint16_t len; /* payload length */
	}; /* header */

	char payload[0];
	/*
	 * char payload[0..1500];
	 * char end[ATCMD_PACKET_END_SIZE];
	 */
} atcmd_packet_t;

/**********************************************************************************************/

#define ATCMD_MSG_HEVENT(fmt, ...)	\
		ATCMD_MSG_EVENT("HEVENT", fmt, ##__VA_ARGS__)

#define ATCMD_MSG_HRXD(buf, len, fmt, ...)	\
		atcmd_msg_snprint(ATCMD_MSG_TYPE_EVENT, buf, len, "HRXD:" fmt, ##__VA_ARGS__)

extern int atcmd_host_enable (void);
extern void atcmd_host_disable (void);
extern int atcmd_host_send_data (char *data, int len, int *err);
extern void atcmd_host_recv_data (char *buf, int len);

extern int atcmd_host_event_send_idle (int len);
extern int atcmd_host_event_send_drop (int len);
extern int atcmd_host_event_send_exit (int len);
extern int atcmd_host_event_send_error (int len, int err);

/**********************************************************************************************/
#endif /* #ifndef __NRC_ATCMD_HOST_H__ */

