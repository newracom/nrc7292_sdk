#ifndef HAL_LMAC_UTIL_H
#define HAL_LMAC_UTIL_H

#if defined(INCLUDE_RXGAIN_CONTROL)
typedef struct _RXGAIN_CTRL_PARAM {
	int8_t rssi_tr_adjust;		//rssi threshold to adjust rxgain
	int8_t rssi_tr_restore;		//rssi threshold to restore rxgain
	uint8_t sample_num;			//max sample count
	uint32_t adjust_rxgain;		//rxgain to be adjusted
	uint32_t adjust_cca_threshold; //cca threshold to be adjusted
	uint32_t time_restore_ms;	//timer to restore rxgain
} RXGAIN_CTRL_PARAM;

typedef struct _RXGAIN_CTRL {
	bool g_auto_rxgain;	//(1:enable 1:disable)
	bool g_auto_rxgain_init;	//init flag
	bool g_initial_ndp;			//initial NDP filter
	uint8_t sample_cur;			//current sample
	int8_t rssi_min;			//min rssi in samples
	int8_t *rssi_sample;		//rssi samples
	RXGAIN_CTRL_PARAM param;	//parameters
	int initial_rx_gain;	//initial rxgain
	uint32_t initial_cca_threshold; //initial cca threshold
	uint32_t g_autorxgain_print;//for debug print
	uint32_t rxcnt;
	TimerHandle_t debug_timer_handle; // debug timer handle
	SemaphoreHandle_t data_mtx;	//data protection
} RXGAIN_CTRL;

bool lmac_rxgain_ctrl_init(RXGAIN_CTRL_PARAM* param);
void lmac_rxgain_ctrl_deinit(void);
uint32_t lmac_rxgain_ctrl_enable(bool enable);
bool lmac_rxgain_ctrl_set_initial_cca_thresh(uint32_t cca_threshold);
bool lmac_rxgain_ctrl_set_initial_rxgain(int init_rxgain);
bool lmac_rxgain_ctrl_set_debug(uint32_t time_sec);
int8_t lmac_rxgain_ctrl_operation(uint8_t vif_id, int8_t rssi, bool isr);
bool lmac_rxgain_ctrl_show(char *buf, uint32_t len);
bool lmac_rxgain_ctrl_get_enable();
void _lmac_check_rxgain_ctrl_idle();
#endif /* INCLUDE_RXGAIN_CONTROL */

uint32_t lmac_bytes_to_bufend(struct _SYS_BUF* sys_buf, uint8_t *ptr);
void    lmac_init_null_frame();
void    lmac_send_callback(int vif_id, void *callback, const uint8_t *addr);
bool    lmac_send_null_frame(bool pm, void(*callback)());
bool    lmac_send_qos_null_frame(bool pm, void(*callback)(), QoSField *qc);
uint8_t* lmac_get_macaddr_magic_code(void);
bool    lmac_check_sf_macaddr(uint8_t *mac, int vif_id);
void    lmac_send_ndp(uint8_t vif_id, SigNdp1M *sig);
#if defined(INCLUDE_DYNAMIC_FRAG)
void lmac_uplink_update_pn(uint8_t vif_id, GenericMacHeader *hdr, bool increase);
bool lmac_uplink_fragment_frame(LMAC_TXBUF *ori_buf, uint8_t ac, uint16_t fragSize);
#endif /* INCLUDE_DYNAMIC_FRAG */
#if defined(INCLUDE_TSF_SYNC_PROBE_REQ)
void lmac_send_probe_req_tsf_sync(int8_t vif_id);
#endif

#if defined(INCLUDE_DEFRAG)
#include "lmac_downlink.h"
#include "umac_info.h"

void lmac_defrag_init_cache(DEFRAG_INFO *cache);
void lmac_defrag_destroy_cache(DEFRAG_INFO *cache);
bool lmac_defrag_reassemble(SYS_BUF * dst_sbuf, struct sysbuf_queue *dfque);
bool lmac_defrag_sync_rx_pn(uint8_t vif_id, GenericMacHeader *hdr, STAINFO *sta);
DEFRAG_ENTRY *lmac_defrag_add(DEFRAG_INFO *cache, unsigned int frag_n, uint16_t sn, uint8_t rx_queue, SYS_BUF * sysbuf);
DEFRAG_ENTRY *lmac_defrag_find_entry(DEFRAG_INFO *cache, 	unsigned int frag_n, unsigned int sn, uint8_t rx_queue, struct lmac_rx_h_data * rx);
void lmac_sysbuf_queue_purge(struct sysbuf_queue *hque);
#endif /* INCLUDE_DEFRAG */
#if defined(CSPI)
void lmac_send_ps_pretend(uint8_t vif_id, GenericMacHeader *mqh);
void lmac_send_rxbuf_to_host (uint8_t vif_id, LMAC_RXBUF *lmac_buf);
#endif

uint32_t util_tsf_diff(uint32_t start_tsf, uint32_t end_tsf);
uint8_t hex_to_int(char c);

#if defined(INCLUDE_TWT_SUPPORT)
void lmac_send_action(uint8_t vif_id, uint8_t *destaddr, ActionFrame *action, uint16_t len, bool protect, bool no_ack);
#endif /* defined(INCLUDE_TWT_SUPPORT) */
int util_str_to_bandwidth(const char *str);
int util_str_to_prim_ch_bandwidth(const char *str);
bool util_str_onoff(const char *str);
#endif

#if defined (INCLUDE_IBSS)
void lmac_set_edca_cw(int vif_id, uint8_t dst_ac, uint8_t src_ac);
#endif

#if defined(INCLUDE_AUTH_CONTROL)
typedef struct _lmac_auth_ctrl_t{
	uint16_t BI;
	uint8_t SCALE;
	uint8_t T_SLOT;
	uint8_t TI_MIN;
	uint8_t TI_MAX;
	uint8_t CUR_TI;
} lmac_auth_ctrl_t;

lmac_auth_ctrl_t* lmac_get_auth_control_param(void);
void lmac_add_task_delay(uint8_t vif_id);
bool lmac_reset_auth_current_ti(void);
void lmac_set_auth_control_bi(uint16_t bi);
uint16_t lmac_get_auth_control_bi();
void lmac_set_auth_control_scale(uint8_t scale);
uint8_t lmac_get_auth_control_scale();
bool lmac_set_auth_current_ti(void);
bool lmac_get_auth_current_ti(int *ti);
bool lmac_set_auth_control_param(uint8_t slot, uint8_t ti_min, uint8_t ti_max);
#endif /* defined(INCLUDE_AUTH_CONTROL) */

/* Sync to AP about PM */
bool lmac_notify_pm_to_ap (void);
int8_t ewma_val(int8_t old, int8_t new, int8_t weight);


