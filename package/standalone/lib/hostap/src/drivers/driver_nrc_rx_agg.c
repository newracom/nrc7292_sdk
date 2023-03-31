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

#include "system_common.h"
#include "system_memory_manager.h"
#include "system_modem_api.h"
#include "umac_s1g_agent.h"
#include "lmac_common.h"
#include "utils/includes.h"
#include "utils/common.h"
#include "../common/wpa_common.h"
#include "../common/ieee802_11_common.h"
#include "../common/eapol_common.h"
#include "utils/eloop.h"
#include "utils/wpa_debug.h"
#include "driver_nrc.h"
#include "driver_nrc_scan.h"
#include "driver_nrc_debug.h"
#include "nrc-wim-types.h"
#include "nrc-vendor.h"
#include "nrc-wim-types.h"
#include "nrc_types.h"
#include "umac_scan.h"
#include "umac_wim_builder.h"
#include "umac_beacon_monitor.h"
#include "nrc_lwip.h"
#include "mbedtls/cipher.h"
#include "mbedtls/cmac.h"
#include "driver_nrc_rx_agg.h"

#ifdef TAG
#undef TAG
#endif
#define TAG "wpa_rx_agg: "

#ifndef BIT_MASK
#define BIT_MASK(nr)    (0x01 << (nr%32))
#endif
#ifndef set_bit
#define set_bit(nr, addr)   *addr |= BIT_MASK(nr)
#endif
#ifndef clear_bit
#define clear_bit(nr, addr) *addr &= ~BIT_MASK(nr)
#endif
#ifndef test_bit
#define test_bit(nr, addr)  (*addr & BIT_MASK(nr))?1:0
#endif

#define REORDER_USE_SEMAPHORE
#define USE_STATIC_TID_INFO

#ifdef USE_STATIC_TID_INFO
struct tid_ampdu_rx g_tid_rx[MAX_TID];
#endif


/* Modify timer */
void mod_timer( TimerHandle_t xTimer, unsigned long expires)
{
	if (xTimer == NULL ){
    	E(TT_WPAS, TAG "tmr: tmr handle is null \r\n");
		return;
	}

	if (expires <= 0){
    	E(TT_WPAS, TAG "tmr: tout val is null \r\n");
		expires = 1;
	}
   
    /* logically xTimerChangePeriod includes xTimerStar() function */
	if( xTimerChangePeriod( xTimer, expires, 1 ) != pdPASS )
	{
    	/* The command could not be sent, even after waiting for 100 ticks
    	   to pass.  Take appropriate action here. */
    	I(TT_WPAS, TAG "tmr: tout val has not changed ....\r\n");
        if( xTimerIsTimerActive( xTimer ) != pdFALSE )
        {
            //xTimerStop(xTimer, 0);
        }
        else
        {
	    	I(TT_WPAS, TAG "tmr is not active, so restart \r\n");			
            xTimerStart( xTimer, 0 ); 
        }
	}
}

int del_timer(TimerHandle_t xTimer)
{
	if (xTimer == NULL )
	{
		return (pdFAIL);
	}
	/* timer ID check */
    /*	if (timer_ptr->tx_timer_id != TX_TIMER_ID)
	{
		return (pdFAIL);
	}
	*/
	return 	xTimerDelete(xTimer, 0);
}

int del_timer_sync(TimerHandle_t xTimer)
{
	return del_timer(xTimer);
}

void deactivate_timer(TimerHandle_t xTimer)
{
	if (xTimer == NULL)
	{
		return;
	}
	xTimerStop(xTimer, 0);
}

// To do, add priority argument.
void nrc_up_timer_task_priority(struct tid_ampdu_rx *tid_agg_rx)
{
	TaskHandle_t timer_task_handle;
	timer_task_handle = xTimerGetTimerDaemonTaskHandle();
	tid_agg_rx->tmr_tsk_priority = uxTaskPriorityGet(timer_task_handle);
	if(!tid_agg_rx->tmr_tsk_prio_changed){
		vTaskPrioritySet(timer_task_handle, 30);
	}
}

void nrc_restore_timer_task_priority(struct tid_ampdu_rx *tid_agg_rx)
{
	TaskHandle_t timer_task_handle;
	timer_task_handle = xTimerGetTimerDaemonTaskHandle();
	if(!tid_agg_rx->tmr_tsk_prio_changed){
		return;
	}
	vTaskPrioritySet(timer_task_handle, tid_agg_rx->tmr_tsk_priority);
}

#if 0
void nrc_rx_dump(unsigned char * addr, int len )
{
	int i;
	for ( i = 0; i < len; i++){
			if(i%16 == 0){
    			system_printf("\n");
				system_printf("0x%4x",i);
			}
			system_printf(" %2x",addr[i]);
			if((i%4 == 0) &&(i!= 0))
    			system_printf(" ");
	}
	return;
}
#endif

