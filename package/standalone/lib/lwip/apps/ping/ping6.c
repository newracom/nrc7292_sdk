/*	$NetBSD: ping6.c,v 1.73 2010/09/20 11:49:48 ahoka Exp $	*/
/*	$KAME: ping6.c,v 1.164 2002/11/16 14:05:37 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*	BSDI	ping.c,v 2.3 1996/01/21 17:56:50 jch Exp	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Muuss.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if 0
#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1989, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)ping.c	8.1 (Berkeley) 6/5/93";
#endif /* not lint */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: ping6.c,v 1.73 2010/09/20 11:49:48 ahoka Exp $");
#endif
#endif

/*
 * Using the InterNet Control Message Protocol (ICMP) "ECHO" facility,
 * measure round-trip-delays and packet loss across network paths.
 *
 * Author -
 *	Mike Muuss
 *	U. S. Army Ballistic Research Laboratory
 *	December, 1983
 *
 * Status -
 *	Public Domain.  Distribution Unlimited.
 * Bugs -
 *	More statistics could always be gathered.
 *	This program has to run SUID to ROOT to access the ICMP socket.
 */
/*
 * NOTE:
 * USE_SIN6_SCOPE_ID assumes that sin6_scope_id has the same semantics
 * as IPV6_PKTINFO.  Some people object it (sin6_scope_id specifies *link*
 * while IPV6_PKTINFO specifies *interface*.  Link is defined as collection of
 * network attached to 1 or more interfaces)
 */

#include <stdio.h>

#include "system_common.h"
#include "lwip/icmp6.h"
#include "lwip/if_api.h"
#include "lwip/inet.h"
#include "lwip/ip6.h"
#include "lwip/netdb.h"
#include "lwip/prot/icmp6.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"

#include "ping.h"
#include "ping_task.h"

#if LWIP_PING

extern struct netif *nrc_netif;

struct tv32 {
	u_int32_t tv32_sec;
	u_int32_t tv32_usec;
};

#define IPV6_PKTINFO		50
#define IPV6_HOPLIMIT       52

struct in6_pktinfo {
	struct in6_addr ipi6_addr;
    int     ipi6_ifindex;
};

#define NI_MAXHOST  64
#define MAXPACKETLEN	131072
#define	IP6LEN		40
#define ICMP6ECHOLEN	8	/* icmp echo header len excluding time */
#define ICMP6ECHOTMLEN sizeof(struct tv32)
#define ICMP6_NIQLEN	(ICMP6ECHOLEN + 8)
/* FQDN case, 64 bits of nonce + 32 bits ttl */
#define ICMP6_NIRLEN	(ICMP6ECHOLEN + 12)
#define	EXTRA		256	/* for AH and various other headers. weird. */
#define	DEFDATALEN	ICMP6ECHOTMLEN
#define MAXDATALEN	MAXPACKETLEN - IP6LEN - ICMP6ECHOLEN
#define	NROUTES		9		/* number of record route slots */
#define MAX_FAIL_COUNT 5

#define	A_(bit)		rcvd_tbl[(bit)>>3]	/* identify byte in array */
#define	B_(bit)		(1 << ((bit) & 0x07))	/* identify bit in byte */
#define	SET(bit)	(A_(bit) |= B_(bit))
#define	CLR(bit)	(A_(bit) &= (B_(bit)))
#define	TST(bit)	(A_(bit) & B_(bit))

#define	F_FLOOD		0x0001
#define	F_INTERVAL	0x0002
#define	F_PINGFILLED	0x0008
#define	F_QUIET		0x0010
#define	F_RROUTE	0x0020
#define	F_SO_DEBUG	0x0040
#define	F_VERBOSE	0x0100

#define F_NODEADDR	0x0800
#define F_FQDN		0x1000
#define F_INTERFACE	0x2000
#define F_SRCADDR	0x4000
#ifdef IPV6_REACHCONF
#define F_REACHCONF	0x8000
#endif
#define F_HOSTNAME	0x10000
#define F_FQDNOLD	0x20000
#define F_NIGROUP	0x40000
#define F_SUPTYPES	0x80000
#define F_NOMINMTU	0x100000
#define F_NOUSERDATA	(F_NODEADDR | F_FQDN | F_FQDNOLD | F_SUPTYPES)
u_int options = 0;

#define IN6LEN		sizeof(struct in6_addr)
#define SA6LEN		sizeof(struct sockaddr_in6)

#define SIN6(s)	((struct sockaddr_in6 *)(s))

/* Android-specific hacks to get this to compile.*/
#define MAXDNAME           1025

/*
 * MAX_DUP_CHK is the number of bits in received table, i.e. the maximum
 * number of received sequence numbers we can keep track of.  Change 128
 * to 8192 for complete accuracy...
 */
#define	MAX_DUP_CHK	(8 * 8192)
static int mx_dup_ck = MAX_DUP_CHK;
static char rcvd_tbl[MAX_DUP_CHK / 8];

static struct sockaddr_in6 dst;	/* who to ping6 */
static struct sockaddr_in6 src;	/* src addr of this packet */
static socklen_t srclen;
static int datalen = DEFDATALEN;
static int s;				/* socket file descriptor */
static u_char outpack[MAXPACKETLEN];
static char BSPACE = '\b';		/* characters written for flood */
static char DOT = '.';
static char *ping_hostname;
static int ident;			/* process id to identify our packets */
static u_int8_t nonce[8];		/* nonce field for node information */
static int hoplimit = -1;		/* hoplimit */
static int pathmtu = 0;		/* path MTU for the destination.  0 = unspec. */

/* counters */
static long npackets = PING_COUNT_DEFAULT;			/* max packets to transmit */
static long nreceived;			/* # of packets we got back */
static long nrepeats;			/* number of duplicates */
static long ntransmitted;		/* sequence # for outbound packets = #sent */
static struct timeval interval = {1, 0}; /* interval between packets */

/* timing */
static int timing;			/* flag to do timing */
static double tmin = 999999999.0;	/* minimum round trip time */
static double tmax = 0.0;		/* maximum round trip time */
static double tsum = 0.0;		/* sum of all times, for doing average */
static double tsumsq = 0.0;		/* sum of all times squared, for std. dev. */

/* for node addresses */
static u_short naflags;

/* for ancillary data(advanced API) */
static struct msghdr smsghdr;
static struct iovec smsgiov;
static char *scmsg = 0;

static xTimerHandle pingTimer;
static int timerAlarm;

/* ICMP6 types */
#define ICMP6_NI_QUERY			139
#define ICMP6_NI_REPLY			140

/* ICMP6 codes for NI Query */
#define ICMP6_NI_SUBJ_IPV6		0	/* Query Subject is an ipv6 address */
#define ICMP6_NI_SUBJ_FQDN		1	/* Query Subject is a Domain name */
#define ICMP6_NI_SUBJ_IPV4		2	/* Query Subject is an ipv4 address */

#define IN6_IS_ADDR_MULTICAST(a) (((const uint8_t *) (a))[0] == 0xff)

