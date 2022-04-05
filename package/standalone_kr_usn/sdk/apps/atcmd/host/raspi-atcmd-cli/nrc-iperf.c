/*
 * NEWRACOM MIT License
 *
 * Copyright (c) 2018 <NEWRACOM LTD>
 *
 * Permission is hereby granted for use on NEWRACOM NRC7291 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */


#include "nrc-atcmd.h"
#include "nrc-iperf.h"

#define iperf_log(fmt, ...)		log_info(fmt, ##__VA_ARGS__)
#define iperf_error(fmt, ...)	log_error(fmt, ##__VA_ARGS__)
#define iperf_debug(fmt, ...)	log_debug(fmt, ##__VA_ARGS__)

#if 0
extern void print_hex_dump (const void *buf, size_t len, int ascii);
#define iperf_print_hex_dump(buf, len, ascii) 	print_hex_dump
#endif

/**********************************************************************************************/

static uint32_t byte_to_bps (iperf_time_t time, uint32_t byte)
{
	return (8 * byte) / time;
}

#if 0
static char *time_to_string (iperf_time_t time)
{
	static char buf[20];

	if (time < 1)
		snprintf(buf, sizeof(buf), "%d m", (int)(time * 1000));
	else
		snprintf(buf, sizeof(buf), "%4.1f ", time);

	return buf;
}
#endif

static char *byte_to_string (uint32_t byte)
{
	static char buf[20];

	if (byte < 1024)
		snprintf(buf, sizeof(buf), "%u ", byte);
	else if (byte < (1024 * 1024))
		snprintf(buf, sizeof(buf), "%4.2f K", byte / 1024.);
	else
		snprintf(buf, sizeof(buf), "%4.2f M", (byte / 1024.) / 1024.);

	return buf;
}

static char *bps_to_string (uint32_t bps)
{
	static char buf[20];

	if (bps < 1000)
		snprintf(buf, sizeof(buf), "%u ", bps);
	else if (bps < (1000 * 1000))
		snprintf(buf, sizeof(buf), "%4.2f K", bps / 1000.);
	else
		snprintf(buf, sizeof(buf), "%4.2f M", (bps / 1000.) / 1000.);

	return buf;
}

/**********************************************************************************************/

static int iperf_get_time (iperf_time_t *time)
{
	struct timespec s_time;

	if (clock_gettime(CLOCK_REALTIME, &s_time) != 0)
		return -errno;

	*time = s_time.tv_sec;
	*time += (s_time.tv_nsec / 1000) / 1000000.;

	return 0;
}

static bool iperf_is_ip4addr (const char *addr)
{
	char c;
	int len;
	int dot;
	int i;

	if (!addr)
		return false;

	len = strlen(addr);
	if (len < STR_IP4ADDR_LEN_MIN || len > STR_IP4ADDR_LEN_MAX)
		return false;

	for (dot = 0, i = 0 ; i < len ; i++)
	{
		c = addr[i];

		if (c == '.')
		{		
			dot++;
			continue;
		}

		if (c < '0' || c > '9')
			return false;
	}

	if (c == '.' || dot != 3)
		return false;

	return true;
}

/**********************************************************************************************/

static bool g_iperf_socket_send_idle = true;
static bool g_iperf_socket_send_passthrough = false;
static int s_socket_id = -1;

static void iperf_udp_server_recv_callback (atcmd_rxd_t *rxd, char *data);
static void iperf_udp_client_recv_callback (atcmd_rxd_t *rxd, char *data);
static void iperf_udp_client_event_callback (enum ATCMD_EVENT event, int argc, char *argv[]);

static void iperf_tcp_server_recv_callback (atcmd_rxd_t *rxd, char *data);
static void iperf_tcp_client_recv_callback (atcmd_rxd_t *rxd, char *data);
static void iperf_tcp_server_event_callback (enum ATCMD_EVENT event, int argc, char *argv[]);
static void iperf_tcp_client_event_callback (enum ATCMD_EVENT event, int argc, char *argv[]);

/* static void iperf_socket_print (iperf_socket_t *socket)
{
	iperf_log("[ socket info ]\n");
	iperf_log(" - id: %d\n", socket->id);
	iperf_log(" - protocol: %s\n", socket->protocol == IPERF_PROTO_UDP ? "udp" :
									(socket->protocol == IPERF_PROTO_TCP ? "tcp" : "?"));
	iperf_log(" - local_port: %u\n", socket->local_port);
	iperf_log(" - remote_port: %u\n", socket->remote_port);
	iperf_log(" - remote_addr: %s\n", socket->remote_addr);
} */

static void iperf_socket_init (iperf_socket_t *socket)
{
	memset(socket, 0, sizeof(iperf_socket_t));

	socket->id = -1;
	socket->protocol = IPERF_PROTO_NONE;
	socket->local_port = 0;
	socket->remote_port = 0;
	strcpy(socket->remote_addr, IPERF_IPADDR_ANY);
}

static void iperf_socket_info_callback (enum ATCMD_INFO info, int argc, char *argv[])
{
	int id;

	if (argc == 1 && memcmp(argv[0], "+SOPEN:", 7) == 0)
	{
		sscanf(argv[0], "+SOPEN:%u", &id);
		s_socket_id = id;
	}
}

static int iperf_socket_open_udp (iperf_socket_t *socket)
{
	int i;
	s_socket_id = -1;
	nrc_atcmd_register_callback(ATCMD_CB_INFO, iperf_socket_info_callback);

	if (nrc_atcmd_send_cmd("AT+SOPEN=\"udp\",%u", socket->local_port) != ATCMD_RET_OK)
		return -1;

	for (i = 0; i < 100; i++)
	{
		if (s_socket_id >= 0)
		{
			socket->id = s_socket_id;
			s_socket_id = -1;
			break;
		}
		usleep(10000);
	}
	if (i == 100)
		socket->id = 0;

	if (socket->remote_port == 0 && strcmp(socket->remote_addr, IPERF_IPADDR_ANY) == 0)
		nrc_atcmd_register_callback(ATCMD_CB_RXD, iperf_udp_server_recv_callback);
	else
	{
		nrc_atcmd_register_callback(ATCMD_CB_EVENT, iperf_udp_client_event_callback);
		nrc_atcmd_register_callback(ATCMD_CB_RXD, iperf_udp_client_recv_callback);
	}

	return 0;
}

