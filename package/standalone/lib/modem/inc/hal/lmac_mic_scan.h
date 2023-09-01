#ifndef LMAC_MIC_SCAN_H
#define LMAC_MIC_SCAN_H

#include "system_common.h"
#include "lmac_debug.h"
#include "lmac_common.h"
#include "system_modem_api.h"
#include "lmac_drv.h"
#include "hal_timer.h"

/* FixME : just considering KR Wireless MIC Band */

#if defined(INCLUDE_MIC_SCAN)
#define MIC_START_FREQ		9250 //Start Freq : 9250MHz
#define MIC_SCAN_LIST_SIZE	14  //SCAN CH Count
#define MIC_1M_DIFF			10	//1MHz
#define MIC_SCAN_RANGE		20	//2MHz (Scan within +- 2MHz Range)
#define MIC_FREQ_INTERVAL	5	//0.5MHz
#define MIC_SHORT_SCAN_RATIO	10	//decrease scan interval ratio
//#define FREQ_TO_IDX(freq)	(freq - MIC_FREQ_SCAN_START) / MIC_1M_DIFF
//1M CH index => 9255(1) 9265(3) 9275(5) 9285(7) 9295(9) 9305(11), 9315(13-only for micscan)
//2M CH index => 9270(4) 9290(8)
#define FREQ_TO_IDX(freq)	((freq - MIC_START_FREQ) / MIC_FREQ_INTERVAL)
#define IDX_TO_FREQ(index)	MIC_START_FREQ + (index * MIC_FREQ_INTERVAL)
#define BW_FREQ_OFFSET(bw)  bw == BW_1M ? 0 : bw == BW_2M ? 5 : 15
void lmac_mic_scan_timer_init();
void lmac_mic_scan_periodic_start(bool release);
void lmac_mic_scan_periodic_stop();
void lmac_mic_scan_tx_timer_set();
void lmac_mic_scan_tx_timer_reset();
void lmac_mic_scan_tx_timer_clear();
bool lmac_mic_scan_pre_scan(uint8_t vif_id, uint16_t *ch_list, uint8_t *ch_num);
void lmac_mic_scan_get_move(bool *enable);
void lmac_mic_scan_set_move(bool enable);
void lmac_mic_scan_status(bool *enable);
void lmac_mic_scan_enable(void);
void lmac_mic_scan_disable(void);
void lmac_mic_scan_clear_detect_cnt();
uint32_t lmac_mic_scan_get_detect_cnt();
#else
STATIC_INLINE_FUNC(lmac_mic_scan_timer_init, void, return);
STATIC_INLINE_FUNC(lmac_mic_scan_periodic_start, void, return, bool release);
STATIC_INLINE_FUNC(lmac_mic_scan_periodic_stop, void, return);
STATIC_INLINE_FUNC(lmac_mic_scan_tx_timer_set, void, return);
STATIC_INLINE_FUNC(lmac_mic_scan_tx_timer_reset, void, return);
STATIC_INLINE_FUNC(lmac_mic_scan_tx_timer_clear, void, return);
STATIC_INLINE_FUNC(lmac_mic_scan_pre_scan, bool, return true, uint8_t vif_id, uint16_t *ch_list, uint8_t *ch_num);
STATIC_INLINE_FUNC(lmac_mic_scan_get_move, void, return, bool *enable);
STATIC_INLINE_FUNC(lmac_mic_scan_set_move, void, return, bool enable);
STATIC_INLINE_FUNC(lmac_mic_scan_status, void, return, bool *enable);
STATIC_INLINE_FUNC(lmac_mic_scan_enable, void, return);
STATIC_INLINE_FUNC(lmac_mic_scan_disable, void, return);
STATIC_INLINE_FUNC(lmac_mic_scan_clear_detect_cnt, void, return);
STATIC_INLINE_FUNC(lmac_mic_scan_get_detect_cnt, uint32_t, return 0);
#endif /* defined(INCLUDE_MIC_SCAN) */

#endif /* LMAC_MIC_SCAN */