//#define ICMP6_FILTER 1

#define ICMP6_FILTER_BLOCK          1
#define ICMP6_FILTER_PASS           2
#define ICMP6_FILTER_BLOCKOTHERS    3
#define ICMP6_FILTER_PASSONLY       4

struct icmp6_filter
{
	uint32_t icmp6_filt[8];
};

#define ICMP6_ECHO_REQUEST          128
#define ICMP6_ECHO_REPLY            129
#define MLD_LISTENER_QUERY          130
#define MLD_LISTENER_REPORT         131
#define MLD_LISTENER_REDUCTION      132

#define ICMP6_FILTER_WILLPASS(type, filterp) \
				((((filterp)->icmp6_filt[(type) >> 5]) & (1 << ((type) & 31))) == 0)

#define ICMP6_FILTER_WILLBLOCK(type, filterp) \
				((((filterp)->icmp6_filt[(type) >> 5]) & (1 << ((type) & 31))) != 0)

#define ICMP6_FILTER_SETPASS(type, filterp) \
				((((filterp)->icmp6_filt[(type) >> 5]) &= ~(1 << ((type) & 31))))

#define ICMP6_FILTER_SETBLOCK(type, filterp) \
				((((filterp)->icmp6_filt[(type) >> 5]) |=  (1 << ((type) & 31))))

#define ICMP6_FILTER_SETPASSALL(filterp) \
				memset (filterp, 0, sizeof (struct icmp6_filter));

#define ICMP6_FILTER_SETBLOCKALL(filterp) \
        memset (filterp, 0xFF, sizeof (struct icmp6_filter));

static int ping6_main(void *arg);
static void fill(char *, char *);
static int get_hoplim(struct msghdr *);
static int get_pathmtu(struct msghdr *);
static struct in6_pktinfo *get_rcvpktinfo(struct msghdr *);
static void retransmit(void);
static size_t pingerlen(void);
static int pinger(void);
static const char *pr_addr(struct sockaddr *, int);
static void pr_icmph(struct icmp6_hdr *, u_char *);
static void pr_iph(struct ip6_hdr *);
static int myechoreply(const struct icmp6_hdr *);
static char *dnsdecode(const u_char **, const u_char *, const u_char *,	char *, size_t);
static void pr_pack(u_char *, int, struct msghdr *);
static int pr_bitrange(u_int32_t, int, int);
static void pr_retip(struct ip6_hdr *, u_char *);
static void summary(void);
static void usage(void);

static void print_buffer(uint8_t *buffer, uint32_t size)
{
	int i = 0;
	ping_mutex_lock();
	LWIP_DEBUGF( PING6_DEBUG,("\n"));
	LWIP_DEBUGF( PING6_DEBUG,("   0        1        2        3        4        5        6        7        8        9       10       11       12       13       14       15    \n"));
	LWIP_DEBUGF( PING6_DEBUG,("-----------------------------------------------------------------------------------------------------------------------------------------------\n"));
	for (i = 0; i < size; i++) {
		LWIP_DEBUGF( PING6_DEBUG,("0x%02x (%c) ", buffer[i],
					  ((buffer[i] >= 32) && (buffer[i] <= 126))? buffer[i] : 0x20));
		if ((i % 16) == 15) {
			LWIP_DEBUGF( PING6_DEBUG,("\n"));
		}
	}
	LWIP_DEBUGF( PING6_DEBUG,("\n\n"));
	ping_mutex_unlock();
}

static void pingTimerCallback(xTimerHandle pxTimer)
{
	timerAlarm++;
}

void ping6_thread(void *arg)
{
	ping_parm_t *ping_info = (ping_parm_t *) arg;
	ping_list_add(ping_info);
	int interval = PING_DELAY_DEFAULT;

	if(ping_info->interval > 0)
		interval = ping_info->interval;

	/* Create timer to be used for transmission */
	pingTimer = xTimerCreate("timerPing",
				 pdMS_TO_TICKS(interval),
				 pdTRUE,
				 (void *) 1,
				 pingTimerCallback);
	if (pingTimer == NULL) {
		ping_mutex_lock();
		LWIP_DEBUGF( PING6_DEBUG,("[%s] Error xTimerCreate failed\n", __func__));
		ping_mutex_unlock();
	}

	ping6_main((void*)ping_info);

	xTimerStop(pingTimer, 0);
	xTimerDelete(pingTimer, 0);

	// Initialize global variable
	ntransmitted = 0;
	nreceived = 0;
	nrepeats = 0;
	ntransmitted = 0;
	tmax = 0.0;
	tsum = 0.0;
	tsumsq = 0.0;

	ping_list_remove(ping_info);
	ping_deinit(ping_info);
}