static int iperf_socket_open_tcp (iperf_socket_t *socket)
{
	int i;
	s_socket_id = -1;
	nrc_atcmd_register_callback(ATCMD_CB_INFO, iperf_socket_info_callback);

	if (socket->local_port > 0) /* TCP Server */
	{
		if (nrc_atcmd_send_cmd("AT+SOPEN=\"tcp\",%u", socket->local_port) != ATCMD_RET_OK)
			return -1;

		nrc_atcmd_register_callback(ATCMD_CB_EVENT, iperf_tcp_server_event_callback);
		nrc_atcmd_register_callback(ATCMD_CB_RXD, iperf_tcp_server_recv_callback);
	}
	else if (socket->remote_port > 0 && strcmp(socket->remote_addr, IPERF_IPADDR_ANY) != 0) /* TCP Client */
	{
		if (nrc_atcmd_send_cmd("AT+SOPEN=\"tcp\",\"%s\",%u", socket->remote_addr, socket->remote_port) != ATCMD_RET_OK)
			return -1;

		nrc_atcmd_register_callback(ATCMD_CB_EVENT, iperf_tcp_client_event_callback);
		nrc_atcmd_register_callback(ATCMD_CB_RXD, iperf_tcp_client_recv_callback);
	}
	else
		return -1;

	for (i = 0; i < 100; i++)
	{
		if (s_socket_id >= 0)
		{
			socket->id = s_socket_id;
			s_socket_id = -1;
			break;
		}
		usleep(10000);
	}
	if (i == 100)
		socket->id = 0;

	return 0;
}

static int iperf_socket_open (iperf_socket_t *socket)
{
	if (nrc_atcmd_send_cmd("AT+SRXLOGLEVEL=1") != ATCMD_RET_OK)
		return -1;

	switch (socket->protocol)
	{
		case IPERF_PROTO_UDP:
			return iperf_socket_open_udp(socket);

		case IPERF_PROTO_TCP:
			return iperf_socket_open_tcp(socket);

		default:
			return -1;
	}
}

static int iperf_socket_close (iperf_socket_t *socket)
{
	if (g_iperf_socket_send_passthrough)
	{
		int i;

		for (i = 0 ; i < 5 ; i++)
		{
			if (g_iperf_socket_send_idle)
				break;

			sleep(1);
		};

		if (i > 5)
		{
			iperf_error("send busy\n");

			return -1;
		}

		if (nrc_atcmd_send_cmd("AT") != ATCMD_RET_OK)
		   return -1;

		g_iperf_socket_send_idle = true;
		g_iperf_socket_send_passthrough = false;
	}

	nrc_atcmd_unregister_callback(ATCMD_CB_INFO);
	nrc_atcmd_unregister_callback(ATCMD_CB_EVENT);
	nrc_atcmd_unregister_callback(ATCMD_CB_RXD);

	if (nrc_atcmd_send_cmd("AT+SCLOSE") != ATCMD_RET_OK)
		return -1;

	if (nrc_atcmd_send_cmd("AT+SRXLOGLEVEL=0") != ATCMD_RET_OK)
		return -1;

	return 0;
}

static int iperf_socket_send (iperf_socket_t *socket, char *buf, int len)
{
	if (g_iperf_socket_send_passthrough)
	{
		if (nrc_atcmd_send_cmd("AT") != ATCMD_RET_OK)
		   return -1;

		g_iperf_socket_send_idle = true;
		g_iperf_socket_send_passthrough = false;
	}

	if (nrc_atcmd_send_cmd("AT+SSEND=%d,\"%s\",%u,%d",
			socket->id, socket->remote_addr, socket->remote_port, len) != ATCMD_RET_OK)
		return -1;

	if (nrc_atcmd_send_data(buf, len) == ATCMD_RET_OK)
		return -1;

	return 0;
}

static int iperf_socket_send_passthrough (iperf_socket_t *socket, char *buf, int len, bool negative)
{
	if (!g_iperf_socket_send_passthrough)
	{
		if (negative)
			len *= -1;
		else if (socket->protocol == IPERF_PROTO_TCP)
		   	len = 0;
		else
		{
			iperf_error("UDP && !negative\n");
			return -1;
		}

		if (nrc_atcmd_send_cmd("AT+SSEND=%d,\"%s\",%u,%d",
			socket->id, socket->remote_addr, socket->remote_port, len) != ATCMD_RET_OK)
			return -1;

		g_iperf_socket_send_idle = false;
		g_iperf_socket_send_passthrough = true;
	}

	if (nrc_atcmd_send_data(buf, len) != ATCMD_RET_OK)
		return -1;

	return 0;
}

/**********************************************************************************************/

#if defined(iperf_print_hex_dump)
static void iperf_server_header_print (iperf_server_header_t *header, bool dump)
{
	iperf_log("[ IPERF_SERVER_HEADER ]\n");
	iperf_log(" - flags: 0x%08X\n", ntohl(header->flags));
	iperf_log(" - total_len1: %d\n", ntohl(header->total_len1));
	iperf_log(" - total_len2: %d\n", ntohl(header->total_len2));
	iperf_log(" - stop_sec: %d\n", ntohl(header->stop_sec));
	iperf_log(" - stop_usec: %d\n", ntohl(header->stop_usec));
	iperf_log(" - error_cnt: %d\n", ntohl(header->error_cnt));
	iperf_log(" - outorder_cnt: %d\n", ntohl(header->outorder_cnt));
	iperf_log(" - datagrams: %d\n", ntohl(header->datagrams));
	iperf_log(" - jitter1: %d\n", ntohl(header->jitter1));
	iperf_log(" - jitter2: %d\n", ntohl(header->jitter2));

	if (ntohl(header->flags) & IPERF_FLAG_HDR_VER1)
	{
		iperf_log(" - minTransit1: %d\n", ntohl(header->minTransit1));
		iperf_log(" - minTransit2: %d\n", ntohl(header->minTransit2));
		iperf_log(" - maxTransit1: %d\n", ntohl(header->maxTransit1));
		iperf_log(" - maxTransit2: %d\n", ntohl(header->maxTransit2));
		iperf_log(" - sumTransit1: %d\n", ntohl(header->sumTransit1));
		iperf_log(" - sumTransit2: %d\n", ntohl(header->sumTransit2));
		iperf_log(" - meanTransit1: %d\n", ntohl(header->meanTransit1));
		iperf_log(" - meanTransit2: %d\n", ntohl(header->meanTransit2));
		iperf_log(" - m2Transit1: %d\n", ntohl(header->m2Transit1));
		iperf_log(" - m2Transit2: %d\n", ntohl(header->m2Transit2));
		iperf_log(" - vdTransit1: %d\n", ntohl(header->vdTransit1));
		iperf_log(" - vdTransit2: %d\n", ntohl(header->vdTransit2));
		iperf_log(" - cntTransit: %d\n", ntohl(header->cntTransit));
		iperf_log(" - IPGcnt: %d\n", ntohl(header->IPGcnt));
		iperf_log(" - IPGsum: %d\n", ntohl(header->IPGsum));
	}

	if (dump)
	{
		if (ntohl(header->flags) & IPERF_FLAG_HDR_VER1)
			iperf_print_hex_dump(header, IPERF_SERVER_HDR1_SIZE, true);
		else
			iperf_print_hex_dump(header, IPERF_SERVER_HDR0_SIZE, true);
	}
}

