#ifndef COAP_CONFIG_H_
#define COAP_CONFIG_H_

#include <lwip/opt.h>
#include <lwip/debug.h>
#include <lwip/def.h> /* provide ntohs, htons */

#define WITH_POSIX 1
#define POSIX 1

#define HAVE_LIMITS_H

#define assert(x) LWIP_ASSERT("CoAP assert failed", x)

/* Define if the system has mbedtls */
#ifdef SUPPORT_MBEDTLS
#define HAVE_MBEDTLS 1
#endif

/* it's just provided by libc. i hope we don't get too many of those, as
 * actually we'd need autotools again to find out what environment we're
 * building in */
#define HAVE_STRNLEN 1
//#define COAP_RESOURCES_NOHASH
//#define HAVE_TIME_H
#endif /* COAP_CONFIG_H_ */