static int
ping6_main(void *arg)
{
	struct sockaddr_in6 from;
	int timeout;
	struct addrinfo hints;
	struct addrinfo *src_info;
	struct addrinfo *dst_info;
	struct pollfd fdmaskp[1];
	int fail_count = 0;
	int cc;
	u_int i, packlen;
	int ch, hold, preload, ret_ga;
	u_char *datap = NULL, *packet = NULL;
	char *e = NULL;
	int if_index = 0;
	char ifname[4];
	char *gateway = NULL;
	int ip6optlen = 0;
	struct cmsghdr *scmsgp = NULL;
	int usepktinfo = 0;
	double intval;
	size_t rthlen;
	char buf[NI_MAXHOST];
	char* target = &buf[0];

	ping_parm_t* ping_info = (ping_parm_t*)arg;
	ipaddr_ntoa_r(&ping_info->addr, buf, sizeof(buf));

	ping_mutex_lock();
	LWIP_PLATFORM_DIAG(("[%s] Ping target: %s\n", __func__, target));
	ping_mutex_unlock();

	/* just to be sure */
	memset(&smsghdr, 0, sizeof(smsghdr));
	memset(&smsgiov, 0, sizeof(smsgiov));

	preload = 0;
	datap = &outpack[ICMP6ECHOLEN + ICMP6ECHOTMLEN];

	/* set packet size */
	if(ping_info->packet_size > 0 && ping_info->packet_size <= MAXDATALEN)
		datalen = ping_info->packet_size;
	else
		datalen = PING_DATA_SIZE;

	if(ping_info->target_count > 0)
		npackets = ping_info->target_count;
	else
		npackets = PING_COUNT_DEFAULT;

	/* set the outgoing interface */
	sprintf(ifname, "wl%d", if_index);

	/* set the src address */
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_NUMERICHOST; /* allow hostname? */
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_protocol = IPPROTO_ICMPV6;

	ret_ga = getaddrinfo(ip6addr_ntoa(netif_ip6_addr(&nrc_netif[if_index], 0)), NULL, &hints, &src_info);
	if (ret_ga) {
		ping_mutex_lock();
		LWIP_DEBUGF( PING6_DEBUG,("[%s] Error invalid source address\n", __func__));
		ping_mutex_unlock();
		return 1;
	}

	/*
	 * res->ai_family must be AF_INET6 and res->ai_addrlen
	 * must be sizeof(src).
	 */
	memcpy(&src, src_info->ai_addr, src_info->ai_addrlen);
	srclen = src_info->ai_addrlen;
	freeaddrinfo(src_info);
	options |= F_SRCADDR;

	ping_mutex_lock();
	LWIP_DEBUGF( PING6_DEBUG,("[%s] after setting, src addr : %s\n",\
	__func__, pr_addr((struct sockaddr *)&src, sizeof(src))));
	ping_mutex_unlock();

	/* getaddrinfo */
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_CANONNAME;
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_protocol = IPPROTO_ICMPV6;

	ret_ga = getaddrinfo(target, NULL, &hints, &dst_info);
	if (ret_ga) {
		ping_mutex_lock();
		LWIP_DEBUGF( PING6_DEBUG,( "[%s] Error invalid destination address\n", __func__));
		ping_mutex_unlock();
	}

	if (dst_info->ai_canonname) {
		ping_hostname = dst_info->ai_canonname;
	} else {
		ping_hostname = target;
	}

	ping_mutex_lock();
	LWIP_DEBUGF( PING6_DEBUG,("[%s] ping_hostname %s\n", __func__, ping_hostname));
	ping_mutex_unlock();

	if (!dst_info->ai_addr) {
		ping_mutex_lock();
		LWIP_DEBUGF( PING6_DEBUG,( "[%s] Error getaddrinfo failed\n", __func__));
		ping_mutex_unlock();
		return 1;
	}
	(void)memcpy(&dst, dst_info->ai_addr, dst_info->ai_addrlen);
	ping_mutex_lock();
	LWIP_DEBUGF( PING6_DEBUG,("[%s] dst_info->ai_addr ==> %s, dst_info->ai_addrlen = %d\n",\
		__func__, pr_addr(dst_info->ai_addr, dst_info->ai_addrlen), dst_info->ai_addrlen));
	ping_mutex_unlock();

	/* Create socket to be used for ping */
	if ((s = socket(dst_info->ai_family, dst_info->ai_socktype, dst_info->ai_protocol)) < 0) {
		ping_mutex_lock();
		LWIP_DEBUGF( PING6_DEBUG,("[%s] Error socket = %d\n", __func__, s));
		ping_mutex_unlock();
		freeaddrinfo(dst_info);
		return 1;
	}

	freeaddrinfo(dst_info);

    /* bind the interface according to source address to be used. */
	if (options & F_SRCADDR) {
	    if (bind(s, (struct sockaddr *)&src, srclen) != 0) {
			ping_mutex_lock();
			LWIP_DEBUGF( PING6_DEBUG,("[%s] Error bind to source interface\n", __func__));
			ping_mutex_unlock();
			lwip_close(s);
			return 1;
		}
	}

	/* set the gateway (next hop) if specified */
	if (gateway) {
		struct addrinfo ghints, *gres;
		int error;

		memset(&ghints, 0, sizeof(ghints));
		ghints.ai_family = AF_INET6;
		ghints.ai_socktype = SOCK_RAW;
		ghints.ai_protocol = IPPROTO_ICMPV6;

		error = getaddrinfo(gateway, NULL, &hints, &gres);
		if (error) {
			ping_mutex_lock();
			LWIP_DEBUGF( PING6_DEBUG,("[%s] Error getaddrinfo for the gateway %s", __func__));
			ping_mutex_unlock();
		}

		if (gres->ai_next) {
			ping_mutex_lock();
			LWIP_DEBUGF( PING6_DEBUG,("[%s] Info gateway resolves to multiple addresses", __func__));
			ping_mutex_unlock();
		}
		freeaddrinfo(gres);
	}

	if ((options & F_NOUSERDATA) == 0) {
		if (datalen >= (int)sizeof(struct tv32)) {
			/* we can time transfer */
			timing = 1;
		} else
			timing = 0;
		/* in F_VERBOSE case, we may get non-echoreply packets*/
		if (options & F_VERBOSE)
			packlen = 2048 + IP6LEN + ICMP6ECHOLEN + EXTRA;
		else
			packlen = datalen + IP6LEN + ICMP6ECHOLEN + EXTRA;
	} else {
		/* suppress timing for node information query */
		timing = 0;
		datalen = 2048;
		packlen = 2048 + IP6LEN + ICMP6ECHOLEN + EXTRA;
	}

	if (!(packet = (u_char *)mem_malloc(packlen))) {
		ping_mutex_lock();
		LWIP_DEBUGF( PING6_DEBUG,("[%s] Error Unable to allocate packet\n", __func__));
		ping_mutex_unlock();
		lwip_close(s);
		return 1;
	}

	if (!(options & F_PINGFILLED)) {
		for (i = ICMP6ECHOLEN; i < packlen; ++i)
			*datap++ = i;
	}

	ident = random() & 0xFFFF;
	memset(nonce, 0, sizeof(nonce));
	for (i = 0; i < sizeof(nonce); i += sizeof(u_int32_t))
		*((u_int32_t *)&nonce[i]) = random();

	hold = 1;

	/* Specify the outgoing interface and/or the source address */
	if (hoplimit != -1)
		ip6optlen += CMSG_SPACE(sizeof(int));

	/* set IP6 packet options */
	if (ip6optlen) {
		if ((scmsg = (char *)mem_malloc(ip6optlen)) == 0) {
			ping_mutex_lock();
			LWIP_DEBUGF( PING6_DEBUG,("[%s] Error can't allocate enough memory", __func__));
			ping_mutex_unlock();
			mem_free(packet);
		}
		smsghdr.msg_control = (caddr_t)scmsg;
		smsghdr.msg_controllen = ip6optlen;
		scmsgp = (struct cmsghdr *)scmsg;
	}

	if ((dst.sin6_scope_id = if_nametoindex(ifname)) == 0) {
		ping_mutex_lock();
		LWIP_DEBUGF( PING6_DEBUG,( "[%s] Error %s: invalid interface name", __func__, ifname));
		ping_mutex_unlock();
		lwip_close(s);
		if (packet)
			mem_free(packet);
		if (scmsg)
			mem_free(scmsg);
		return 1;
	}

	ping_mutex_lock();
	LWIP_DEBUGF( PING6_DEBUG,("[%s] PING6(%lu=40+8+%lu bytes) ", __func__, (unsigned long)(40 + pingerlen()),
	    (unsigned long)(pingerlen() - 8)));
	LWIP_DEBUGF( PING6_DEBUG,("%s --> ", pr_addr((struct sockaddr *)&src, sizeof(src))));
	LWIP_DEBUGF( PING6_DEBUG,("%s\n", pr_addr((struct sockaddr *)&dst, sizeof(dst))));
	ping_mutex_unlock();

	while (preload--) {		/* Fire off them quickies. */
		(void)pinger();
	}

	if ((options & F_FLOOD) == 0) {
		if (xTimerStart(pingTimer, 0) != pdPASS) {
			ping_mutex_lock();
			LWIP_DEBUGF( PING6_DEBUG,("[%s] Error Timer start failed\n", __func__));
			ping_mutex_unlock();
		}
		if (ntransmitted == 0) {
			retransmit();
		}
	}

	timerAlarm = 0;
	fail_count = 0;

	ping_mutex_lock();
	LWIP_DEBUGF( PING6_DEBUG,("[%s] options : 0x%x\n", __func__, options));
	ping_mutex_unlock();

	for (;;) {
		struct msghdr m;
		struct cmsghdr *cm;
		u_char buf[1024];
		struct iovec iov[2];

		if (timerAlarm) {
			retransmit();
			timerAlarm = 0;
			continue;
		}

		if (options & F_FLOOD) {
			(void)pinger();
			timeout = 10;
		} else {
			timeout = 1000;
		}
		fdmaskp[0].fd = s;
		fdmaskp[0].events = POLLIN;
		cc = poll(fdmaskp, 1, timeout);
		if (cc < 0) {
			if (errno != EINTR) {
				ping_mutex_lock();
				LWIP_DEBUGF( PING6_DEBUG,("poll\n"));
				ping_mutex_unlock();
			}
			continue;
		} else if (cc == 0) {
			fail_count++;
			if (fail_count < MAX_FAIL_COUNT) {
				continue;
			} else {
				break;
			}
		} else {
			fail_count = 0;
		}

		m.msg_name = (caddr_t)&from;
		m.msg_namelen = sizeof(from);
		memset(&iov, 0, sizeof(iov));
		iov[0].iov_base = (caddr_t)packet;
		iov[0].iov_len = packlen;
		m.msg_iov = iov;
		m.msg_iovlen = 1;
		cm = (struct cmsghdr *)buf;
		m.msg_control = (caddr_t)buf;
		m.msg_controllen = sizeof(buf);

		cc = recvmsg(s, &m, 0);
		if (cc < 0) {
			if (errno != EINTR) {
				ping_mutex_lock();
				LWIP_DEBUGF( PING6_DEBUG,("[%s] Error recvmsg\n", __func__));
				ping_mutex_unlock();
				//sleep(1);
			}
			continue;
		} else if (cc == 0) {
			int mtu;

			/*
			 * receive control messages only. Process the
			 * exceptions (currently the only possiblity is
			 * a path MTU notification.)
			 */
			if ((mtu = get_pathmtu(&m)) > 0) {
				if ((options & F_VERBOSE) != 0) {
					ping_mutex_lock();
					LWIP_DEBUGF( PING6_DEBUG,("new path MTU (%d) is notified\n", mtu));
					ping_mutex_unlock();
				}
			}
			continue;
		} else {
			/*
			 * an ICMPv6 message (probably an echoreply) arrived.
			 */
			pr_pack(packet, cc, &m);
		}
		if ((npackets && nreceived >= npackets) || ping_info->force_stop) {
			ping_mutex_lock();
			LWIP_DEBUGF( PING6_DEBUG,("[%s] Debug break out of infinite for loop.\n", __func__));
			ping_mutex_unlock();
			break;
		}
	}
	summary();

	lwip_close(s);
	if (packet)
		mem_free(packet);
	if (scmsg)
		mem_free(scmsg);
	return 0;
}