static void iperf_client_header_print (iperf_client_header_t *header, bool dump)
{
	iperf_log("[ IPERF_CLIENT_HEADER ]\n");
	iperf_log(" - flags: 0x%08X\n", ntohl(header->flags));
	iperf_log(" - numThreads: %d\n", ntohl(header->numThreads));
	iperf_log(" - mPort: %d\n", ntohl(header->mPort));
	iperf_log(" - bufferlen: %d\n", ntohl(header->bufferlen));
	iperf_log(" - mWindowSize: %d\n", ntohl(header->mWindowSize));
	iperf_log(" - mAmount: %d\n", ntohl(header->mAmount));

	if (ntohl(header->flags) & IPERF_FLAG_HDR_VER1)
	{
		iperf_log(" - mRate: %d\n", ntohl(header->mRate));
		iperf_log(" - mUDPRateUnits: %d\n", ntohl(header->mUDPRateUnits));
		iperf_log(" - mRealTime: %d\n", ntohl(header->mRealTime));
	}

	if (dump)
	{
		if (ntohl(header->flags) & IPERF_FLAG_HDR_VER1)
			iperf_print_hex_dump(header, IPERF_CLIENT_HDR1_SIZE, true);
		else
			iperf_print_hex_dump(header, IPERF_CLIENT_HDR0_SIZE, true);
	}
}

static void iperf_udp_datagram_print (iperf_udp_datagram_t *datagram, int size, bool server, bool dump)
{
	iperf_log("[ IPERF_UDP_DATAGRAM ]\n");
	iperf_log(" - id: %d\n", ntohl(datagram->id));
	iperf_log(" - tv_sec: %u\n", ntohl(datagram->tv_sec));
	iperf_log(" - tv_usec: %u\n", ntohl(datagram->tv_usec));

	if (server)
		iperf_server_header_print(&datagram->server_header, false);
	else
		iperf_client_header_print(&datagram->client_header, false);

	if (dump)
		iperf_print_hex_dump(datagram, size, true);
}
#else
#define iperf_server_header_print(header, dump)
#define iperf_client_header_print(header, dump)
#define iperf_udp_datagram_print(datagram, size, server, dump)
#endif

/**********************************************************************************************/

static iperf_udp_server_info_t g_iperf_udp_server_info;

static void iperf_udp_server_init_info (iperf_udp_server_info_t *info, iperf_opt_t *option)
{
	memset(info, 0, sizeof(iperf_udp_server_info_t));

	info->client.id = -1;

	info->start = false;
	info->done = false;

	info->server_port = IPERF_DEFAULT_SERVER_PORT;
	info->report_interval = IPERF_DEFAULT_REPORT_INTERVAL;

	if (option)
	{
		if (option->server_port > 0)
			info->server_port = option->server_port;

		if (option->report_interval > 0)
			info->report_interval = option->report_interval;
	}
};

static void iperf_udp_server_init_payload (iperf_udp_datagram_t *datagram)
{
	char *payload;
	int payload_size;
	int i, j;

	payload = (char *)&datagram->server_header;

	if (datagram->server_header.flags & IPERF_FLAG_HDR_VER1)
		payload += IPERF_SERVER_HDR1_SIZE;
	else
		payload += IPERF_SERVER_HDR0_SIZE;

	payload_size = payload - (char *)datagram;

	for (i = 0, j = 0 ; i < payload_size ; i++)
	{
		payload[i] = '0' + j;

		if (++j == 10)
			j = 0;
	}
}

static void iperf_udp_server_report (iperf_time_t start_time, iperf_time_t stop_time,
										double jitter, int32_t total, int32_t datagrams,
										int32_t errors, int32_t outoforder)
{
	iperf_time_t interval = stop_time - start_time;
	uint32_t byte = datagrams * IPERF_DEFAULT_UDP_DATAGRAM_SIZE;
	uint32_t bps = byte_to_bps(interval, byte);

	iperf_log("  %4.1f ~ %4.1f sec  %7sBytes  %7sbits/sec  %6.3f ms  %4d/%5d (%.2g%%)",
					start_time, stop_time,
					byte_to_string(byte), bps_to_string(bps),
					jitter * 1000,
					errors, total,
					(errors * 100.) / total);

	if (outoforder > 0)
		iperf_log("\t OUT_OF_ORDER(%d)\n", outoforder);
	else
		iperf_log("\n");
}

static void iperf_udp_server_report_to_client (iperf_udp_server_info_t *info)
{
	const bool server_header_ver1 = true;
	char buf[IPERF_DEFAULT_UDP_DATAGRAM_SIZE];
	iperf_udp_datagram_t *datagram = (iperf_udp_datagram_t *)buf;
	iperf_time_t total_time;
	uint64_t total_len;

//	iperf_log(" Report to client\n");

	total_time = info->stop_time - info->start_time;
	total_len = info->datagram_cnt;
   	total_len *= IPERF_DEFAULT_UDP_DATAGRAM_SIZE;

	memcpy(datagram, &info->last_datagram, 12);

	datagram->server_header.flags = server_header_ver1 ? htonl(IPERF_FLAG_HDR_VER1) : 0;
	datagram->server_header.total_len1 = htonl((total_len >> 32) & 0xffffffff);
	datagram->server_header.total_len2 = htonl(total_len & 0xffffffff);
	datagram->server_header.stop_sec = htonl((int32_t)total_time);
	datagram->server_header.stop_usec = htonl((int32_t)((total_time - ntohl(datagram->server_header.stop_sec)) * 1000000));
	datagram->server_header.error_cnt = htonl(info->error_cnt) ;
	datagram->server_header.outorder_cnt = htonl(info->outoforder_cnt);
	datagram->server_header.datagrams = htonl(info->datagram_seq);
	datagram->server_header.jitter1 = htonl((int32_t)info->jitter);
	datagram->server_header.jitter2 = htonl((int32_t)((info->jitter - ntohl(datagram->server_header.jitter1)) * 1000000));

	if (server_header_ver1)
	{
		/* -e, --enhancedreports
		 *  use enhanced reporting giving more tcp/udp and traffic information
		 */
		datagram->server_header.minTransit1 = htonl(0);
		datagram->server_header.minTransit2 = htonl(0);
		datagram->server_header.maxTransit1 = htonl(0);
		datagram->server_header.maxTransit2 = htonl(0);
		datagram->server_header.sumTransit1 = htonl(0);
		datagram->server_header.sumTransit2 = htonl(0);
		datagram->server_header.meanTransit1 = htonl(0);
		datagram->server_header.meanTransit2 = htonl(0);
		datagram->server_header.m2Transit1 = htonl(0);
		datagram->server_header.m2Transit2 = htonl(0);
		datagram->server_header.vdTransit1 = htonl(0);
		datagram->server_header.vdTransit2 = htonl(0);
		datagram->server_header.cntTransit = htonl(0);

		datagram->server_header.IPGcnt = htonl(info->datagram_cnt / (total_time / 1000000));
		datagram->server_header.IPGsum = htonl(1);
	}

	iperf_udp_server_init_payload(datagram);

	iperf_socket_send(&info->client, (char *)datagram, sizeof(buf));
}

