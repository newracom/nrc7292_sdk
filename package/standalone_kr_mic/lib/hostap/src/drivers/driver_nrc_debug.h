/*
 * Copyright (c) 2016-2021 Newracom, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _DRIVER_NRC_DEBUG_H_
#define _DRIVER_NRC_DEBUG_H_

#include "drivers/driver_nrc.h"

void wpa_driver_debug_frame(uint8_t* frame, uint16_t len);
const char* wpa_driver_alg_str(enum wpa_alg alg);
void wpa_driver_debug_assoc_params(struct wpa_driver_associate_params *params);
void wpa_driver_debug_key(struct nrc_wpa_key *key);
void wpa_driver_debug_key_all(struct nrc_wpa_if *intf);

#endif // _DRIVER_NRC_DEBUG_H_