/*
 * retransmit --
 *	This routine transmits another ping6.
 */
static void
retransmit(void)
{
	TickType_t timer;
	if (pinger() == 0)
		return;

	/*
	 * If we're not transmitting any more packets, change the timer
	 * to wait two round-trip times if we've received any packets or
	 * ten seconds if we haven't.
	 */
#define	MAXWAIT		10
	if (nreceived) {
		if (tmax != 0.0) {
			timer = 2 * tmax;
		} else {
			timer = 1000;
		}
	} else {
		timer = MAXWAIT * 1000;
	}

	if (xTimerChangePeriod(pingTimer, timer, 100) == pdFAIL) {
		ping_mutex_lock();
		LWIP_DEBUGF( PING6_DEBUG, ("[%s] xTimerChangePeriod failed\n", __func__));
		ping_mutex_unlock();
	}
}

/*
 * pinger --
 *	Compose and transmit an ICMP ECHO REQUEST packet.  The IP packet
 * will be added on by the kernel.  The ID field is our UNIX process ID,
 * and the sequence number is an ascending integer.  The first 8 bytes
 * of the data portion are used to hold a UNIX "timeval" struct in VAX
 * byte-order, to compute the round-trip time.
 */
static size_t
pingerlen(void)
{
	size_t l;

	if (options & F_FQDN)
		l = ICMP6_NIQLEN + sizeof(dst.sin6_addr);
	else if (options & F_FQDNOLD)
		l = ICMP6_NIQLEN;
	else if (options & F_NODEADDR)
		l = ICMP6_NIQLEN + sizeof(dst.sin6_addr);
	else if (options & F_SUPTYPES)
		l = ICMP6_NIQLEN;
	else
		l = ICMP6ECHOLEN + datalen;

	return l;
}

static int
pinger(void)
{
	struct icmp6_hdr *icp;
	struct iovec iov[2];
	int i, cc;
	struct icmp6_nodeinfo *nip;
	int seq;

	if (npackets && ntransmitted >= npackets)
		return(-1);	/* no more transmission */

	icp = (struct icmp6_hdr *)outpack;
	nip = (struct icmp6_nodeinfo *)outpack;
	memset(icp, 0, sizeof(*icp));
	icp->chksum = 0;
	seq = ntransmitted++;
	CLR(seq % mx_dup_ck);

	icp->type = ICMP6_TYPE_EREQ;
	icp->code = 0;
	icp->data = htons(ident);
	icp->data |= htons(ntohs(seq))<<16;
	if (timing) {
		struct timeval tv;
		struct tv32 *tv32;
		u32_t now = sys_now();
		tv.tv_sec = now / configTICK_RATE_HZ;
		tv.tv_usec = (now % configTICK_RATE_HZ)*1000;
		//(void)gettimeofday(&tv, NULL);
		tv32 = (struct tv32 *)&outpack[ICMP6ECHOLEN];
		tv32->tv32_sec = htonl(tv.tv_sec);
		tv32->tv32_usec = htonl(tv.tv_usec);
	}
	cc = ICMP6ECHOLEN + datalen;

#ifdef DIAGNOSTIC
	if (pingerlen() != cc) {
		ping_mutex_lock();
		LWIP_DEBUGF( PING6_DEBUG, ("Warning internal error; length mismatch"));
		ping_mutex_unlock();
	}
#endif

	smsghdr.msg_name = (caddr_t)&dst;
	smsghdr.msg_namelen = sizeof(dst);
	memset(&iov, 0, sizeof(iov));
	iov[0].iov_base = (caddr_t)outpack;
	iov[0].iov_len = cc;
	smsghdr.msg_iov = iov;
	smsghdr.msg_iovlen = 1;

	i = sendmsg(s, &smsghdr, 0);

	if (i < 0 || i != cc)  {
		if (i < 0) {
			ping_mutex_lock();
			LWIP_DEBUGF( PING6_DEBUG, ("[%s] Error sendmsg failed..\n", __func__));
		    LWIP_DEBUGF( PING6_DEBUG, ("ping6: wrote %s %d chars, ret=%d\n",
		    ping_hostname, cc, i));
			ping_mutex_unlock();
		}
	}

	return(0);
}

