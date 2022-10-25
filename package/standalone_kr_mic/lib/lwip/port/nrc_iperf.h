/*
 * MIT License
 *
 * Copyright (c) 2021 Newracom, Inc.
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


#ifndef __NRC_IPERF_H__
#define __NRC_IPERF_H__

#include "lwip/sockets.h"

/**********************************************************************************************/
#define IPERF_DEFAULT_SERVER_PORT			5001
#define IPERF_DEFAULT_SEND_TIME				10

#define IPERF_DEFAULT_UDP_RATE				(1024 * 1024) // bps
#define IPERF_DEFAULT_DATA_BUF_LEN			1470
#define IPERF_DEFAULT_UDP_DATAGRAM_SIZE		IPERF_DEFAULT_DATA_BUF_LEN // byte

#define IPERF_UDP_MAX_RECV_SIZE       (4*1024)        // 4KB

#define RECVFROM_WAITING_TIME_SEC			0 /* sec */
#define RECVFROM_WAITING_TIME_USEC			500000 /* usec */

#define IPERF_FLAG_HDR_VER1				0x80000000
#define IPERF_CLIENT_HDR0_SIZE		(4*6)
#define IPERF_CLIENT_HDR1_SIZE		IPERF_CLIENT_HDR0_SIZE + (4*3)
#define IPERF_SERVER_HDR0_SIZE		(4*10)
#define IPERF_SERVER_HDR1_SIZE		IPERF_SERVER_HDR0_SIZE + (4*15)

/*
 * Definitions for IP type of service (ip_tos)
 */
#ifndef IPTOS_LOWDELAY
# define IPTOS_LOWDELAY          0x10
# define IPTOS_THROUGHPUT        0x08
# define IPTOS_RELIABILITY       0x04
# define IPTOS_LOWCOST           0x02
# define IPTOS_MINCOST           IPTOS_LOWCOST
#endif /* IPTOS_LOWDELAY */

/*
 * Definitions for DiffServ Codepoints as per RFC2474
 */
#ifndef IPTOS_DSCP_AF11
# define	IPTOS_DSCP_AF11		0x28
# define	IPTOS_DSCP_AF12		0x30
# define	IPTOS_DSCP_AF13		0x38
# define	IPTOS_DSCP_AF21		0x48
# define	IPTOS_DSCP_AF22		0x50
# define	IPTOS_DSCP_AF23		0x58
# define	IPTOS_DSCP_AF31		0x68
# define	IPTOS_DSCP_AF32		0x70
# define	IPTOS_DSCP_AF33		0x78
# define	IPTOS_DSCP_AF41		0x88
# define	IPTOS_DSCP_AF42		0x90
# define	IPTOS_DSCP_AF43		0x98
# define	IPTOS_DSCP_EF		0xb8
#endif /* IPTOS_DSCP_AF11 */

#ifndef IPTOS_DSCP_CS0
# define	IPTOS_DSCP_CS0		0x00
# define	IPTOS_DSCP_CS1		0x20
# define	IPTOS_DSCP_CS2		0x40
# define	IPTOS_DSCP_CS3		0x60
# define	IPTOS_DSCP_CS4		0x80
# define	IPTOS_DSCP_CS5		0xa0
# define	IPTOS_DSCP_CS6		0xc0
# define	IPTOS_DSCP_CS7		0xe0
#endif /* IPTOS_DSCP_CS0 */
#ifndef IPTOS_DSCP_EF
# define	IPTOS_DSCP_EF		0xb8
#endif /* IPTOS_DSCP_EF */

#define KILO 1000
#define MEGA ( KILO * KILO )

typedef double iperf_time_t ; // sec

