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

#ifndef _DRIVER_NRC_RX_AGG_H_
#define _DRIVER_NRC_RX_AGG_H_

#include "system_type.h"
#include "driver_nrc.h"


#define IEEE80211_SCTL_FRAG		0x000F
#define IEEE80211_SCTL_SEQ		0xFFF0

#define IEEE80211_SN_MASK		((IEEE80211_SCTL_SEQ) >> 4)
#define IEEE80211_MAX_SN		IEEE80211_SN_MASK
#define IEEE80211_SN_MODULO		(IEEE80211_MAX_SN + 1)


	/* ACK policy */
#define IEEE80211_QOS_CTL_ACK_POLICY_NORMAL	0x0000
#define IEEE80211_QOS_CTL_ACK_POLICY_NOACK	0x0020
#define IEEE80211_QOS_CTL_ACK_POLICY_NO_EXPL	0x0040
#define IEEE80211_QOS_CTL_ACK_POLICY_BLOCKACK	0x0060
#define IEEE80211_QOS_CTL_ACK_POLICY_MASK	0x0060
#define IEEE80211_SCTL_FRAG		0x000F


/**
 * ieee80211_has_a4 - check if IEEE80211_FCTL_TODS and IEEE80211_FCTL_FROMDS are set
 * @fc: frame control bytes in little-endian byteorder
 */
#if 0
static inline bool ieee80211_has_a4(__le16 fc)
{
	__le16 tmp = cpu_to_le16(IEEE80211_FCTL_TODS | IEEE80211_FCTL_FROMDS);
	return (fc & tmp) == tmp;
}
#else
static inline bool ieee80211_has_a4(GenericMacHeader * gmh)
{
	return (gmh->to_ds && gmh->from_ds);
}
#endif

/**
 * ieee80211_get_SA - get pointer to SA
 * @hdr: the frame
 *
 * Given an 802.11 frame, this function returns the offset
 * to the source address (SA). It does not verify that the
 * header is long enough to contain the address, and the
 * header must be long enough to contain the frame control
 * field.
 */
#if 0 
static inline u8 *ieee80211_get_SA(struct ieee80211_hdr *hdr)
{
	if (ieee80211_has_a4(hdr->frame_control))
		return hdr->addr4;
	if (ieee80211_has_fromds(hdr->frame_control))
		return hdr->addr3;
	return hdr->addr2;
}
#else // jhkim To do : Check any side effect.
static inline u8 *ieee80211_get_SA(GenericMacHeader * gmh)
{
	if (ieee80211_has_a4(gmh)){
		//struct ieee80211_hdr *hdr = (struct ieee80211_hdr *hdr) gmh;
		//return hdr->addr4;
		return 0;  // currently not suppport addr4 so return 0;
	}
	if (ieee80211_has_fromds(gmh))
		return gmh->address3;
	return gmh->address2;
}
#endif


/**
 * ieee80211_get_qos_ctl - get pointer to qos control bytes
 * @hdr: the frame
 *
 * The qos ctrl bytes come after the frame_control, duration, seq_num
 * and 3 or 4 addresses of length ETH_ALEN.
 * 3 addr: 2 + 2 + 2 + 3*6 = 24
 * 4 addr: 2 + 2 + 2 + 4*6 = 30
 */
#if 0
static inline u8 *ieee80211_get_qos_ctl(struct ieee80211_hdr *hdr)
{
	if (ieee80211_has_a4(hdr->frame_control))
		return (u8 *)hdr + 30;
	else
		return (u8 *)hdr + 24;
}
#else
static inline u8 *ieee80211_get_qos_ctl(GenericMacHeader * gmh)
{
	if (ieee80211_has_a4(gmh))
		return (u8 *)gmh + 30;
	else
		return (u8 *)gmh + 24;
}

#endif


#define jiffies xTaskGetTickCount()

static inline bool ieee80211_sn_less(u16 sn1, u16 sn2)
{
	return ((sn1 - sn2) & IEEE80211_SN_MASK) > (IEEE80211_SN_MODULO >> 1);
}

static inline u16 ieee80211_sn_add(u16 sn1, u16 sn2)
{
	return (sn1 + sn2) & IEEE80211_SN_MASK;
}

static inline u16 ieee80211_sn_inc(u16 sn)
{
	return ieee80211_sn_add(sn, 1);
}

static inline u16 ieee80211_sn_sub(u16 sn1, u16 sn2)
{
	return (sn1 - sn2) & IEEE80211_SN_MASK;
}