static int
myechoreply(const struct icmp6_hdr *icp)
{
	u_int16_t icmp6_id;
	icmp6_id = icp->data & 0x0000FFFF;
	if (icmp6_id == lwip_htons(ident))
		return 1;
	else
		return 0;
}

static char *
dnsdecode(const u_char **sp, const u_char *ep, const u_char *base, char *buf,
	  size_t bufsiz)
{
	int i;
	const u_char *cp;
	char cresult[MAXDNAME + 1];
	const u_char *comp;
	int l;

	i = 0;		/* XXXGCC -Wuninitialized [sun2] */

	cp = *sp;
	*buf = '\0';

	if (cp >= ep)
		return NULL;
	while (cp < ep) {
		i = *cp;
		if (i == 0 || cp != *sp) {
			if (strlcat((char *)buf, ".", bufsiz) >= bufsiz)
				return NULL;	/*result overrun*/
		}
		if (i == 0)
			break;
		cp++;

		if ((i & 0xc0) == 0xc0 && cp - base > (i & 0x3f)) {
			/* DNS compression */
			if (!base)
				return NULL;

			comp = base + (i & 0x3f);
			if (dnsdecode(&comp, cp, base, cresult,
			    sizeof(cresult)) == NULL)
				return NULL;
			if (strlcat(buf, cresult, bufsiz) >= bufsiz)
				return NULL;	/*result overrun*/
			break;
		} else if ((i & 0x3f) == i) {
			if (i > ep - cp)
				return NULL;	/*source overrun*/
			while (i-- > 0 && cp < ep) {
				l = snprintf(cresult, sizeof(cresult),
				    isprint(*cp) ? "%c" : "\\%03o", *cp & 0xff);
				if (l >= (int)sizeof(cresult) || l < 0)
					return NULL;
				if (strlcat(buf, cresult, bufsiz) >= bufsiz)
					return NULL;	/*result overrun*/
				cp++;
			}
		} else
			return NULL;	/*invalid label*/
	}
	if (i != 0)
		return NULL;	/*not terminated*/
	cp++;
	*sp = cp;
	return buf;
}

/*
 * pr_pack --
 *	Print out the packet, if it came from us.  This logic is necessary
 * because ALL readers of the ICMP socket get a copy of ALL ICMP packets
 * which arrive ('tis only fair).  This permits multiple copies of this
 * program to be run without having intermingled output (or statistics!).
 */
static void
pr_pack(u_char *buf, int cc, struct msghdr *mhdr)
{
	struct ip6_hdr *ip6hdr;
	struct icmp6_hdr *icp;
	struct icmp6_nodeinfo *ni;
	u_char *icmp_buf = buf + sizeof(struct ip6_hdr);
	int i;
	int hoplim;
	struct sockaddr *from;
	int fromlen;
	u_char *cp = NULL, *dp, *end = buf + cc;
	struct in6_pktinfo *pktinfo = NULL;
	struct timeval tv, tp;
	struct tv32 *tpp;
	double triptime = 0;
	int dupflag;
	size_t off;
	int oldfqdn;
	u_int16_t seq;
	char dnsname[MAXDNAME + 1];

	u32_t now = sys_now();
	tv.tv_sec = now / configTICK_RATE_HZ;
	tv.tv_usec = (now % configTICK_RATE_HZ)*1000;

	print_buffer(buf, cc);
	if (!mhdr || !mhdr->msg_name ||
	    mhdr->msg_namelen != sizeof(struct sockaddr_in6) ||
	    ((struct sockaddr *)mhdr->msg_name)->sa_family != AF_INET6) {
		ping_mutex_lock();
		LWIP_DEBUGF( PING6_DEBUG, ("[%s] Error invalid peername\n", __func__));
		ping_mutex_unlock();
		return;
	}
	from = (struct sockaddr *)mhdr->msg_name;
	fromlen = mhdr->msg_namelen;
	if (cc < (int)sizeof(struct icmp6_hdr)) {
		ping_mutex_lock();
		LWIP_DEBUGF( PING6_DEBUG, ("[%s] Error packet too short (%d bytes) from %s\n",\
		 __func__, cc, pr_addr(from, fromlen)));
		ping_mutex_unlock();
		return;
	}

	ip6hdr = (struct ip6_hdr *) buf;

	if (ip6hdr->_nexth != IP6_NEXTH_ICMP6) {
		ping_mutex_lock();
		LWIP_DEBUGF( PING6_DEBUG, ("[%s] received non icmp6 packet... returning...\n", __func__));
		ping_mutex_unlock();
		return;
	}

	icp = (struct icmp6_hdr *) icmp_buf;
	ni = (struct icmp6_nodeinfo *) icmp_buf;
	off = 0;

	hoplim = ip6hdr->_hoplim;

	pktinfo = get_rcvpktinfo(mhdr);

	if (icp->type == ICMP6_TYPE_EREP && myechoreply(icp)) {
		struct icmp6_echo_hdr *echo_hdr = (struct icmp6_echo_hdr *) icmp_buf;
		seq = echo_hdr->seqno;
		++nreceived;
		if (timing) {
			tpp = (struct tv32 *)(icp + 1);
			tp.tv_sec = ntohl(tpp->tv32_sec);
			tp.tv_usec = ntohl(tpp->tv32_usec);
			tv.tv_sec -= tp.tv_sec;
			tv.tv_usec -= tp.tv_usec;
			triptime = ((double)tv.tv_sec) * 1000.0 +
			    ((double)tv.tv_usec) / 1000.0;
			tsum += triptime;
			tsumsq += triptime * triptime;
			if (triptime < tmin)
				tmin = triptime;
			if (triptime > tmax)
				tmax = triptime;
		}

		if (TST(seq % mx_dup_ck)) {
			++nrepeats;
			--nreceived;
			dupflag = 1;
		} else {
			SET(seq % mx_dup_ck);
			dupflag = 0;
		}

		if (options & F_QUIET)
			return;

		if (!(options & F_FLOOD)) {
			ping_mutex_lock();
			LWIP_PLATFORM_DIAG(  ("%d bytes from %s, icmp_seq=%u", cc,
			    pr_addr(from, fromlen), seq));
			LWIP_PLATFORM_DIAG(  (" hlim=%d", hoplim));

			if ((options & F_VERBOSE) != 0) {
				struct sockaddr_in6 dstsa;

				memset(&dstsa, 0, sizeof(dstsa));
				dstsa.sin6_family = AF_INET6;
#ifdef SIN6_LEN
				dstsa.sin6_len = sizeof(dstsa);
#endif
				dstsa.sin6_scope_id = pktinfo->ipi6_ifindex;
				dstsa.sin6_addr = pktinfo->ipi6_addr;
				LWIP_PLATFORM_DIAG(  (" dst=%s",
				    pr_addr((struct sockaddr *)&dstsa,
				    sizeof(dstsa))));
			}

			if (timing)
				LWIP_PLATFORM_DIAG(  (" time=%.3f ms", triptime));

			if (dupflag)
				LWIP_PLATFORM_DIAG(  ("(DUP!)"));

			/* check the data */
			LWIP_PLATFORM_DIAG(  ("\n"));
			cp = icmp_buf + off + ICMP6ECHOLEN + ICMP6ECHOTMLEN;
			dp = outpack + ICMP6ECHOLEN + ICMP6ECHOTMLEN;
			for (i = 8; cp < end; ++i, ++cp, ++dp) {
				if (*cp != *dp) {
					LWIP_PLATFORM_DIAG(  ("\nwrong data byte #%d should be 0x%x but was 0x%x", i, *dp, *cp));
					break;
				}
			}
			ping_mutex_unlock();
		}
	} else {
		/* We've got something other than an ECHOREPLY */
		if (!(options & F_VERBOSE))
			return;
		ping_mutex_lock();
		LWIP_PLATFORM_DIAG(("%d bytes from %s: ", cc, pr_addr(from, fromlen)));
		ping_mutex_unlock();
		pr_icmph(icp, end);
	}
}

