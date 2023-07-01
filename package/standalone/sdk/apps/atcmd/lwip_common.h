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

#ifndef __LWIP_COMMON_H__
#define __LWIP_COMMON_H__
/**********************************************************************************************/

#include "lwip/sockets.h"
#include "lwip/ip.h"
#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"
#include "lwip/sys.h"


#define IP_HDR_LEN(hdr)				(IPH_HL(hdr) << 2)
#define ICMP_ECHO_HLEN				sizeof(struct icmp_echo_hdr)

#ifdef CONFIG_ATCMD_IPV6
#define ICMP6_ECHO_HLEN				sizeof(struct icmp6_echo_hdr)

#ifdef ATCMD_NETIF_INDEX
#define SIN6_SCOPE_ID				(ATCMD_NETIF_INDEX + 1)
#else
#define SIN6_SCOPE_ID				1
#endif
#endif /* #ifdef CONFIG_ATCMD_IPV6 */


extern struct netif *nrc_netif[];

/**********************************************************************************************/
#endif /* #ifndef __LWIP_COMMON_H__ */

