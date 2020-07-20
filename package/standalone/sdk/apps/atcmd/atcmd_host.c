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


#include "atcmd.h"


/**********************************************************************************************/

static void atcmd_host_receive_data (atcmd_packet_t *packet)
{
	atcmd_socket_t *socket;
	char *data;
	int len;
	int ret;

	switch (packet->socket.protocol)
	{
		case ATCMD_SOCKET_PROTO_UDP:
		case ATCMD_SOCKET_PROTO_TCP:
			socket = (atcmd_socket_t *)&packet->socket;
			data = packet->data;
			len = packet->header.length - sizeof(atcmd_socket_t);

			ret = atcmd_receive_data_callback(socket, data, len);
			break;

		default:
			ret = -EINVAL;
	}

	if (packet->header.async)
		return;

	switch (ret)
	{
		case 0:
			ATCMD_LOG_RETURN("TXD", ATCMD_SUCCESS);
			break;

		case -EAGAIN:
			ATCMD_LOG_RETURN("TXD", ATCMD_SUCCESS);
			break;

		default:
			ATCMD_LOG_RETURN("TXD", ATCMD_ERROR_FAIL);
	}
}

int atcmd_host_receive (char *buf, int len)
{
#define HSSA_RX_QUE_NUM		2

	static bool cmd_mode = false;
	static char rxq[HSSA_RX_QUE_NUM][ATCMD_PKT_LEN_MAX];
	static int rxq_idx = 0;
	static char *rxbuf = rxq[0];
	static int cnt = 0;
	atcmd_packet_t *packet = NULL;
	int i, j;

	if (!buf || !len)
		return -1;

	for (i = 0 ; i < len ; i++)
	{
		rxbuf[cnt++] = buf[i];

		if (cnt == 2)
		{
			if (memcmp(rxbuf, "AT", 2) == 0 || memcmp(rxbuf, "at", 2) == 0)
			{
/*				_atcmd_debug("host_rx: cmd_mode=1\n"); */

				cmd_mode = true;
				continue;
			}
		}
		else if (cnt > 2 && cmd_mode)
		{
			if (memcmp(&rxbuf[cnt - 2], "\r\n", 2) == 0)
			{
				rxbuf[cnt] = '\0';

/*				_atcmd_debug("host_rx: %s\n", rxbuf); */

				atcmd_receive_command(rxbuf, cnt);

				cnt = 0;
				cmd_mode = false;

/*				_atcmd_debug("host_rx: cmd_mode=0\n"); */
			}
			else if (cnt == ATCMD_PKT_LEN_MAX)
			{
				cnt = 0;
				cmd_mode = false;

				_atcmd_error("host_rx: buffer overflow, cmd_mode=0\n");
			}

			continue;
		}

		if (cnt == ATCMD_PKT_START_LEN)
		{
			if (memcmp(rxbuf, ATCMD_PKT_START, ATCMD_PKT_START_LEN) != 0)
			{
				for (cnt--, j = 0 ; j < cnt ; j++)
					rxbuf[j] = rxbuf[j + 1];
			}
		}
		else if (cnt > ATCMD_PKT_LEN_MIN && cnt <= ATCMD_PKT_LEN_MAX)
		{
			if (memcmp(&rxbuf[cnt - ATCMD_PKT_END_LEN], ATCMD_PKT_END, ATCMD_PKT_END_LEN) == 0)
			{
				packet = (atcmd_packet_t *)rxbuf;

				switch (packet->header.type)
				{
					case ATCMD_PKT_CMD:
						atcmd_receive_command(packet->cmd, packet->header.length);
						break;

					case ATCMD_PKT_DATA:
						if ((cnt - ATCMD_PKT_LEN_MIN) != packet->header.length)
						{
							_atcmd_error("%s: cnt=%d payload=%d/%d, len=%u, err=%d\n", __func__,
										cnt, cnt - ATCMD_PKT_LEN_MIN, packet->header.length,
										packet->header.length - 24,
										(cnt - ATCMD_PKT_LEN_MIN) - packet->header.length);

						}

						atcmd_host_receive_data(packet);
						break;
				}

				cnt = 0;

				if (++rxq_idx >= HSSA_RX_QUE_NUM)
					rxq_idx = 0;

				rxbuf = rxq[rxq_idx];
			}
			else if (cnt == ATCMD_PKT_LEN_MAX)
			{
				packet = (atcmd_packet_t *)rxbuf;

				_atcmd_error("host_rx: buffer overflow, type=%u, len=%u, cnt=%d\n",
								packet->header.type, packet->header.length, cnt);

				atcmd_dump_hex_print(rxbuf, cnt, true);
				cnt = 0;
			}
		}
	}

	return 0;
}

static int atcmd_host_transmit (atcmd_packet_t *packet)
{
	int len;
	int ret;

	if (!packet)
		return -1;

	memcpy(packet->start, ATCMD_PKT_START, ATCMD_PKT_START_LEN);
	memcpy(packet->payload + packet->header.length, ATCMD_PKT_END, ATCMD_PKT_END_LEN);

	len = ATCMD_PKT_LEN_MIN + packet->header.length;

	ret = atcmd_transmit((char *)packet, len);
	if (ret != len)
		return -1;

	return 0;
}

int atcmd_host_transmit_response (int type, char *resp, int len)
{
	static char buf[ATCMD_PKT_LEN_MAX];
	atcmd_packet_t *packet = (atcmd_packet_t *)buf;

	switch (type)
	{
		case ATCMD_LOG_TYPE_RETURN:
			packet->header.type = ATCMD_PKT_RET;
			break;

		case ATCMD_LOG_TYPE_INFO:
			packet->header.type = ATCMD_PKT_INFO;
			break;

		case ATCMD_LOG_TYPE_EVENT:
			packet->header.type = ATCMD_PKT_EVENT;
			break;

		default:
			_atcmd_error("invalid type, %d\n", type);
			return -1;
	}

	packet->header.length = len;

	memcpy(packet->resp, resp, len);

	return atcmd_host_transmit(packet);
}

int atcmd_host_transmit_data (atcmd_socket_t *socket, char *data, int len)
{
	static char buf[ATCMD_PKT_LEN_MAX];
	atcmd_packet_t *packet = (atcmd_packet_t *)buf;
	int ret;
	int i;

	if (!socket || !data || !len)
		return -1;

	switch (socket->protocol)
	{
		case ATCMD_SOCKET_PROTO_UDP:
		case ATCMD_SOCKET_PROTO_TCP:
			memcpy(&packet->socket, socket, sizeof(atcmd_socket_t));
			break;

		default:
			return -1;
	}

	packet->header.type = ATCMD_PKT_DATA;

	for (i = 0 ; i < len ; i += ATCMD_DATA_LEN_MAX)
	{
		if ((len - i) <= ATCMD_DATA_LEN_MAX)
			packet->header.length = len - i;
		else
			packet->header.length = ATCMD_DATA_LEN_MAX;

		memcpy(packet->data, data + i, packet->header.length);

		packet->header.length += sizeof(atcmd_socket_t);

		if (atcmd_host_transmit(packet) != 0)
			return -1;
	}

	return 0;
}