void nrc_stop_rx_ba_session(struct nrc_wpa_sta *sta, u16 tid)
{

#if 0
	tid_rx = rcu_dereference_protected(sta->ampdu_mlme.tid_rx[tid],
					lockdep_is_held(&sta->ampdu_mlme.mtx));
	if (!test_bit(tid, sta->ampdu_mlme.agg_session_valid))
		return;
#endif
	//V(TT_WPAS, TAG "stop_ba_sess: sta(0x%x), tid(%d) + \n", sta, tid);

	struct tid_ampdu_rx * tid_agg_rx;
#ifdef INCLUDE_STATIC_REORDER_INFO
	tid_agg_rx = &sta->ampdu_mlme.tid_rx[tid];
#else
	tid_agg_rx = sta->ampdu_mlme.tid_rx[tid];
#endif
	if (!tid_agg_rx){
		I(TT_WPAS, TAG "stop_ba_sess: no tid_agg_rx, sta(0x%x), tid(%d) + \n",
					    sta, tid);
		return;
    }

	del_timer_sync(tid_agg_rx->reorder_timer);
	vSemaphoreDelete(tid_agg_rx->reorder_lock);
	os_free(tid_agg_rx->reorder_buf);
	os_free(tid_agg_rx->reorder_time);

#ifndef INCLUDE_STATIC_REORDER_INFO
	os_free(tid_agg_rx);
#else
    #ifdef USE_STATIC_TID_INFO
	tid_agg_rx = NULL;
	#endif
#endif
	//tid_agg_rx->removed = true;
	//V(TT_WPAS, TAG "stop_ba_sess: sta(0x%x), tid(%d) - \n",sta, tid);
	
}

int nrc_check_ampdu_mlme_resources(struct nrc_wpa_sta *sta)
{
	V(TT_WPAS, TAG "Free reorder resource: sta(0x%x) + \n",sta);
	for(int i = 0; i < MAX_TID; i ++ ){
		nrc_stop_rx_ba_session(sta, i);
	}
	V(TT_WPAS, TAG "Free reorder resource: sta(0x%x) - \n", sta);
	return 0;
}


#if 0//Stop rx ba session related function.

/*
 * After accepting the AddBA Request we activated a timer,
 * resetting it after each frame that arrives from the originator.
 */
static void sta_rx_agg_session_timer_expired(struct timer_list *t)
{
	struct tid_ampdu_rx *tid_rx = from_timer(tid_rx, t, session_timer);
	struct sta_info *sta = tid_rx->sta;
	u8 tid = tid_rx->tid;
	unsigned long timeout;

	timeout = tid_rx->last_rx + TU_TO_JIFFIES(tid_rx->timeout);
	if (time_is_after_jiffies(timeout)) {
		mod_timer(&tid_rx->session_timer, timeout);
		return;
	}

#if 0
	ht_dbg(sta->sdata, "RX session timer expired on %pM tid %d\n",
	       sta->sta.addr, tid);
#else
	printk("%s, sta(0x%x), RX session timer expired on %pM tid %d\n",
	       __func__, sta, sta->sta.addr, tid);

#endif
	set_bit(tid, sta->ampdu_mlme.tid_rx_timer_expired);
	ieee80211_queue_work(&sta->local->hw, &sta->ampdu_mlme.work);
}

#endif

void nrc_sta_reorder_release(	  struct tid_ampdu_rx *tid_agg_rx,
												  struct nrxb_head *frames);
void nrc_mac_rx_handler(struct nrxb_head * nrxb_queue );


/*
 * This function makes calls into the RX path, therefore
 * it has to be invoked under RCU read lock.
 */
void nrc_release_reorder_timeout(struct nrc_wpa_sta *sta, int tid)

{
	struct nrxb_head frames;
	struct nrc_wpa_rx_data * rx;
	struct tid_ampdu_rx *tid_agg_rx;

#ifdef INCLUDE_STATIC_REORDER_INFO
	tid_agg_rx = &sta->ampdu_mlme.tid_rx[tid];
#else
	tid_agg_rx = sta->ampdu_mlme.tid_rx[tid];
#endif
	if (!tid_agg_rx){
		I(TT_WPAS, TAG "Rel reorder tout: Not allocated ampdu_mlme\n");
		return;
	}

	if(tid_agg_rx->stored_mpdu_num == 0){
		I(TT_WPAS, TAG "Rel reorder tout: No stored mpdu\n");
		return;
	}
	nrxb_queue_head_init(&frames);
#ifdef REORDER_USE_SEMAPHORE	
	if( xSemaphoreTake( tid_agg_rx->reorder_lock, pdMS_TO_TICKS(50) ) != pdTRUE ){
		I(TT_WPAS, TAG "Rel reorder tout:FAIL Take semaphore, tid(%d)\n",
					    tid_agg_rx->tid);
	}
#endif

	nrc_sta_reorder_release(tid_agg_rx, &frames);

#ifdef REORDER_USE_SEMAPHORE	
	if( xSemaphoreGive( tid_agg_rx->reorder_lock ) != pdTRUE ){
		I(TT_WPAS, TAG "Rel reorder tout:FAIL: Give semaphore tid(%d)\n",
			            tid_agg_rx->tid);
	}
#endif

	nrc_mac_rx_handler(&frames);

}


