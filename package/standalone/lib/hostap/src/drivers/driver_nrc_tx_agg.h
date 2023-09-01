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

#ifndef _DRIVER_NRC_TX_AGG_H_
#define _DRIVER_NRC_TX_AGG_H_

#include "system_type.h"
#include "driver_nrc.h"

/* For ADDBA state machine */
enum {
	AMPDU_SES_NO		= 0,	//No session yet.
	AMPDU_SES_WORK		= 1,	//Session is working
	AMPDU_SES_TRIGGERED = 2,	//Session triggered already
};



void nrc_tx_check_aggr(uint8_t vif_id, struct nrc_wpa_sta * sta, uint8_t tid, uint8_t ac);
int nrc_check_ampdu_mlme_tx_resources(uint8_t vif_id, struct nrc_wpa_sta *sta);
void nrc_stop_tx_ba_session(uint8_t vif_id, struct nrc_wpa_sta *sta, uint8_t tid);

#endif //_DRIVER_NRC_TX_AGG_H_