static void iperf_udp_server_measure_jitter (int32_t id,
											iperf_time_t tx_time, iperf_time_t rx_time,
											iperf_time_t *jitter, iperf_time_t *transit)
{
	iperf_time_t prev_jitter = *jitter;
	iperf_time_t prev_transit = *transit;
	iperf_time_t cur_transit, diff_transit;

	if (id == 0)
	{
		prev_jitter = 0;
		prev_transit = 0;
	}

	cur_transit = rx_time - tx_time;
	diff_transit = cur_transit - prev_transit;
	prev_transit = cur_transit;

	if (diff_transit < 0)
		diff_transit = -diff_transit;

	*jitter += (diff_transit - prev_jitter) / 16.0;
	*transit = prev_transit;
}

static int iperf_udp_server_run (iperf_opt_t *option)
{
	iperf_udp_server_info_t *info = &g_iperf_udp_server_info;
	iperf_time_t current_time, elapse_time, report_time;
	int32_t report_total, report_success, report_error, report_outoforder;
	int i;

	iperf_log("[ IPERF UDP Server ]\n");

	iperf_udp_server_init_info(info, option);

	for (i = 0 ; ; i++)
	{
		usleep(1000);

		if (info->start)
			break;

/*		if (i > 0 && (i % 1000) == 0)
			iperf_log("Waiting ... %d sec\n", i / 1000); */
	}

	iperf_log("   Interval         Transfer       Bandwidth        Jitter     Lost/Total Datagrams\n");

	report_time = info->report_interval;

	for (report_total = report_success = report_error = report_outoforder = 0; !info->done ; )
	{
		iperf_get_time(&current_time);

		elapse_time = current_time - info->start_time;

		if (info->report_interval > 0 && elapse_time < info->send_time)
		{
			if (elapse_time >= report_time)
			{
				iperf_udp_server_report(report_time - info->report_interval, elapse_time,
										info->jitter,
										info->datagram_seq - report_total,
										info->datagram_cnt - report_success,
										info->error_cnt - report_error,
										info->outoforder_cnt - report_outoforder);

				report_time = elapse_time + info->report_interval;
				report_total = info->datagram_seq;
				report_success = info->datagram_cnt;
				report_error = info->error_cnt;
				report_outoforder = info->outoforder_cnt;
			}
		}

		usleep(1000);
	}

	elapse_time = info->stop_time - info->start_time;

	iperf_udp_server_report(report_time - info->report_interval, elapse_time,
							info->jitter,
							info->datagram_seq - report_total,
							info->datagram_cnt - report_success,
							info->error_cnt - report_error,
							info->outoforder_cnt - report_outoforder);

	iperf_udp_server_report(0, elapse_time,
							info->jitter,
							info->datagram_seq,
							info->datagram_cnt,
							info->error_cnt,
							info->outoforder_cnt);

	iperf_udp_server_report_to_client(info);

	iperf_log(" Done: %d/%d\n", info->datagram_cnt, info->datagram_seq);

	sleep(1);

	return 0;
}

static void iperf_udp_server_recv (iperf_socket_t *socket, char *buf, int len)
{
	iperf_time_t rx_time;

	iperf_get_time(&rx_time);

	if (len == IPERF_DEFAULT_UDP_DATAGRAM_SIZE)
	{
		iperf_udp_server_info_t *info = &g_iperf_udp_server_info;
		iperf_udp_datagram_t *datagram = (iperf_udp_datagram_t *)buf;

		int32_t id = ntohl(datagram->id);
		iperf_time_t tx_time = ntohl(datagram->tv_sec) + ntohl(datagram->tv_usec) / 1000000.;

		if (id == 0)
		{
			int32_t amount = ntohl(datagram->client_header.mAmount);

			if (amount >= 0)
				info->send_byte = amount;
			else
				info->send_time = -amount / 100; /* sec */

			memcpy(&info->client, socket, sizeof(iperf_socket_t));

			info->start_time = rx_time;

			info->datagram_cnt = 0;
			info->datagram_seq = 0;
			info->error_cnt = 0;

			info->start = true;

			iperf_log(" Connected with client: %s port %u\n",
							info->client.remote_addr, info->client.remote_port);
		}
		else
		{
			info->datagram_cnt++;
			info->datagram_seq++;

			if (id < 0)
			{
				id = -id;
				info->stop_time = rx_time;
				memcpy(&info->last_datagram, datagram, 12);
				info->done = true;

				if (info->datagram_seq != id)
				{
/*					iperf_debug("last seq: %d -> %d\n", info->datagram_seq, id); */

					info->datagram_seq = id;
				}
			}
			else
			{
				if (id < info->datagram_seq)
				{
/*					iperf_debug("out of order: %d -> %d\n", info->datagram_seq, id); */

					info->outoforder_cnt++;
					info->datagram_seq = id;
				}
				else if (id > info->datagram_seq)
				{
/*					iperf_debug("lost datagrams: %d, %d -> %d\n", id - info->datagram_seq, info->datagram_seq, id); */

					info->error_cnt += id - info->datagram_seq;
					info->datagram_seq = id;
				}
			}
		}

		iperf_udp_server_measure_jitter(id, tx_time, rx_time, &info->jitter, &info->transit);
	}
	else
		iperf_error("len=%d\n", len);
}

static void iperf_udp_server_recv_callback (atcmd_rxd_t *rxd, char *data)
{
	iperf_socket_t socket;

	iperf_socket_init(&socket);

	socket.id = rxd->id;
	socket.protocol = IPERF_PROTO_UDP;
	socket.local_port = 0;
	socket.remote_port = rxd->remote_port;
	nrc_atcmd_param_to_str(rxd->remote_addr, socket.remote_addr, sizeof(socket.remote_addr));

/*	iperf_debug("server_rx, id=%d len=%d remote=%s,%d\n",
				rxd->id, rxd->len, rxd->remote_addr, rxd->remote_port); */

	iperf_udp_server_recv(&socket, data, rxd->len);
}

/**********************************************************************************************/

static iperf_udp_client_info_t g_iperf_udp_client_info;

static void iperf_udp_client_init_info (iperf_udp_client_info_t *info, iperf_opt_t *option)
{
	memset(info, 0, sizeof(iperf_udp_client_info_t));

	info->done = false;
	info->datagram_cnt = 0;

	info->server_port = IPERF_DEFAULT_SERVER_PORT;
	info->datagram_size = IPERF_DEFAULT_UDP_DATAGRAM_SIZE;
	info->send_time = IPERF_DEFAULT_SEND_TIME;
	info->report_interval = IPERF_DEFAULT_REPORT_INTERVAL;

	if (option)
	{
		if (option->server_port > 0)
			info->server_port = option->server_port;

		if (option->datagram_size > 0)
			info->datagram_size = option->datagram_size;

		if (option->send_time > 0)
			info->send_time = option->send_time;

		if (option->report_interval > 0)
			info->report_interval = option->report_interval;
	}
}