/* 50ms timeout */
#define HT_RX_REORDER_BUF_TIMEOUT ( (50)/portTICK_PERIOD_MS )

 void sta_rx_agg_reorder_timer_expired(TimerHandle_t xTimer)
{
	struct tid_ampdu_rx *tid_rx = NULL;
	tid_rx = (struct tid_ampdu_rx *)pvTimerGetTimerID(xTimer);
	int tid = tid_rx->tid;
	
	I(TT_WPAS, TAG "\n reorder tmr expired: sta(0x%x), tid(%d) jiffie(%d)+\n",
	               tid_rx->sta, tid_rx->tid, jiffies);
	nrc_release_reorder_timeout(tid_rx->sta, tid_rx->tid);

	I(TT_WPAS, TAG "\n reorder tmr expired: sta(0x%x), tid(%d) jiffie(%d)-\n",
	                tid_rx->sta, tid_rx->tid, jiffies);

}

u16 nrc_start_rx_ba_session(struct nrc_wpa_sta *sta,
				      u8 dialog_token, u16 timeout,
				      u16 start_seq_num, u16 ba_policy, u16 tid,
				      u16 buf_size, bool tx, bool auto_seq)

{


	struct tid_ampdu_rx *tid_agg_rx;
	int i, ret = -EOPNOTSUPP;
	u16 status = WLAN_STATUS_REQUEST_DECLINED;
	u16 max_buf_size;

	if (tid >= MAX_TID) {
		E(TT_WPAS, TAG "STA %pM requests BA session on unsupported tid %d\n",
		  sta->addr, tid);
		goto end;
	}

	if (sta == NULL){
		E(TT_WPAS, TAG "STA info(0x%x) is null.\n", sta, tid);
		goto end;
	}

#if 0 // this is for turn on/off by api.
	if (test_sta_flag(sta, WLAN_STA_BLOCK_BA)) {
		printk("%s, Suspend in progress - Denying ADDBA request (%pM tid %d)\n",
		       __func__, sta->sta.addr, tid);
		goto end;
	}
#endif
	max_buf_size = WLAN_BA_MAX_BUF;

	/* determine default buffer size */
#if 0
	if (buf_size == 0)
		buf_size = (buf_size <= max_buf_size) ? buf_size : max_buf_size;
#else
	buf_size = max_buf_size;
#endif

	/* prepare A-MPDU MLME for Rx aggregation */
#ifdef INCLUDE_STATIC_REORDER_INFO
	tid_agg_rx = &sta->ampdu_mlme.tid_rx[tid];
#else
  #ifdef USE_STATIC_TID_INFO
	tid_agg_rx = &g_tid_rx[tid];
  #else
	tid_agg_rx = os_zalloc(sizeof(tid_agg_rx));
  #endif // USE_STATIC_TID_INFO
#endif  // INCLUDE_STATIC_REORDER_INFO

	if (!tid_agg_rx){
		E(TT_WPAS, TAG "StartBaSession: FAIL: os_malloc e#(0x%x\n",
			            tid_agg_rx);
		goto end;
	}

#ifdef REORDER_USE_SEMAPHORE	
     tid_agg_rx->reorder_lock = xSemaphoreCreateMutex();

	if (!tid_agg_rx->reorder_lock){
		E(TT_WPAS, TAG "StartBaSession: FAIL: CreateMutex e#(0x%x)\n",
			            tid_agg_rx->reorder_lock);
#ifndef INCLUDE_STATIC_REORDER_INFO
		os_free(tid_agg_rx);
#endif
		goto end;

	}else{
		V(TT_WPAS,
		 TAG "SUCC: CreateMutex tid_agg_rx->reorder_lock addr#(0x%x)\n",
		        tid_agg_rx->reorder_lock);
	}
#endif
	
#if 0 // no plan to use yet.
	timer_setup(&tid_agg_rx->session_timer,
		         sta_rx_agg_session_timer_expired, TIMER_DEFERRABLE);
#endif

	tid_agg_rx->reorder_timer = xTimerCreate("rx_reorder_timer",
	                              HT_RX_REORDER_BUF_TIMEOUT,
	                              pdFALSE,
	                              tid_agg_rx,//tid_agg_rx is used by the callback. 
								  sta_rx_agg_reorder_timer_expired);

#if 1 // for debug.
	if(tid_agg_rx->reorder_timer == NULL){
		E(TT_WPAS, TAG "FAIL: Reorder timer creation error#(%d)\n",
	      tid_agg_rx->reorder_timer);
		vSemaphoreDelete(tid_agg_rx->reorder_lock);
#ifndef INCLUDE_STATIC_REORDER_INFO
		os_free(tid_agg_rx);
#endif
		goto end;
	}else{
		V(TT_WPAS, TAG "SUCC: Reorder timer creation, (%d) \n", jiffies);
 	}
#endif

	/* prepare reordering buffer */
#ifdef	INCLUDE_SIMPLE_REORDER_QUEUE
	tid_agg_rx->reorder_buf =
		os_malloc(buf_size * sizeof(struct nrc_wpa_rx_data ));
#else
	tid_agg_rx->reorder_buf =
		os_malloc(buf_size * sizeof(struct nrxb_head));
#endif
	tid_agg_rx->reorder_time =
		os_malloc(buf_size * sizeof(unsigned long));


	if (!tid_agg_rx->reorder_buf || !tid_agg_rx->reorder_time) {
		vSemaphoreDelete(tid_agg_rx->reorder_lock);
		os_free(tid_agg_rx->reorder_buf);
		os_free(tid_agg_rx->reorder_time);
#ifndef INCLUDE_STATIC_REORDER_INFO
		os_free(tid_agg_rx);
#endif
		goto end;
	}

#ifdef INCLUDE_SIMPLE_REORDER_QUEUE 
	for (i = 0; i < buf_size; i++)
		tid_agg_rx->reorder_buf[i] = NULL;
#else
	for (i = 0; i < buf_size; i++)
		nrxb_queue_head_init(&tid_agg_rx->reorder_buf[i]);
#endif
	
	/* update data */
	tid_agg_rx->ssn = start_seq_num;
	tid_agg_rx->head_seq_num = start_seq_num;
	tid_agg_rx->buf_size = buf_size;
	tid_agg_rx->timeout = timeout;
	tid_agg_rx->stored_mpdu_num = 0;
	tid_agg_rx->auto_seq = auto_seq;
	tid_agg_rx->started = false;
	tid_agg_rx->removed = false;
	tid_agg_rx->tid = tid;
	tid_agg_rx->sta = sta;
	tid_agg_rx->tmr_tsk_priority = 0;
	tid_agg_rx->tmr_tsk_prio_changed = 0;
	status = WLAN_STATUS_SUCCESS;

	/* activate it for RX */
#ifndef INCLUDE_STATIC_REORDER_INFO
	sta->ampdu_mlme.tid_rx[tid] = tid_agg_rx;
#endif
    I(TT_WPAS, TAG "%s,ssn(%d),h_sn(%d),bufsz(%d),tout(%d),auto_seq(%d),tid(%d),sta(0x%x)\n",
	  __func__,tid_agg_rx->ssn,tid_agg_rx->head_seq_num,tid_agg_rx->buf_size,
	  tid_agg_rx->timeout, tid_agg_rx->auto_seq, tid_agg_rx->tid,
	  tid_agg_rx->sta);

#if 0 // To do.
	if (timeout) {
		mod_timer(&tid_agg_rx->session_timer, TU_TO_EXP_TIME(timeout));
		tid_agg_rx->last_rx = jiffies;
	}
#endif

end:
	if (status == WLAN_STATUS_SUCCESS) {
		sta->ampdu_mlme.tid_rx_token[tid] = dialog_token;
		V(TT_WPAS, TAG "%s, SUCC, token#(%d)\n",__func__,
		                   sta->ampdu_mlme.tid_rx_token[tid]);
		return status;
	}else{
		I(TT_WPAS, TAG "%s, FAIL \n",__func__);
		return status;
	}
}


