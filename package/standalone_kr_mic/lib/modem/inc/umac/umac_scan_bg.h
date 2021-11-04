#ifndef UMAC_SCAN_BG_H
#define UMAC_SCAN_BG_H

#include "system_common.h"

#if defined(CONFIG_BG_SCAN)
bool		bg_scan_filter_frame(SYS_BUF *buf);
void		on_qm_idle();
void		umac_scan_bg_init(uint8_t vif_id, void (*cb)());
void		umac_scan_bg_deinit();
void		umac_scan_bg_start(uint8_t vif_id);
void		umac_scan_bg_done();
#else
STATIC_INLINE_FUNC(bg_scan_queue_push, int, return 1, SYS_BUF *packet);
STATIC_INLINE_FUNC(bg_scan_filter_frame, bool, return false);
STATIC_INLINE_FUNC(on_qm_idle, void, return);
STATIC_INLINE_FUNC(umac_scan_bg_init, void, return, int vif_id, void (*cb)());
STATIC_INLINE_FUNC(umac_scan_bg_deinit, void, return);
STATIC_INLINE_FUNC(umac_scan_bg_start, void, return);
STATIC_INLINE_FUNC(umac_scan_bg_done, void, return);
#endif /* defined(CONFIG_BG_SCAN) */

#endif /* UMAC_SCAN_BG_H */