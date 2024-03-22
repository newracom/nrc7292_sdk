#ifndef HAL_LMAC_UTIL_H
#define HAL_LMAC_UTIL_H

#if defined(INCLUDE_RXGAIN_CONTROL)
typedef struct _RXGAIN_CTRL {
	bool g_auto_rxgain;	//(1:enable 1:disable)
	int initial_rx_gain;	//
	int g_rssi_tr_adjust;	//rssi threshold to adjust rxgain
	int g_rssi_tr_restore;	//rssi threshold to restore rxgain
	uint32_t g_adjust_rxgain;	//rxgain to be adjusted
	bool g_autorxgain_print;	//for debug print
} RXGAIN_CTRL;

void lmac_rxgain_ctrl_init();
uint32_t lmac_rxgain_ctrl_enable(bool enable);
bool lmac_rxgain_ctrl_set(int adjust_rssi, int restore_rssi, uint32_t target_rxgain);
void lmac_rxgain_ctrl_set_debug(bool enable);
int lmac_rxgain_ctrl_operation(uint8_t vif_id, int rssi_avg);
void lmac_rxgain_ctrl_restore_intial_rxgain();
void lmac_rxgain_ctrl_show();
#endif /* INCLUDE_RXGAIN_CONTROL */

uint32_t lmac_bytes_to_bufend(struct _SYS_BUF* sys_buf, uint8_t *ptr);
void    lmac_init_null_frame();
void    lmac_send_null_frame(void(*callback)());
bool	lmac_send_qos_null_frame(bool pm);
uint8_t* lmac_get_macaddr_magic_code(void);
bool    lmac_check_sf_macaddr(uint8_t *mac, int vif_id);
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
bool lmac_defrag_sync_rx_pn(uint8_t vif_id, GenericMacHeader *hdr);
DEFRAG_ENTRY *lmac_defrag_add(DEFRAG_INFO *cache, unsigned int frag_n, uint16_t sn, uint8_t rx_queue, SYS_BUF * sysbuf);
DEFRAG_ENTRY *lmac_defrag_find_entry(DEFRAG_INFO *cache, 	unsigned int frag_n, unsigned int sn, uint8_t rx_queue, struct lmac_rx_h_data * rx);
void lmac_sysbuf_queue_purge(struct sysbuf_queue *hque);
#endif /* INCLUDE_DEFRAG */
#if defined(CSPI)
void lmac_send_ps_pretend(uint8_t vif_id, GenericMacHeader *mqh);
#endif

uint32_t util_tsf_diff(uint32_t start_tsf, uint32_t end_tsf);
uint8_t hex_to_int(char c);

#if defined(INCLUDE_TWT_SUPPORT)
void    lmac_twt_send_pspoll();
void    lmac_twt_send_null_frame(uint8_t pm);
#endif /* defined(INCLUDE_TWT_SUPPORT) */
int util_str_to_bandwidth(const char *str);
int util_str_to_prim_ch_bandwidth(const char *str);
bool util_str_onoff(const char *str);
#endif

#if defined (INCLUDE_IBSS)
void lmac_set_edca_cw(int vif_id, uint8_t dst_ac, uint8_t src_ac);
#endif

#if defined(INCLUDE_AUTH_CONTROL)
#include "hal_sflash.h"
void get_auth_ctrl_param();
#endif /* defined(INCLUDE_AUTH_CONTROL) */

/* Sync to AP about PM */
bool lmac_notify_pm_to_ap (void);
int8_t ewma_val(int8_t old, int8_t new, int8_t weight);