#ifndef INCLUDE_SIMPLE_REORDER_QUEUE
static inline bool nrc_rx_reorder_ready(struct tid_ampdu_rx *tid_agg_rx,
					      int index)
{
	// check only whether there is queued frame or not.
	struct nrxb_head *frames = &tid_agg_rx->reorder_buf[index];
	struct nrc_wpa_rx_data *tail = nrxb_peek_tail(frames);

	if (!tail)
		return false;

	/* If a frame is a SDU(not the end) of AMSDU,
	   we can think it is not buffered in reorder queue */

	/* Currently NRC7292 is not using AMSDU, so Disable */
#if 0
	status = IEEE80211_SKB_RXCB(tail);
	if (status->flag & RX_FLAG_AMSDU_MORE)
		return false;
#endif

	return true;
}
#endif


static void nrc_release_reorder_frame(	    struct tid_ampdu_rx *tid_agg_rx,
					    int index,
					    struct nrxb_head *frames)
{

#ifdef INCLUDE_SIMPLE_REORDER_QUEUE 
	struct nrc_wpa_rx_data *nrxb = tid_agg_rx->reorder_buf[index];
#else
	struct nrxb_head *nrxb_list = &tid_agg_rx->reorder_buf[index];
	struct nrc_wpa_rx_data *nrxb;
#endif

#ifdef INCLUDE_SIMPLE_REORDER_QUEUE
	if(!nrxb){
		goto no_frame;
	}
#else
	if (nrxb_queue_empty(nrxb_list)){
		//I(TT_WPAS, TAG "%s,no frame(empty) in (%d)th buf\n",__func__, index);
		goto no_frame;
	}
#endif