static void iperf_udp_client_init_payload (iperf_udp_datagram_t *datagram, int32_t datagram_size)
{
	char *payload;
	int payload_size;
	int i, j;

	payload = (char *)&datagram->client_header;

	if (datagram->client_header.flags & IPERF_FLAG_HDR_VER1)
		payload += IPERF_CLIENT_HDR1_SIZE;
	else
		payload += IPERF_CLIENT_HDR0_SIZE;

	payload_size = datagram_size - (payload - (char *)datagram);

	for (i = 0, j = 0 ; i < payload_size ; i++)
	{
		payload[i] = '0' + j;

		if (++j == 10)
			j = 0;
	}
}

static void iperf_udp_client_init_datagram (iperf_udp_client_info_t *info,
											iperf_udp_datagram_t *datagram)
{
	bool client_header_ver1 = false;

	memset(datagram, 0, info->datagram_size);

	datagram->id = htonl(0);
	datagram->tv_sec = htonl(0);
	datagram->tv_usec = htonl(0);

	datagram->client_header.flags = htonl(client_header_ver1 ? IPERF_FLAG_HDR_VER1 : 0);
	datagram->client_header.numThreads = htonl(1);
	datagram->client_header.mPort = htonl(info->server_port);
	datagram->client_header.bufferlen = htonl(0);
	datagram->client_header.mWindowSize = htonl(0);
	datagram->client_header.mAmount = htonl(-(info->send_time * 100));
	if (client_header_ver1)
	{
		datagram->client_header.mRate = htonl(IPERF_DEFAULT_UDP_RATE);
		datagram->client_header.mUDPRateUnits = htonl(0);
		datagram->client_header.mRealTime = htonl(0);
	}

	iperf_udp_client_init_payload(datagram, info->datagram_size);
}

static void iperf_udp_client_report (iperf_time_t start_time, iperf_time_t end_time, int datagrams)
{
	iperf_time_t interval = end_time - start_time;
	uint32_t byte = datagrams * IPERF_DEFAULT_UDP_DATAGRAM_SIZE;
	uint32_t bps = byte_to_bps(interval, byte);

	iperf_log("  %4.1f ~ %4.1f sec  %7sBytes  %7sbits/sec\n",
					start_time, end_time,
					byte_to_string(byte), bps_to_string(bps));
}

/* static void iperf_udp_client_report_from_server (iperf_server_header_t *header)
{
	iperf_time_t time = ntohl(header->stop_sec) + (ntohl(header->stop_usec) / 1000000.);
	int32_t byte = ntohl(header->total_len2);
	int32_t datagrams = ntohl(header->datagrams);
	int32_t errors = ntohl(header->error_cnt);
	int32_t jitter = (ntohl(header->jitter1) * 1000000) + ntohl(header->jitter2);

	iperf_log(" Server Report:\n");
	iperf_log("  %4.1f ~ %4.1f sec  %7sBytes  %7sbits/sec  %.3f ms  %d/%d (%.2g%%)\n",
					0., time,
					byte_to_string(byte),
					bps_to_string(byte_to_bps(time, byte)),
					jitter / 1000.,
					errors, datagrams,
					(errors * 100.) / datagrams);
} */

static int iperf_udp_client_run (iperf_socket_t *socket, iperf_opt_t *option)
{
	iperf_udp_client_info_t *info = &g_iperf_udp_client_info;
	iperf_time_t start_time, stop_time, current_time, elapse_time, report_time;
	iperf_udp_datagram_t *datagram;
	int i, j;

	iperf_log("[ IPERF UDP Client ]\n");

	if (!socket)
		return -1;

	iperf_udp_client_init_info(info, option);

	datagram = (iperf_udp_datagram_t *)malloc(info->datagram_size);
	if (!datagram)
		return -1;

	iperf_udp_client_init_datagram(info, datagram);

	iperf_log(" Sending %d byte datagrams ...\n", info->datagram_size);
	iperf_log("   Interval        Transfer      Bandwidth\n");

	for (i = 0, j = 0 ; i >= 0 ; i++, j++)
	{
		iperf_get_time(&current_time);

		if (i == 0)
		{
			start_time = current_time;
			stop_time = start_time + info->send_time;
			report_time = info->report_interval;
		}
		else
		{
			elapse_time = current_time - start_time;

			if (info->report_interval > 0 && elapse_time >= report_time)
			{
				iperf_udp_client_report(report_time - info->report_interval, elapse_time, j);

				report_time = elapse_time + info->report_interval;
				j = 0;
			}

			if (current_time >= stop_time)
			{
				iperf_udp_client_report(report_time - info->report_interval, elapse_time, j);
				iperf_udp_client_report(0, elapse_time, i);

				iperf_log(" Sent %u datagrams\n", i);

				info->datagram_cnt = i;

				i = ~(i - 1);
			}
		}

		datagram->id = htonl(i);
		datagram->tv_sec = htonl((int32_t)current_time);
		datagram->tv_usec = htonl((int32_t)((current_time - ntohl(datagram->tv_sec)) *1000000));

		iperf_udp_datagram_print(datagram, info->datagram_size, false, true);

		if (option->passthrough)
			iperf_socket_send_passthrough(socket, (char *)datagram, info->datagram_size, option->negative);
		else
			iperf_socket_send(socket, (char *)datagram, info->datagram_size);
	}

	for (i = 0 ; i < 10 ; i++)
	{
		if (info->done)
		{
			iperf_log(" Done\n");
			break;
		}

		usleep(100 * 1000);
	}


	if (i >= 10)
		iperf_log("Report from server: timeout\n");

	free((char *)datagram);
	datagram = NULL;

	return 0;
}

static void iperf_udp_client_recv (iperf_socket_t *socket, char *buf, int len)
{
	iperf_udp_client_info_t *info = &g_iperf_udp_client_info;

	if (len == IPERF_DEFAULT_UDP_DATAGRAM_SIZE)
	{
		if (info->datagram_cnt > 0)
		{
			iperf_udp_datagram_t *datagram = (iperf_udp_datagram_t *)&buf[0];

			if (ntohl(datagram->id) == -info->datagram_cnt)
			{
/*				iperf_udp_client_report_from_server(&datagram->server_header); */
				info->done = true;
			}
		}
	}
}

static void iperf_udp_client_recv_callback (atcmd_rxd_t *rxd, char *data)
{
	iperf_socket_t socket;

	iperf_socket_init(&socket);

	socket.id = rxd->id;
	socket.protocol = IPERF_PROTO_UDP;
	socket.local_port = 0;
	socket.remote_port = rxd->remote_port;
	nrc_atcmd_param_to_str(rxd->remote_addr, socket.remote_addr, sizeof(socket.remote_addr));

/*	iperf_debug("client_rx, id=%d len=%d remote=%s,%d\n",
				rxd->id, rxd->len, rxd->remote_addr, rxd->remote_port); */

	iperf_udp_client_recv(&socket, data, rxd->len);
}