static int
pr_bitrange(u_int32_t v, int soff, int ii)
{
	int off;
	int i;

	off = 0;
	while (off < 32) {
		/* shift till we have 0x01 */
		if ((v & 0x01) == 0) {
			if (ii > 1){
				ping_mutex_lock();
				LWIP_DEBUGF( PING6_DEBUG, ("-%u", soff + off - 1));
				ping_mutex_unlock();
			}
			ii = 0;
			switch (v & 0x0f) {
			case 0x00:
				v >>= 4;
				off += 4;
				continue;
			case 0x08:
				v >>= 3;
				off += 3;
				continue;
			case 0x04: case 0x0c:
				v >>= 2;
				off += 2;
				continue;
			default:
				v >>= 1;
				off += 1;
				continue;
			}
		}

		/* we have 0x01 with us */
		for (i = 0; i < 32 - off; i++) {
			if ((v & (0x01 << i)) == 0)
				break;
		}
		if (!ii) {
			ping_mutex_lock();
			LWIP_DEBUGF( PING6_DEBUG, (" %u", soff + off));
			ping_mutex_unlock();
		}
		ii += i;
		v >>= i; off += i;
	}
	return ii;
}

static int
get_hoplim(struct msghdr *mhdr)
{
	struct cmsghdr *cm;

	for (cm = (struct cmsghdr *)CMSG_FIRSTHDR(mhdr); cm;
	     cm = (struct cmsghdr *)CMSG_NXTHDR(mhdr, cm)) {
		if (cm->cmsg_len == 0) {
			ping_mutex_lock();
			LWIP_DEBUGF( PING6_DEBUG, ("[%s] cm->cmsg_len = 0\n", __func__));
			ping_mutex_unlock();
			return(-1);
		}

		ping_mutex_lock();
		LWIP_DEBUGF( PING6_DEBUG, ("[%s] cm->cmsg_level = %d\n", __func__, cm->cmsg_level));
		LWIP_DEBUGF( PING6_DEBUG, ("[%s] cm->cmsg_type = %d\n", __func__, cm->cmsg_type));
		LWIP_DEBUGF( PING6_DEBUG, ("[%s] cm->cmsg_len = %d\n", __func__, cm->cmsg_len));
		ping_mutex_unlock();
		if (cm->cmsg_level == IPPROTO_IPV6 &&
		    cm->cmsg_type == IPV6_HOPLIMIT &&
		    cm->cmsg_len == CMSG_LEN(sizeof(int)))
			return(*(int *)CMSG_DATA(cm));
	}
	ping_mutex_lock();
	LWIP_DEBUGF( PING6_DEBUG, ("[%s] returning -1\n", __func__));
	ping_mutex_unlock();
	return(-1);
}

static struct in6_pktinfo *
get_rcvpktinfo(struct msghdr *mhdr)
{
	struct cmsghdr *cm;

	for (cm = (struct cmsghdr *)CMSG_FIRSTHDR(mhdr); cm;
	     cm = (struct cmsghdr *)CMSG_NXTHDR(mhdr, cm)) {
		if (cm->cmsg_len == 0)
			return(NULL);

		if (cm->cmsg_level == IPPROTO_IPV6 &&
		    cm->cmsg_type == IPV6_PKTINFO &&
		    cm->cmsg_len == CMSG_LEN(sizeof(struct in6_pktinfo)))
			return((struct in6_pktinfo *)CMSG_DATA(cm));
	}

	return(NULL);
}

static int
get_pathmtu(struct msghdr *mhdr)
{
#ifdef IPV6_RECVPATHMTU
	struct cmsghdr *cm;
	struct ip6_mtuinfo *mtuctl = NULL;

	for (cm = (struct cmsghdr *)CMSG_FIRSTHDR(mhdr); cm;
	     cm = (struct cmsghdr *)CMSG_NXTHDR(mhdr, cm)) {
		if (cm->cmsg_len == 0)
			return(0);

		if (cm->cmsg_level == IPPROTO_IPV6 &&
		    cm->cmsg_type == IPV6_PATHMTU &&
		    cm->cmsg_len == CMSG_LEN(sizeof(struct ip6_mtuinfo))) {
			mtuctl = (struct ip6_mtuinfo *)CMSG_DATA(cm);

			/*
			 * If the notified destination is different from
			 * the one we are pinging, just ignore the info.
			 * We check the scope ID only when both notified value
			 * and our own value have non-0 values, because we may
			 * have used the default scope zone ID for sending,
			 * in which case the scope ID value is 0.
			 */
			if (!IN6_ARE_ADDR_EQUAL(&mtuctl->ip6m_addr.sin6_addr,
						&dst.sin6_addr) ||
			    (mtuctl->ip6m_addr.sin6_scope_id &&
			     dst.sin6_scope_id &&
			     mtuctl->ip6m_addr.sin6_scope_id !=
			     dst.sin6_scope_id)) {
				ping_mutex_lock();
				LWIP_DEBUGF( PING6_DEBUG, ("path MTU for %s is notified. "
					       "(ignored)\n",
					   pr_addr((struct sockaddr *)&mtuctl->ip6m_addr,
					   sizeof(mtuctl->ip6m_addr))));
				ping_mutex_unlock();
				return(0);
			}

			/*
			 * Ignore an invalid MTU. XXX: can we just believe
			 * the kernel check?
			 */
			if (mtuctl->ip6m_mtu < IPV6_MMTU)
				return(0);

			/* notification for our destination. return the MTU. */
			return((int)mtuctl->ip6m_mtu);
		}
	}
#endif
	return(0);
}