	/* Release frames from the reorder ring buffer */
	tid_agg_rx->stored_mpdu_num--;

#ifdef INCLUDE_SIMPLE_REORDER_QUEUE
	tid_agg_rx->reorder_buf[index] = NULL;
	nrxb_queue_tail(frames, nrxb);	
	GenericMacHeader *gmh = (GenericMacHeader *) nrxb->u.hdr;
	u16 sc = gmh->sequence_number;
	I(TT_WPAS, TAG "%s, dequeue frame sn(%d) f r buf idx(%d), tid_agg_rx->stored_mpdu_num(%d) \n",
			   __func__, sc, index, tid_agg_rx->stored_mpdu_num);	
	
#else
	while ((nrxb = __nrxb_dequeue(nrxb_list)) ) {
		nrxb_queue_tail(frames, nrxb);
#if 0		
		GenericMacHeader *gmh = (GenericMacHeader *) nrxb->u.hdr;
    	u16 sc = gmh->sequence_number;
		I(TT_WPAS, TAG "%s, dequeue frame sn(%d) f r buf idx(%d), tid_agg_rx->stored_mpdu_num(%d) \n",
			       __func__, sc, index, tid_agg_rx->stored_mpdu_num); 	
#endif
		if( (tid_agg_rx->stored_mpdu_num == 0) ||
		     (nrxb_list->qlen == 0 ) )
			break;
	}
#endif

no_frame:
	tid_agg_rx->head_seq_num = ieee80211_sn_inc(tid_agg_rx->head_seq_num);
}


static void nrc_release_reorder_frames(	     struct tid_ampdu_rx *tid_agg_rx,
										     u16 head_seq_num,
										     struct nrxb_head *frames)
{
	int index;

	while (ieee80211_sn_less(tid_agg_rx->head_seq_num, head_seq_num) &&
		    tid_agg_rx->stored_mpdu_num > 0 ) {
		index = tid_agg_rx->head_seq_num % tid_agg_rx->buf_size;
		
		I(TT_WPAS, TAG "%s,sta(0x%x),rbufidx(%d), tid(%d), head_seq_num(%d), stored_mpdu#(%d)\n",
		      __func__, tid_agg_rx->sta, index,tid_agg_rx->tid,
		 	     tid_agg_rx->head_seq_num,tid_agg_rx->stored_mpdu_num);
	
     	nrc_release_reorder_frame(tid_agg_rx, index, frames);
		if(tid_agg_rx->stored_mpdu_num < 1 ){
			tid_agg_rx->head_seq_num = head_seq_num;
		}
	}
}

void nrc_sta_reorder_release(	  struct tid_ampdu_rx *tid_agg_rx,
					  struct nrxb_head *frames)
{

#ifdef INCLUDE_SIMPLE_REORDER_QUEUE
	int index, j;
#else
	int index, i, j;
#endif
	