static void iperf_udp_client_event_callback (enum ATCMD_EVENT event, int argc, char *argv[])
{
/*	iperf_udp_client_info_t *info = &g_iperf_udp_client_info; */

	switch (event)
	{
		case ATCMD_SEVENT_SEND_IDLE:
			g_iperf_socket_send_idle = true;

			if (argc != 2)
				break;

/*			iperf_debug("sevent_send_idle: id=%d\n", atoi(argv[0])); */
			break;

		default:
			break;
	}
}

/**********************************************************************************************/

static iperf_tcp_server_info_t g_iperf_tcp_server_info;

static void iperf_tcp_server_init_info (iperf_tcp_server_info_t *info, iperf_opt_t *option)
{
	memset(info, 0, sizeof(iperf_tcp_server_info_t));

	info->client.id = -1;

	info->start = false;
	info->done = false;

	info->server_port = IPERF_DEFAULT_SERVER_PORT;
	info->report_interval = IPERF_DEFAULT_REPORT_INTERVAL;

	if (option)
	{
		if (option->server_port > 0)
			info->server_port = option->server_port;

		if (option->report_interval > 0)
			info->report_interval = option->report_interval;
	}
};

static void iperf_tcp_server_report (iperf_time_t start_time, iperf_time_t stop_time, uint32_t recv_byte)
{
	iperf_time_t interval = stop_time - start_time;
	uint32_t byte = recv_byte;
	uint32_t bps = byte_to_bps(interval, byte);

	iperf_log("  %4.1f ~ %4.1f sec  %7sBytes  %7sbits/sec\n",
					start_time, stop_time,
					byte_to_string(byte), bps_to_string(bps));
}

static int iperf_tcp_server_run (iperf_opt_t *option)
{
	iperf_tcp_server_info_t *info = &g_iperf_tcp_server_info;
	iperf_time_t current_time, elapse_time, report_time;
	uint32_t recv_byte, report_recv_byte;
	int i;

	iperf_log("[ IPERF TCP Server ]\n");

	iperf_tcp_server_init_info(info, option);

	for (i = 0 ; ; i++)
	{
		usleep(1000);

		if (info->start)
			break;

/*		if (i > 0 && (i % 1000) == 0)
			iperf_debug("Waiting ... %d sec\n", i / 1000); */
	}

	iperf_log("   Interval         Transfer       Bandwidth\n");

	elapse_time = 0;
	report_time = info->report_interval;

	for (report_recv_byte = 0; elapse_time < info->send_time || !info->done ; )
	{
		iperf_get_time(&current_time);

		elapse_time = current_time - info->start_time;

		if (info->report_interval > 0 && elapse_time < info->send_time)
		{
			if (elapse_time >= report_time)
			{
				recv_byte = info->recv_byte;

				iperf_tcp_server_report(report_time - info->report_interval, elapse_time,
										recv_byte - report_recv_byte);

				report_time = elapse_time + info->report_interval;
				report_recv_byte = recv_byte;
			}
		}

		usleep(1000);
	}

	iperf_tcp_server_report(report_time - info->report_interval, elapse_time,
							info->recv_byte - report_recv_byte);

	iperf_tcp_server_report(0, elapse_time, info->recv_byte);

	iperf_log(" Done\n");

	return 0;
}

static void iperf_tcp_server_recv (iperf_socket_t *socket, char *buf, int len)
{
	iperf_time_t rx_time;

	iperf_get_time(&rx_time);

	if (len > 0)
	{
		iperf_tcp_server_info_t *info = &g_iperf_tcp_server_info;

		if (info->recv_byte == 0)
		{
			iperf_tcp_datagram_t *datagram = (iperf_tcp_datagram_t *)buf;
			int32_t amount = ntohl(datagram->client_header.mAmount);

			if (amount >= 0)
				info->send_byte = amount;
			else
				info->send_time = -amount / 100; /* sec */

			info->start_time = rx_time;
			info->start = true;
			
			memcpy(&info->client, socket, sizeof(iperf_socket_t));
			
			iperf_log(" Connected with client: %s port %u\n",
							info->client.remote_addr, info->client.remote_port);
		}

		info->recv_byte += len;
	}
	else
		iperf_error("len=%d\n", len);
}

static void iperf_tcp_server_recv_callback (atcmd_rxd_t *rxd, char *data)
{
	iperf_tcp_server_info_t *info = &g_iperf_tcp_server_info;

	if (info->client.id > 0 && info->client.id == rxd->id)
	{	
		iperf_socket_t socket;

		iperf_socket_init(&socket);

		socket.id = rxd->id;
		socket.protocol = IPERF_PROTO_TCP;
		socket.local_port = 0;
		socket.remote_port = rxd->remote_port;
		nrc_atcmd_param_to_str(rxd->remote_addr, socket.remote_addr, sizeof(socket.remote_addr));

/*		iperf_debug("server_rx, id=%d len=%d remote=%s,%d\n",
			rxd->id, rxd->len, rxd->remote_addr, rxd->remote_port); */

		iperf_tcp_server_recv(&socket, data, rxd->len);
	}
}

static void iperf_tcp_server_event_callback (enum ATCMD_EVENT event, int argc, char *argv[])
{
	iperf_tcp_server_info_t *info = &g_iperf_tcp_server_info;
	int id;
	int err;

	switch (event)
	{
		case ATCMD_SEVENT_CONNECT:
			if (argc != 1)
			{
				iperf_debug("SEVENT_CONNECT: argc=%d\n", argc);
				break;
			}

			info->client.id = atoi(argv[0]);

/*			iperf_log(" Connected with client: id=%d\n", info->client.id); */
			break;

		case ATCMD_SEVENT_CLOSE:
			if (argc != 1)
				break;

			id = atoi(argv[0]);

/*			iperf_debug("sevent_close: id=%d\n", id); */

			if (id == info->client.id)
				info->done = true;
			break;

		case ATCMD_SEVENT_RECV_ERROR:
			if (argc != 2)
				break;

			id = atoi(argv[0]);
			err = atoi(argv[1]);

/*			iperf_debug("sevent_recv_error: id=%d err=%d\n", id, err); */

			if (id == info->client.id)
			{
				switch (err)
				{
					case ENOTCONN:
						info->done = true;
				}
			}
			break;

		default:
			break;
	}
}

/**********************************************************************************************/

static iperf_tcp_client_info_t g_iperf_tcp_client_info;