/*
 * summary --
 *	Print out statistics.
 */
static void
summary(void)
{
	ping_mutex_lock();
	LWIP_PLATFORM_DIAG(("\n--- %s ping6 statistics ---\n", ping_hostname));
	LWIP_PLATFORM_DIAG(("%ld packets transmitted, ", ntransmitted));
	LWIP_PLATFORM_DIAG(("%ld packets received, ", nreceived));

	if (nrepeats)
		LWIP_PLATFORM_DIAG(("+%ld duplicates, ", nrepeats));
	if (ntransmitted) {
		if (nreceived > ntransmitted) {
			LWIP_PLATFORM_DIAG(("-- somebody's duplicating packets!"));
		} else {
			LWIP_PLATFORM_DIAG(("%.1f%% packet loss\n",
			    ((((double)ntransmitted - nreceived) * 100.0) /
			    ntransmitted)));
		}
	}
	ping_mutex_unlock();

	if (nreceived && timing) {
		/* Only display average to microseconds */
		double num = nreceived + nrepeats;
		double dev, avg;
		if (num > 1) {
			avg = tsum / num;
			dev = sqrt((tsumsq - num * avg * avg) / (num - 1));
		} else {
			avg = tsum;
			dev = 0.0;
		}

		ping_mutex_lock();
		LWIP_PLATFORM_DIAG((
		    "round-trip min/avg/max/std-dev = %.3f/%.3f/%.3f/%.3f ms\n",
		    tmin, avg, tmax, dev));
		ping_mutex_unlock();
	}
}

/*subject type*/
static const char *niqcode[] = {
	"IPv6 address",
	"DNS label",	/*or empty*/
	"IPv4 address",
};

/*result code*/
static const char *nircode[] = {
	"Success", "Refused", "Unknown",
};


/*
 * pr_icmph --
 *	Print a descriptive string about an ICMP header.
 */
static void
pr_icmph(struct icmp6_hdr *icp, u_char *end)
{
	char ntop_buf[INET6_ADDRSTRLEN];
	struct nd_redirect *red;
	struct icmp6_nodeinfo *ni;
	char dnsname[MAXDNAME + 1];
	const u_char *cp;
	size_t l;

	ping_mutex_lock();
	switch (icp->type) {
	case ICMP6_TYPE_DUR:
		switch (icp->code) {
		case ICMP6_DUR_NO_ROUTE:
			LWIP_DEBUGF( PING6_DEBUG, ("No Route to Destination\n"));
			break;
		case ICMP6_DUR_PROHIBITED:
			LWIP_DEBUGF( PING6_DEBUG, ("Destination Administratively Unreachable\n"));
			break;
		case ICMP6_DUR_SCOPE:
			LWIP_DEBUGF( PING6_DEBUG, ("Destination Unreachable Beyond Scope\n"));
			break;
		case ICMP6_DUR_ADDRESS:
			LWIP_DEBUGF( PING6_DEBUG, ("Destination Host Unreachable\n"));
			break;
		case ICMP6_DUR_PORT:
			LWIP_DEBUGF( PING6_DEBUG, ("Destination Port Unreachable\n"));
			break;
		case ICMP6_DUR_POLICY:
			LWIP_DEBUGF( PING6_DEBUG, ("Source address failed ingress/egress policy\n"));
			break;
		case ICMP6_DUR_REJECT_ROUTE:
			LWIP_DEBUGF( PING6_DEBUG, ("Reject route to destination\n"));
			break;
		default:
			LWIP_DEBUGF( PING6_DEBUG, ("Destination Unreachable, Bad Code: %d\n", icp->code));
			break;
		}
		/* Print returned IP header information */
		pr_retip((struct ip6_hdr *)(icp + 1), end);
		break;
	case ICMP6_TYPE_PTB:
		LWIP_DEBUGF( PING6_DEBUG, ("Packet too big mtu\n"));
		pr_retip((struct ip6_hdr *)(icp + 1), end);
		break;
	case ICMP6_TYPE_TE:
		switch (icp->code) {
		case ICMP6_TE_HL:
			LWIP_DEBUGF( PING6_DEBUG, ("Time to live exceeded\n"));
			break;
		case ICMP6_TE_FRAG:
			LWIP_DEBUGF( PING6_DEBUG, ("Frag reassembly time exceeded\n"));
			break;
		default:
			LWIP_DEBUGF( PING6_DEBUG, ("Time exceeded, Bad Code: %d\n", icp->code));
			break;
		}
		pr_retip((struct ip6_hdr *)(icp + 1), end);
		break;
	case ICMP6_TYPE_PP:
		LWIP_DEBUGF( PING6_DEBUG, ("Parameter problem: "));
		switch (icp->code) {
		case ICMP6_PP_FIELD:
			LWIP_DEBUGF( PING6_DEBUG, ("Erroneous Header\n"));
			break;
		case ICMP6_PP_HEADER:
			LWIP_DEBUGF( PING6_DEBUG, ("Unknown Nextheader\n"));
			break;
		case ICMP6_PP_OPTION:
			LWIP_DEBUGF( PING6_DEBUG, ("Unrecognized Option\n"));
			break;
		default:
			LWIP_DEBUGF( PING6_DEBUG, ("Bad code(%d)\n", icp->code));
			break;
		}
		pr_retip((struct ip6_hdr *)(icp + 1), end);
		break;
	case ICMP6_TYPE_EREQ:
		LWIP_DEBUGF( PING6_DEBUG, ("Echo Request\n"));
		/* XXX ID + Seq + Data */
		break;
	case ICMP6_TYPE_EREP:
		LWIP_DEBUGF( PING6_DEBUG, ("Echo Reply\n"));
		/* XXX ID + Seq + Data */
		break;
	case ICMP6_TYPE_MLQ:
		LWIP_DEBUGF( PING6_DEBUG, ("Listener Query\n"));
		break;
	case ICMP6_TYPE_MLR:
		LWIP_DEBUGF( PING6_DEBUG, ("Listener Report\n"));
		break;
	case ICMP6_TYPE_MLD:
		LWIP_DEBUGF( PING6_DEBUG, ("Listener Done\n"));
		break;
	case ICMP6_TYPE_RS:
		LWIP_DEBUGF( PING6_DEBUG, ("Router Solicitation\n"));
		break;
	case ICMP6_TYPE_RA:
		LWIP_DEBUGF( PING6_DEBUG, ("Router Advertisement\n"));
		break;
	case ICMP6_TYPE_NS:
		LWIP_DEBUGF( PING6_DEBUG, ("Neighbor Solicitation\n"));
		break;
	case ICMP6_TYPE_NA:
		LWIP_DEBUGF( PING6_DEBUG, ("Neighbor Advertisement\n"));
		break;
	case ICMP6_TYPE_RD:
		LWIP_DEBUGF( PING6_DEBUG, ("Redirect\n"));
		break;

	default:
		LWIP_DEBUGF( PING6_DEBUG, ("Bad ICMP type: %d", icp->type));
	}
	ping_mutex_unlock();
}