	/* release the buffer until next missing frame */
	index = tid_agg_rx->head_seq_num % tid_agg_rx->buf_size;
#ifdef INCLUDE_SIMPLE_REORDER_QUEUE
	if( !tid_agg_rx->reorder_buf[index] &&
		 tid_agg_rx->stored_mpdu_num) {
#else
	if (!nrc_rx_reorder_ready(tid_agg_rx, index) &&
	    tid_agg_rx->stored_mpdu_num) {
#endif	    
		/*
		 * No buffers ready to be released, but check whether any
		 * frames in the reorder buffer have timed out.
		 */
		int skipped = 1;
		for (j = (index + 1) % tid_agg_rx->buf_size;
		     j != index;
		     j = (j + 1) % tid_agg_rx->buf_size) {

#ifdef INCLUDE_SIMPLE_REORDER_QUEUE
			if (!tid_agg_rx->reorder_buf[j]){
#else
			if (!nrc_rx_reorder_ready(tid_agg_rx, j)) {
#endif
				skipped++;
				continue;
			}
#if 1			
			// jhkim not a timeout case. so update timer.
			if (skipped &&
			    !time_after(jiffies, tid_agg_rx->reorder_time[j] +
			    HT_RX_REORDER_BUF_TIMEOUT) )
				goto set_release_timer;
#else
			if (skipped &&
			    !time_after(jiffies, tid_agg_rx->reorder_time[j]) )
				goto set_release_timer;

#endif
			//jhkim : when timeout occurs on j idx.
			/* don't leave incomplete A-MSDUs around */

#ifdef INCLUDE_SIMPLE_REORDER_QUEUE
			I(TT_WPAS, TAG "%s Release an RX reorder frame idx(%d) due to timeout\n",
			  __func__,j);
			nrc_release_reorder_frame(tid_agg_rx, j, frames);
#else
			if(tid_agg_rx->stored_mpdu_num > 1){
				for (i = (index + 1) % tid_agg_rx->buf_size; i != j;
				     i = (i + 1) % tid_agg_rx->buf_size){
					__nrxb_queue_purge(&tid_agg_rx->reorder_buf[i]);
					I(TT_WPAS, TAG "%s, after __nrxb_queue_purge, i(%d), j tout(%d), skipped(%d)\n",
					  __func__, i, j, skipped);
				}
			}
			I(TT_WPAS, TAG "%s Release an RX reorder frame idx(%d) due to timeout\n",
			  __func__,j);
			nrc_release_reorder_frame(tid_agg_rx, j, frames);
#endif			
			/*
			 * Increment the head seq# also for the skipped slots.
			 */
			tid_agg_rx->head_seq_num = (tid_agg_rx->head_seq_num +
						                skipped) & IEEE80211_SN_MASK;
			skipped = 0;

			if(tid_agg_rx->stored_mpdu_num == 0)
				break;
		}

#ifdef INCLUDE_SIMPLE_REORDER_QUEUE
	} else while (tid_agg_rx->reorder_buf[index] ) {

#else
	} else while (nrc_rx_reorder_ready(tid_agg_rx, index)) {
#endif	
		I(TT_WPAS, TAG "%s, got head sn, so release\n",__func__);
		nrc_release_reorder_frame(tid_agg_rx, index, frames);
		index =	tid_agg_rx->head_seq_num % tid_agg_rx->buf_size;
	}

	if (tid_agg_rx->stored_mpdu_num) {
		j = index = tid_agg_rx->head_seq_num % tid_agg_rx->buf_size;

		for (; j != (index - 1) % tid_agg_rx->buf_size;
		     j = (j + 1) % tid_agg_rx->buf_size) {
#ifdef INCLUDE_SIMPLE_REORDER_QUEUE
			if ( tid_agg_rx->reorder_buf[j] )
#else			 	
			if (nrc_rx_reorder_ready(tid_agg_rx, j))
#endif				
				break;
		}

 set_release_timer:
		I(TT_WPAS, TAG "%s, renewing timer now,prio_changed(%d),removed(%d) \n",
			__func__, tid_agg_rx->tmr_tsk_prio_changed, tid_agg_rx->removed);
		if (!tid_agg_rx->removed){
			mod_timer(tid_agg_rx->reorder_timer, HT_RX_REORDER_BUF_TIMEOUT);
			I(TT_WPAS, TAG "%s, mod_timer:idx[%d] org(%d), new tout(%d)\n",__func__,j,
				   tid_agg_rx->reorder_time[j], tid_agg_rx->reorder_time[j] +
				  HT_RX_REORDER_BUF_TIMEOUT);
#if 1
			if(!tid_agg_rx->tmr_tsk_prio_changed){
				unsigned long flags = system_irq_save();
				nrc_up_timer_task_priority(tid_agg_rx);
				system_irq_restore(flags);
				tid_agg_rx->tmr_tsk_prio_changed = true;
			}
#endif
	    }
	} else {
		I(TT_WPAS, TAG "%s, stopping timer now, prio_changed(%d)\n",
			__func__,tid_agg_rx->tmr_tsk_prio_changed);
		xTimerStop(tid_agg_rx->reorder_timer, 0);
#if 1		
		if(tid_agg_rx->tmr_tsk_prio_changed){
			unsigned long flags = system_irq_save();
			nrc_restore_timer_task_priority(tid_agg_rx);
			system_irq_restore(flags);
			tid_agg_rx->tmr_tsk_prio_changed = false;
		}
#endif
	}
}
 
static bool nrc_sta_manage_reorder_buf(	     struct tid_ampdu_rx *tid_agg_rx,
													struct nrc_wpa_rx_data *nrxb,
                             					     struct nrxb_head *frames)
{
	u16 head_seq_num, buf_size;
	int index;
	bool ret = true;
	unsigned long flags;

	GenericMacHeader * gmh = (GenericMacHeader *)nrxb->u.hdr;
	u16 mpdu_seq_num  = gmh->sequence_number;

	//V(TT_WPAS, TAG "%s,tid(%d),sn(%d),h_sn(%d)\n",
	//		__func__, tid_agg_rx->tid, mpdu_seq_num, tid_agg_rx->head_seq_num  );

#ifdef REORDER_USE_SEMAPHORE	
	if( xSemaphoreTake( tid_agg_rx->reorder_lock, pdMS_TO_TICKS(50) ) != pdTRUE ){
	    E(TT_WPAS, TAG "%s, FAIL: Take semaphore, tid(%d)\n",
		 	           __func__, tid_agg_rx->tid);
	}
#endif

	V(TT_WPAS, TAG "Took SEMAPHORE %s,tid(%d),sn(%d),h_sn(%d)\n",
			__func__, tid_agg_rx->tid, mpdu_seq_num, tid_agg_rx->head_seq_num  );
	/*
	 * Offloaded BA sessions have no known starting sequence number so pick
	 * one from first Rxed frame for this tid after BA was started.
	 */
	if (tid_agg_rx->auto_seq) {
		tid_agg_rx->auto_seq = false;
		tid_agg_rx->ssn = mpdu_seq_num;
		tid_agg_rx->head_seq_num = mpdu_seq_num;
		I(TT_WPAS,
			TAG "%s, sta(0x%x)tid(%d),auto_seq: cur_sn(%d), tid_agg_rx->ssn(%d), head_seq_num(%d)+\n",
			 __func__, tid_agg_rx->sta,tid_agg_rx->tid, mpdu_seq_num,  tid_agg_rx->ssn, tid_agg_rx->head_seq_num );
	}

	buf_size = tid_agg_rx->buf_size;
	head_seq_num = tid_agg_rx->head_seq_num;

	/*
	 * If the current MPDU's SN is smaller than the SSN, it shouldn't
	 * be reordered.
	 */
	if (!(tid_agg_rx->started)) {
		if (ieee80211_sn_less(mpdu_seq_num, head_seq_num)) {
			ret = false;
			I(TT_WPAS, TAG "%s, Not started, forward : Lower sn : cur sn(%d), haed_sn(%d)\n",
					__func__, mpdu_seq_num, head_seq_num );
			goto out;
		}
		tid_agg_rx->started = true;
	}

	/* frame with out of date sequence number */
	if (ieee80211_sn_less(mpdu_seq_num, head_seq_num)) {
		os_free(nrxb->u.frame);
		nrxb->u.frame = NULL;
		free_nrxb(nrxb);
		nrxb = NULL;
		I(TT_WPAS, TAG "%s, OoO,Free nrxb: cur sn(%d), hd_sn(%d)\n",
				__func__, mpdu_seq_num, head_seq_num );
		goto out;
	}

	/*
	 * If frame the sequence number exceeds our buffering window
	 * size release some previous frames to make room for this one.
	 */
	if (!ieee80211_sn_less(mpdu_seq_num, head_seq_num + buf_size)) {
#if 0 // origin
		head_seq_num = ieee80211_sn_inc(
				ieee80211_sn_sub(mpdu_seq_num, buf_size));
#else // move sliding window as half of buffer size.
		head_seq_num = ieee80211_sn_inc(
				ieee80211_sn_sub(mpdu_seq_num, buf_size-2)); 

#endif
		/* release stored frames up to new head to stack */
		I(TT_WPAS,
		  TAG "%s, sta(0x%x),Rel buffered frames before new caled hd_sn(%d). Cur frame#(%d) exceeds windows\n",
	       __func__, tid_agg_rx->sta, head_seq_num, mpdu_seq_num);
		flags = system_irq_save();
		nrc_release_reorder_frames( tid_agg_rx, head_seq_num, frames);
		system_irq_restore(flags);
	}

	/* Now the new frame is always in the range of the reordering buffer */
	index = mpdu_seq_num % tid_agg_rx->buf_size;

	/* check if we already stored this frame */
#ifdef INCLUDE_SIMPLE_REORDER_QUEUE
	if( tid_agg_rx->reorder_buf[index] ){
#else
	if (nrc_rx_reorder_ready(tid_agg_rx, index)) {
#endif		
		os_free(nrxb->u.frame);
		nrxb->u.frame = NULL;
		free_nrxb(nrxb);
		nrxb = NULL;
		I(TT_WPAS, TAG "%s,sta(0x%x),b_idx(%d)Free, already stored:cur sn(%d), hd_sn(%d)\n",
        	__func__, tid_agg_rx->sta, index, mpdu_seq_num, head_seq_num );
		goto out;
	}

	/*
	 * If the current MPDU is in the right order and nothing else
	 * is stored we can process it directly, no need to buffer it.
	 * If it is first but there's something stored, we may be able
	 * to release frames after this one.
	 */
	if (mpdu_seq_num == tid_agg_rx->head_seq_num &&
	    tid_agg_rx->stored_mpdu_num == 0) {
		if (!(ieee80211_is_frag(gmh)))
			tid_agg_rx->head_seq_num = ieee80211_sn_inc(tid_agg_rx->head_seq_num);
		ret = false;
		I(TT_WPAS, TAG "%s, Nomal sn, forward directly: sta(0x%x), cur sn(%d), haed_sn(%d)\n",
			__func__, tid_agg_rx->sta, mpdu_seq_num, tid_agg_rx->head_seq_num );
		goto out;
	}

	/* put the frame in the reordering buffer */
#ifdef INCLUDE_SIMPLE_REORDER_QUEUE
	tid_agg_rx->reorder_buf[index] = nrxb;
#else
	nrxb_queue_tail(&tid_agg_rx->reorder_buf[index], nrxb);
#endif

    I(TT_WPAS, TAG "%s, sta(0x%x),Put frame to reorder buf index(%d), cur sn(%d), haed_sn(%d)\n",
      __func__,tid_agg_rx->sta, index, mpdu_seq_num, head_seq_num);
		
	if (!(ieee80211_is_frag(gmh) )) {
		tid_agg_rx->reorder_time[index] = jiffies;
		tid_agg_rx->stored_mpdu_num++;
		flags = system_irq_save();
		nrc_sta_reorder_release(tid_agg_rx, frames);
		system_irq_restore(flags);
	}

 out:
#ifdef REORDER_USE_SEMAPHORE	
	if( xSemaphoreGive( tid_agg_rx->reorder_lock ) != pdTRUE ){
		E(TT_WPAS, TAG "%s, FAIL: Give semaphore tid(%d)\n",__func__, tid_agg_rx->tid);
	}
#endif

	return ret;
}

extern int get_qos_tid(GenericMacHeader *gmh);
/*
 * Reorder MPDUs from A-MPDUs, keeping them on a buffer. Returns
 * true if the MPDU was buffered, false if it should be processed.
 */

void nrc_rx_reorder_ampdu(struct nrc_wpa_rx_data *rx,
				                struct nrxb_head *frames)
{
	struct nrc_wpa_rx_data * nrxb = rx;
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *) rx->u.hdr;
	struct nrc_wpa_sta *sta = rx->sta;
	struct tid_ampdu_rx *tid_agg_rx;
	u16 sc;
	u8 tid, ack_policy;

	//goto dont_reorder;

	if (!sta ){
		goto dont_reorder;
	}

	GenericMacHeader * gmh = (GenericMacHeader *)hdr;

	if (!ieee80211_is_qos_data(gmh )||
	    ieee80211_group_addr(hdr->addr1)){
		goto dont_reorder;
	}

	/*
	 * filter the QoS data rx stream according to
	 * STA/TID and check if this STA/TID is on aggregation
	 */

#if 0
	ack_policy =  (* ieee80211_get_qos_ctl(gmh) &
			     IEEE80211_QOS_CTL_ACK_POLICY_MASK);
#endif	
	tid = (u8)get_qos_tid(gmh);
	//V(TT_WPAS, TAG "%s,ack_policy(0x%x),tid(%d),sn(%d)\n",
	//		__func__, ack_policy, tid, gmh->sequence_number );

	if(sta->block_ack[tid] & BLOCK_ACK_RX ){
  #ifdef INCLUDE_STATIC_REORDER_INFO
		tid_agg_rx = &sta->ampdu_mlme.tid_rx[tid];
  #else
		tid_agg_rx = sta->ampdu_mlme.tid_rx[tid];
  #endif
	}else{
		I(TT_WPAS,
			TAG "sta(0x%x),tid(%d),ba_flag(0x%x) Not Accepted\n",
			     sta, tid, sta->block_ack[tid]);
		goto dont_reorder;
	}

	if (!tid_agg_rx) {
		I(TT_WPAS,
			TAG "%s,tid(%d) No Allocated tid_agg_rx\n",
			__func__, tid);
		//send_delba();
		goto dont_reorder;
	}
	
	if ( ieee80211_is_qos_null(gmh)){
		//V(TT_WPAS, TAG "%s, QoS null so no reorder \n",__func__, tid);
		goto dont_reorder;
	}

#if 0 //need to check.
	/* not part of a BA session */
	if (ack_policy != IEEE80211_QOS_CTL_ACK_POLICY_BLOCKACK &&
	    ack_policy != IEEE80211_QOS_CTL_ACK_POLICY_NORMAL){
		I(TT_WPAS, TAG "%s,This frame does't have ba ack policy(0x%x)",
			__func__, ack_policy);    
		goto dont_reorder;
	}
#endif

	/* new, potentially un-ordered, ampdu frame - process it */

#if 1 // need to fix.
	if (tid_agg_rx->timeout){
		tid_agg_rx->last_rx = jiffies;
    	//I(TT_WPAS, TAG "%s, tout(%d), last_rx(%d) \n",
        //  __func__,tid_agg_rx->timeout, tid_agg_rx->last_rx );
	}
#endif


#if 0 // To block fragmented frame.
	/* if this mpdu is fragmented - terminate rx aggregation session */
	//sc = le16_to_cpu(hdr->seq_ctrl);
	//sc = hdr->seq_ctrl;
	//if (sc & IEEE80211_SCTL_FRAG) {
	if (ieee80211_is_frag(gmh)){
#if 0  //To do		
		//skb_queue_tail(&rx->sdata->skb_queue, skb);
    	//printk("%s,fragmented frame, so terminate rx agg session\n",__func__);    		
		//ieee80211_queue_work(&local->hw, &rx->sdata->work);
		//return;
#else // temp.
		I(TT_WPAS, TAG "%s,!!!!! Caution : Fragmented frame sn(%d), sz(%d)!!!!",
		                __func__, gmh->sequence_number, rx->len);
		goto dont_reorder; // temp
#endif
	}
#endif 
	/*
	 * No locking needed -- we will only ever process one
	 * RX packet at a time, and thus own tid_agg_rx. All
	 * other code manipulating it needs to (and does) make
	 * sure that we cannot get to it any more before doing
	 * anything with it.
	 */
	if ( nrc_sta_manage_reorder_buf(tid_agg_rx, rx, frames) )
		 return;

 dont_reorder:
	nrxb_queue_tail(frames, rx);

}


#ifdef TAG
#undef TAG
#endif