static void iperf_tcp_client_init_info (iperf_tcp_client_info_t *info, iperf_opt_t *option)
{
	memset(info, 0, sizeof(iperf_tcp_client_info_t));

	info->done = false;
	info->datagram_cnt = 0;

	info->server_port = IPERF_DEFAULT_SERVER_PORT;
	info->datagram_size = IPERF_DEFAULT_TCP_DATA_SIZE;
	info->send_time = IPERF_DEFAULT_SEND_TIME;
	info->report_interval = IPERF_DEFAULT_REPORT_INTERVAL;

	if (option)
	{
		if (option->server_port > 0)
			info->server_port = option->server_port;

		if (option->datagram_size > 0)
			info->datagram_size = option->datagram_size;

		if (option->send_time > 0)
			info->send_time = option->send_time;

		if (option->report_interval > 0)
			info->report_interval = option->report_interval;
	}
}

static void iperf_tcp_client_init_payload (iperf_tcp_datagram_t *datagram, int32_t datagram_size)
{
	char *payload;
	int payload_size;
	int i, j;

	payload = (char *)datagram + IPERF_CLIENT_HDR0_SIZE;
	payload_size = datagram_size - IPERF_CLIENT_HDR0_SIZE;

	for (i = 0, j = 0 ; i < payload_size ; i++)
	{
		payload[i] = '0' + j;

		if (++j == 10)
			j = 0;
	}
}

static void iperf_tcp_client_init_datagram (iperf_tcp_client_info_t *info, iperf_tcp_datagram_t *datagram)
{
	memset(datagram, 0, info->datagram_size);

	datagram->client_header.flags = htonl(0);
	datagram->client_header.numThreads = htonl(1);
	datagram->client_header.mPort = htonl(info->server_port);
	datagram->client_header.bufferlen = htonl(0);
	datagram->client_header.mWindowSize = htonl(0);
	datagram->client_header.mAmount = htonl(-(info->send_time * 100));

	iperf_tcp_client_init_payload(datagram, info->datagram_size);
}

static void iperf_tcp_client_report (iperf_time_t start_time, iperf_time_t end_time, int datagram_cnt, int datagram_size)
{
	iperf_time_t interval = end_time - start_time;
	uint32_t byte = datagram_cnt * datagram_size;
	uint32_t bps = byte_to_bps(interval, byte);

	iperf_log("  %4.1f ~ %4.1f sec  %7sBytes  %7sbits/sec\n",
					start_time, end_time,
					byte_to_string(byte), bps_to_string(bps));
}

static int iperf_tcp_client_run (iperf_socket_t *socket, iperf_opt_t *option) // sec
{
	iperf_tcp_client_info_t *info = &g_iperf_tcp_client_info;
	iperf_time_t start_time, stop_time, current_time, elapse_time, report_time;
	iperf_tcp_datagram_t *datagram;
	int i, j;

	iperf_log("[ IPERF TCP Client ]\n");

	if (!socket)
		return -1;

	iperf_tcp_client_init_info(info, option);

	datagram = (iperf_tcp_datagram_t *)malloc(info->datagram_size);
	if (!datagram)
		return -1;

	iperf_tcp_client_init_datagram(info, datagram);

	iperf_log(" Sending %d byte datagram ...\n", info->datagram_size);
	iperf_log("   Interval        Transfer      Bandwidth\n");

	for (i = 0, j = 0 ; i >= 0 ; i++, j++)
	{
		iperf_get_time(&current_time);

		if (i == 0)
		{
			start_time = current_time;
			stop_time = start_time + info->send_time;
			report_time = info->report_interval;
		}
		else
		{
			elapse_time = current_time - start_time;

			if (info->report_interval > 0 && elapse_time >= report_time)
			{
				iperf_tcp_client_report(report_time - info->report_interval, elapse_time, j, info->datagram_size);

				report_time = elapse_time + info->report_interval;
				j = 0;
			}

			if (current_time >= stop_time)
			{
				iperf_tcp_client_report(report_time - info->report_interval, elapse_time, j, info->datagram_size);
				iperf_tcp_client_report(0, elapse_time, i, info->datagram_size);

				iperf_log(" Sent %u datagrams\n", i);

				info->datagram_cnt = i;

				i = ~(i - 1);
			}
		}

		if (option->passthrough)
			iperf_socket_send_passthrough(socket, (char *)datagram, info->datagram_size, option->negative);
		else
			iperf_socket_send(socket, (char *)datagram, info->datagram_size);
	}

	if (option->passthrough)
		info->done = false;
	else
		info->done = true;

	for (i = 0 ; i < 10 ; i++)
	{
		if (info->done)
			break;

		usleep(100 * 1000);
	}

	iperf_log(" Done\n");

	free((char *)datagram);
	datagram = NULL;

	return 0;
}

static void iperf_tcp_client_recv_callback (atcmd_rxd_t *rxd, char *data)
{
	iperf_socket_t socket;

	iperf_socket_init(&socket);

	socket.id = rxd->id;
	socket.protocol = IPERF_PROTO_TCP;
	socket.local_port = 0;
	socket.remote_port = rxd->remote_port;
	nrc_atcmd_param_to_str(rxd->remote_addr, socket.remote_addr, sizeof(socket.remote_addr));

	iperf_debug("client_rx, id=%d len=%d remote=%s,%d\n",
				rxd->id, rxd->len, rxd->remote_addr, rxd->remote_port);
}

static void iperf_tcp_client_event_callback (enum ATCMD_EVENT event, int argc, char *argv[])
{
	iperf_tcp_client_info_t *info = &g_iperf_tcp_client_info;

	switch (event)
	{
		case ATCMD_SEVENT_SEND_IDLE:
			g_iperf_socket_send_idle = true;

			if (argc != 2)
				break;

/*			iperf_debug("sevent_send_idle: id=%d\n", atoi(argv[0])); */

			info->done = true;
			break;

		default:
			break;
	}
}

/**********************************************************************************************/

static void iperf_option_init (iperf_opt_t *option)
{
	memset(option, 0, sizeof(iperf_opt_t));

	option->server = true;
	option->udp = false;
	option->passthrough = false;
	option->negative = false;

	option->server_port = IPERF_DEFAULT_SERVER_PORT;
	strcpy(option->server_ip, IPERF_IPADDR_ANY);

	option->send_time = IPERF_DEFAULT_SEND_TIME;
	option->report_interval = IPERF_DEFAULT_REPORT_INTERVAL;
	option->datagram_size = IPERF_DEFAULT_UDP_DATAGRAM_SIZE;
}