/*
 * pr_iph --
 *	Print an IP6 header.
 */
static void
pr_iph(struct ip6_hdr *ip6)
{
	char ntop_buf[INET6_ADDRSTRLEN];

	ping_mutex_lock();
	LWIP_DEBUGF( PING6_DEBUG,("Vr TC  Flow Plen Nxt Hlim\n"));
	LWIP_DEBUGF( PING6_DEBUG,(" %1x %02x %05x %04x  %02x   %02x\n",
				  IP6H_V(ip6), IP6H_TC(ip6), IP6H_FL(ip6),
				  ntohs(ip6->_plen), ip6->_nexth, ip6->_hoplim));
	if (!inet_ntop(AF_INET6, &ip6->src, ntop_buf, sizeof(ntop_buf)))
		strlcpy(ntop_buf, "?", sizeof(ntop_buf));
	LWIP_DEBUGF( PING6_DEBUG,("%s->", ntop_buf));
	if (!inet_ntop(AF_INET6, &ip6->dest, ntop_buf, sizeof(ntop_buf)))
		strlcpy(ntop_buf, "?", sizeof(ntop_buf));
	LWIP_DEBUGF( PING6_DEBUG,("%s\n", ntop_buf));
	ping_mutex_unlock();
}

static int
getnameinfo(const struct sockaddr *sa, socklen_t salen,
		    char *host, size_t hostlen,
			char *serv, size_t servlen, int flags)
{
	struct sockaddr_in *sin = NULL;
	struct sockaddr_in6 *sin6 = NULL;

	 if (flags & ~(AI_NUMERICHOST | AI_NUMERICSERV)) {
		return EINVAL;
	}

	switch (sa->sa_family) {
#if LWIP_IPV4
	case AF_INET:
		sin = (struct sockaddr_in *) sa;
		if (flags & AI_NUMERICHOST) {
			if (inet_ntop (AF_INET, &sin->sin_addr, host, hostlen) == NULL) {
				return EOVERFLOW;
			}
		}

		if (flags & AI_NUMERICSERV) {
			if (snprintf(serv, servlen, "%d", ntohs (sin->sin_port)) < 0) {
				return EOVERFLOW;
			}
		}

		break;
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
	case AF_INET6:
		sin6 = (struct sockaddr_in6 *) sa;
		if (flags & AI_NUMERICHOST) {
			if (inet_ntop (AF_INET6, &sin6->sin6_addr, host, hostlen) == NULL) {
				return EOVERFLOW;
			}
		}

		if (flags & AI_NUMERICSERV) {
			if (snprintf(serv, servlen, "%d", ntohs (sin6->sin6_port)) < 0) {
				return EOVERFLOW;
			}
		}
		break;
	}
#endif /* LWIP_IPV6 */
	return 0;
}

/*
 * pr_addr --
 *	Return an ascii host address as a dotted quad and optionally with
 * a hostname.
 */
static const char *
pr_addr(struct sockaddr *addr, int addrlen)
{
	static char buf[NI_MAXHOST];
	int flag = 0;

	if ((options & F_HOSTNAME) == 0)
		flag |= AI_NUMERICHOST;

	if (getnameinfo(addr, addrlen, buf, sizeof(buf), NULL, 0, flag) == 0)
		return (buf);
	else
		return "?";
}

/*
 * pr_retip --
 *	Dump some info on a returned (via ICMPv6) IPv6 packet.
 */
static void
pr_retip(struct ip6_hdr *ip6, u_char *end)
{
	u_char *cp = (u_char *)ip6, nh;
	int hlen;

	if (end - (u_char *)ip6 < (intptr_t)sizeof(*ip6)) {
		ping_mutex_lock();
		LWIP_DEBUGF( PING6_DEBUG,("IP6"));
		ping_mutex_unlock();
		goto trunc;
	}
	pr_iph(ip6);
	hlen = sizeof(*ip6);

	nh = ip6->_nexth;
	cp += hlen;
	while (end - cp >= 8) {
		switch (nh) {
		case IPPROTO_ICMPV6:
			ping_mutex_lock();
			LWIP_DEBUGF( PING6_DEBUG,("ICMP6: type = %d, code = %d\n",
			    *cp, *(cp + 1)));
			ping_mutex_unlock();
			return;
		case IPPROTO_TCP:
			ping_mutex_lock();
			LWIP_DEBUGF( PING6_DEBUG,("TCP: from port %u, to port %u (decimal)\n",
			    (*cp * 256 + *(cp + 1)),
			    (*(cp + 2) * 256 + *(cp + 3))));
			ping_mutex_unlock();
			return;
		case IPPROTO_UDP:
			ping_mutex_lock();
			LWIP_DEBUGF( PING6_DEBUG,("UDP: from port %u, to port %u (decimal)\n",
			    (*cp * 256 + *(cp + 1)),
			    (*(cp + 2) * 256 + *(cp + 3))));
			ping_mutex_unlock();
			return;
		default:
			ping_mutex_lock();
			LWIP_DEBUGF( PING6_DEBUG,("Unknown Header(%d)\n", nh));
			ping_mutex_unlock();
			return;
		}

		if ((cp += hlen) >= end)
			goto trunc;
	}
	if (end - cp < 8)
		goto trunc;

	ping_mutex_lock();
	LWIP_DEBUGF( PING6_DEBUG,("\n"));
	ping_mutex_unlock();
	return;

  trunc:
	ping_mutex_lock();
	LWIP_DEBUGF( PING6_DEBUG,("...\n"));
	ping_mutex_unlock();
	return;
}

static void
fill(char *bp, char *patp)
{
	int ii, jj, kk;
	int pat[16];
	char *cp;

	for (cp = patp; *cp; cp++)
		if (!isxdigit((unsigned char)*cp)){
			ping_mutex_lock();
			LWIP_DEBUGF( PING6_DEBUG,( "patterns must be specified as hex digits"));
			ping_mutex_unlock();
		}
	ii = sscanf(patp,
	    "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
	    &pat[0], &pat[1], &pat[2], &pat[3], &pat[4], &pat[5], &pat[6],
	    &pat[7], &pat[8], &pat[9], &pat[10], &pat[11], &pat[12],
	    &pat[13], &pat[14], &pat[15]);

/* xxx */
	if (ii > 0)
		for (kk = 0;
		    kk <= (int)(MAXDATALEN - (8 + sizeof(struct tv32) + ii));
		    kk += ii)
			for (jj = 0; jj < ii; ++jj)
				bp[jj + kk] = pat[jj];
	if (!(options & F_QUIET)) {
		ping_mutex_lock();
		LWIP_DEBUGF( PING6_DEBUG,("PATTERN: 0x"));
		for (jj = 0; jj < ii; ++jj)
			LWIP_DEBUGF( PING6_DEBUG,("%02x", bp[jj] & 0xFF));
		LWIP_DEBUGF( PING6_DEBUG,("\n"));
		ping_mutex_unlock();
	}
}
#endif /* LWIP_PING */