#define alloc_nrxb(a)	os_malloc(a)
#define free_nrxb(a)    os_free(a)

#define time_after(a,b)	 ((long)((b) - (a)) < 0)

struct nrxb_head {
	/* These two members must be first. */
	struct nrc_wpa_rx_data	*next;
	struct nrc_wpa_rx_data	*prev;
	uint32_t	qlen;
};

static inline struct nrc_wpa_rx_data * nrxb_peek_tail(const struct nrxb_head *list_)
{
	struct nrc_wpa_rx_data *nrxb = list_->prev;
	if (nrxb == (struct nrc_wpa_rx_data *)list_)
		nrxb = NULL;
	return nrxb;

}

static inline void nrxb_queue_head_init(struct nrxb_head *list)
{
	list->prev = list->next = (struct nrc_wpa_rx_data *)list;
	list->qlen = 0;
}

static inline void __nrxb_insert(struct nrc_wpa_rx_data *new_nrxb,
				struct nrc_wpa_rx_data *prev, struct nrc_wpa_rx_data *next,
				struct nrxb_head *list)
{
	new_nrxb->next = next;
	new_nrxb->prev = prev;
	next->prev = new_nrxb;
	prev->next = new_nrxb;
	list->qlen++;
}

static inline void __nrxb_queue_after(struct nrxb_head *list,
				     struct nrc_wpa_rx_data *prev,
				     struct nrc_wpa_rx_data *new_nrxb)
{
	__nrxb_insert(new_nrxb, prev, prev->next, list);
}


static inline void __nrxb_queue_before(struct nrxb_head *list,
				      struct nrc_wpa_rx_data *next,
				      struct nrc_wpa_rx_data *new_nrxb)
{
	__nrxb_insert(new_nrxb, next->prev, next, list);
}

static inline void nrxb_queue_tail(struct nrxb_head *list,
				   struct nrc_wpa_rx_data *new_nrxb)
{
	__nrxb_queue_before(list, (struct nrc_wpa_rx_data *)list, new_nrxb);
}

static inline void __nrxb_unlink(struct nrc_wpa_rx_data *nrxb, struct nrxb_head *list)
{
	struct nrc_wpa_rx_data *next, *prev;
	list->qlen--;
	next	   = nrxb->next;
	prev	   = nrxb->prev;
	nrxb->next  = nrxb->prev = NULL;
	next->prev = prev;
	prev->next = next;
}

static inline int nrxb_queue_empty(const struct nrxb_head *list)
{
	return list->next == (const struct nrc_wpa_rx_data *) list;
}

static inline struct nrc_wpa_rx_data *nrxb_peek(const struct nrxb_head *list_)
{
	struct nrc_wpa_rx_data *nrxb = list_->next;

	if (nrxb == (struct nrc_wpa_rx_data *)list_)
		nrxb = NULL;
	return nrxb;
}

static inline struct nrc_wpa_rx_data *__nrxb_dequeue(struct nrxb_head *list)
{
	struct nrc_wpa_rx_data *nrxb = nrxb_peek(list);
	if (nrxb)
		__nrxb_unlink(nrxb, list);
	return nrxb;
}

static inline void __nrxb_queue_purge(struct nrxb_head *list)
{
	struct nrc_wpa_rx_data *nrxb;
	while ((nrxb = __nrxb_dequeue(list)) != NULL){
		os_free(nrxb->u.frame);
		nrxb->u.frame=NULL;
		free_nrxb(nrxb);
		nrxb=NULL;
	}
}

void discard(SYS_BUF* buffer);

static inline void __nrxb_queue_purge_sysrxb(struct nrxb_head *list)
{
	struct nrc_wpa_rx_data *nrxb;
	while ((nrxb = __nrxb_dequeue(list)) != NULL){
		free_nrxb(nrxb);
		discard(nrxb->buf);		
	}
}

u16 nrc_start_rx_ba_session(struct nrc_wpa_sta *sta,
				      u8 dialog_token, u16 timeout,
				      u16 start_seq_num, u16 ba_policy, u16 tid,
				      u16 buf_size, bool tx, bool auto_seq);
void nrc_stop_rx_ba_session(struct nrc_wpa_sta *sta, u16 tid);
void nrc_rx_reorder_ampdu(struct nrc_wpa_rx_data *rx,
                		       struct nrxb_head *frames);
int nrc_check_ampdu_mlme_resources(struct nrc_wpa_sta *sta);
void nrc_rx_dump(unsigned char * addr, int len );


#endif // _DRIVER_NRC_RX_AGG_H_