static void iperf_option_help (char *cmd)
{
	iperf_log("Usage: %s <-s|-c host> [options]\n", cmd);
	iperf_log("\r\n");
	iperf_log("Client/Server:\n");
	iperf_log("  -i, --interval #      seconds between periodic bandwidth reports (default: %d sec)\n",	IPERF_DEFAULT_REPORT_INTERVAL);
	iperf_log("  -p, --port #          server port to listen on/connect to (default: %d)\n", IPERF_DEFAULT_SERVER_PORT);
	iperf_log("  -u, --udp             use UDP rather than TCP\n");
	iperf_log("\r\n");

	iperf_log("Server specific:\n");
	iperf_log("  -s, --server          run in server mode\n");
	iperf_log("\r\n");

	iperf_log("Client specific:\n");
	iperf_log("  -c, --client <host>   run in client mode, connecting to <host>\n");
	iperf_log("  -t, --time #          time in seconds to transmit for (default: %d sec)\n", IPERF_DEFAULT_SEND_TIME);
	iperf_log("  -P, --passthrough     transmit in passthrough mode\n");
	iperf_log("  -N, --negative        use negative length in passthrough mode (always negative in UDP)\n");
	iperf_log("\r\n");

	iperf_log("Miscellaneous:\n");
	iperf_log("  -h, --help            print this message and quit\n");
	iperf_log("\r\n");
}

static void iperf_option_print (iperf_opt_t *option)
{
	iperf_log("[ IPERF OPTION ]\n");
	iperf_log(" - role: %s\n", option->server ? "server" : "client");
	iperf_log(" - protocol: %s\n", option->udp ? "udp" : "tcp");
	iperf_log(" - server_port: %d\n", option->server_port);
	if (!option->server)
	{
		iperf_log(" - server_ip: %s\n", option->server_ip);
		iperf_log(" - send_time: %d\n", option->send_time);
		iperf_log(" - send_passthrough: %s %s\n", option->passthrough ? "on" : "off",
												option->negative ? "(-)" : "");
		if (option->udp)
			iperf_log(" - datagram_size: %d\n", option->datagram_size);
	}
	iperf_log(" - report_interval: %d\n", option->report_interval);
	iperf_log("\r\n");
}

static int iperf_option_parse (int argc, char *argv[], iperf_opt_t *option)
{
	static struct option opt_info[] =
	{
		/* Client/Server */
		{ "interval",		required_argument,		0, 	'i' },
		{ "port",			required_argument,		0, 	'p' },
		{ "udp",			no_argument,			0, 	'u' },

		/* Server */
		{ "server",			no_argument,			0, 	's' },

		/* Client */
		{ "client",			required_argument,		0, 	'c' },
		{ "time",			required_argument,		0, 	't' },
		{ "passthrough",	no_argument,	    	0, 	'P' },
		{ "negative",		no_argument,			0, 	'N' },

		/* Miscellaneous */
		{ "help",			no_argument,			0,	'h' },

		{ 0, 0, 0, 0 }
	};
	int opt_idx = 0;
	int ret;

	iperf_option_init(option);

	for (optind = 0 ; ; )
	{
		ret = getopt_long(argc, argv, "i:p:usc:t:PNh", opt_info, &opt_idx);

		switch (ret)
		{
			case -1: // end
				if (option->server)
					option->passthrough = false;

				if (!option->passthrough)
					option->negative = false;
				else if (option->udp)
					option->negative = true;

			   	iperf_option_print(option);
				return 0;

			/*
			 * Client/Server 
			 */
			case 'i': // interval
				option->report_interval = atoi(optarg);
				break;

			case 'p': // port
				option->server_port = atoi(optarg);
				break;

			case 'u': // udp
				option->udp = true;
				break;

			/*
			 * Server 
			 */
			case 's': // server
				option->server = true;
				break;

			/*
			 * Client 
			 */
			case 'c': // client
				option->server = false;

				if (strlen(optarg) > IPERF_IPADDR_LEN_MAX)
					return -1;

				if (!iperf_is_ip4addr(optarg))
					return -1;

				strcpy(option->server_ip, optarg);
				break;

			case 't': // time
				option->send_time = atoi(optarg);
				break;

			case 'P': // passthrough
				option->passthrough = true;
				break;

			case 'N': // negative
				option->negative = true;
				break;

			/*
			 * Miscellaneous 
			 */
			case 'h': // help
				iperf_option_help(argv[0]);
				return 1;

			default:
				iperf_error("ret=%c(0x%X), optind=%d optopt=%c\n", ret, ret, optind, optopt);
				return -1;
		}
	}
}

static int _iperf_main (int argc, char *argv[])
{
	iperf_opt_t option;
	iperf_socket_t socket;

	if (argc <= 1 || strcmp(argv[0], "iperf") != 0)
	{
		iperf_option_help("iperf");
		return -1;
	}

	iperf_log("\r\n");

	switch (iperf_option_parse(argc, argv, &option))
	{
		case 0:
			break;

		case 1: /* help */
			return 0;

		default:
			iperf_log("invalid option\n");
			return -1;
	}

/*	iperf_socket_init(&socket); */

	socket.id = -1;
	socket.protocol = option.udp ? IPERF_PROTO_UDP : IPERF_PROTO_TCP;
	if (option.server)
	{
		socket.local_port = option.server_port;
		socket.remote_port = 0;
		strcpy(socket.remote_addr, IPERF_IPADDR_ANY);
	}
	else
	{
		if (option.udp)
			socket.local_port = IPERF_DEFAULT_CLIENT_PORT;
		else
			socket.local_port = 0;
		socket.remote_port = option.server_port;
		strcpy(socket.remote_addr, option.server_ip);
	}

/*	iperf_socket_print(&socket); */

	iperf_log("\r\n");

	if (iperf_socket_open(&socket) == 0)
	{
		iperf_log("\r\n");

		nrc_atcmd_log_off();

		if (option.server)
		{
			bool server_exit = false;
			char buf[16];
			int len;

			while (!server_exit)
			{
				if (option.udp)
					iperf_udp_server_run(&option);
				else
					iperf_tcp_server_run(&option);

				while (1)
				{
					iperf_log("\nPress ENTER key or Input \"quit\" : ");

					if (fgets(buf, sizeof(buf), stdin) == NULL)
						continue;

					len = strlen(buf) - 1;
					buf[len] = '\0';

					if (len == 0)
					{
						iperf_log("\n");
						break;
					}
					else if (strcmp(buf, "quit") == 0)
					{
						server_exit = true;
						break;
					}
				}
			}
		}
		else
		{
			if (option.udp)
				iperf_udp_client_run(&socket, &option);
			else
				iperf_tcp_client_run(&socket, &option);
		}

		nrc_atcmd_log_on();

		iperf_log("\r\n");

		iperf_socket_close(&socket);
	}

	return 0;
}

int iperf_main (char *cmd)
{
#define IPERF_OPTION_MAX	10

	char *argv[IPERF_OPTION_MAX + 1];
	int argc;

	for (argv[0] = cmd, argc = 1 ; *cmd != '\0' ; cmd++)
	{
		if (argc > (IPERF_OPTION_MAX + 1))
		{
			iperf_error("maximum option number: %d\n", IPERF_OPTION_MAX);
			return -1;
		}

		if (*cmd == ' ')
		{
			*cmd = '\0';
			argv[argc++] = cmd + 1;
		}
	}

	return _iperf_main(argc, argv);
}

