/*
 * coap.h -- main header file for CoAP stack of libcoap
 *
 * Copyright (C) 2010-2012,2015-2016 Olaf Bergmann <bergmann@tzi.org>
 *               2015 Carsten Schoenert <c.schoenert@t-online.de>
 *
 * This file is part of the CoAP library libcoap. Please see README for terms
 * of use.
 */

#ifndef COAP_H_
#define COAP_H_

#define PACKAGE_NAME "libcoap"
#define PACKAGE_VERSION "4.2.0"
#define PACKAGE_STRING PACKAGE_NAME PACKAGE_VERSION


#ifdef __cplusplus
extern "C" {
#endif

#include "libcoap.h"

#include "address.h"
#include "async.h"
#include "bits.h"
#include "block.h"
#include "coap_dtls.h"
#include "coap_event.h"
#include "coap_io.h"
#include "coap_time.h"
#include "coap_debug.h"
#include "encode.h"
#include "coap_mem.h"
#include "net.h"
#include "option.h"
#include "pdu.h"
#include "prng.h"
#include "resource.h"
#include "str.h"
#include "subscribe.h"
#include "uri.h"
#include "uthash.h"
#include "utlist.h"

#ifdef __cplusplus
}
#endif

#endif /* COAP_H_ */