typedef struct
{
	int32_t flags;
	int32_t total_len1;
	int32_t total_len2;
	int32_t stop_sec;
	int32_t stop_usec;
	int32_t error_cnt;
	int32_t outoforder_cnt;
	int32_t datagrams;
	int32_t jitter1;
	int32_t jitter2;

	/* header version 1 */
	int32_t minTransit1;
	int32_t minTransit2;
	int32_t maxTransit1;
	int32_t maxTransit2;
	int32_t sumTransit1;
	int32_t sumTransit2;
	int32_t meanTransit1;
	int32_t meanTransit2;
	int32_t m2Transit1;
	int32_t m2Transit2;
	int32_t vdTransit1;
	int32_t vdTransit2;
	int32_t cntTransit;
	int32_t IPGcnt;
	int32_t IPGsum;
} iperf_server_header_t;

typedef struct
{
	int32_t flags;
	int32_t numThreads;
	int32_t mPort;
	int32_t bufferlen;
	int32_t mWindowSize;
	int32_t mAmount; 		// -: -t option (sec), +: -n option (byte)

	/* header version 1 */
	int32_t mRate; 			// -b option (bits/sec)
	int32_t mUDPRateUnits; 	// 0:BW, 1:PPS (Don't use)
	int32_t mRealTime; 		// Don't care
} iperf_client_header_t;

typedef struct
{
	int32_t id;
	uint32_t tv_sec;
	uint32_t tv_usec;

	union
	{
		iperf_server_header_t server_header;
		iperf_client_header_t client_header;
	};
} iperf_udp_datagram_t;

typedef struct
{
	union
	{
		iperf_server_header_t server_header;
		iperf_client_header_t client_header;
	};
} iperf_tcp_datagram_t;

typedef struct
{
	struct sockaddr_in clientaddr;

	bool start;
	bool done;

	iperf_time_t send_time;
	int32_t send_byte;
	uint64_t recv_byte;
	int32_t datagram_cnt;
	int32_t datagram_seq;
	int32_t error_cnt;
	int32_t outoforder_cnt;

	iperf_time_t jitter;
	iperf_time_t transit;

	iperf_time_t server_base_time;
	iperf_time_t client_base_time;

	iperf_time_t start_time;
	iperf_time_t stop_time;

	int32_t last_id;

	/* options */
	uint16_t server_port;
	int32_t report_interval;
} iperf_server_info_t;

typedef struct
{
	uint32_t datagram_cnt;
	iperf_time_t start_time;
	iperf_time_t end_time;
} iperf_client_info_t;

// server/client mode
enum ThreadMode {
	kMode_Unknown = 0,
	kMode_Server,
	kMode_Client,
	kMode_Max,
};

struct hdr_typelen {
    int32_t type;
    int32_t length;
};

struct client_hdr_ack {
    struct hdr_typelen typelen;
    uint32_t flags;
    uint32_t version_u;
    uint32_t version_l;
    uint32_t reserved1;
    uint32_t reserved2;
};

typedef struct iperf_opt
{
	uint32_t mAmount; // -n or -t
	uint32_t mBufLen;	// -l
	uint32_t mAppRate; // -b
	uint32_t mSock;
	uint16_t mPort; // -p

	ip_addr_t addr;
	union {
		iperf_server_info_t server_info;
		iperf_client_info_t client_info;
	};

	uint8_t mTOS ;	// -S
	enum ThreadMode mThreadMode;         // -s or -c
	bool   mUDP;                    // -u
	bool   mForceStop;                    // -u
} iperf_opt_t;


iperf_opt_t * iperf_option_alloc(void);
void iperf_option_free(iperf_opt_t* option);
int nrc_iperf_list_init(void);
void nrc_iperf_list_deinit(void);
int nrc_iperf_task_list_add(iperf_opt_t* option);
int nrc_iperf_task_list_del(iperf_opt_t* option);
int  iperf_run(int argc, char *argv[], void *report_cb);

int iperf_get_time (iperf_time_t *time);
uint32_t byte_to_bps (iperf_time_t time, uint32_t byte);
char *byte_to_string (uint32_t byte);
char *bps_to_string (uint32_t bps);

#endif /* __NRC_IPERF_H__ */
