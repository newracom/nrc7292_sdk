#ifndef __UMAC_WIM_BUILDER_H__
#define __UMAC_WIM_BUILDER_H__

#if defined(NRC_ROMLIB)
#include "romlib.h"
#endif /* defined(NRC_ROMLIB) */

#if defined(NRC_ROMLIB)
#define WIM_BUILDER_CREATE(a,b,c)			wim_builder_create_fw(a,b,c)
#define WIM_BUILDER_APPEND(a,b,c,d)			wim_builder_append(a,b,c,d)
#define WIM_BUILDER_RUN_WM(a,b)				wim_builder_run_wm_fw(a,b)
#define WIM_BUILDER_RUN_WM_WQ(a, b)			wim_builder_run_wm_wq(a, b)
#define WIM_BUILDER_DESTROY(a)				wim_builder_destroy_fw(a)
#define WIM_BUILDER_MAKE_HIF(a,b,c,d,e)		wim_builder_make_hif_fw(a,b,c,d,e)
#define WIM_BUILDER_MAKE_EVENT(a,b,c,d)		wim_builder_make_event_fw(a,b,c,d)
#else
#define WIM_BUILDER_CREATE(a,b,c) 			wim_builder_create(a,b,c)
#define WIM_BUILDER_APPEND(a,b,c,d) 		wim_builder_append(a,b,c,d)
#define WIM_BUILDER_RUN_WM(a,b)				wim_builder_run_wm(a,b)
#define WIM_BUILDER_RUN_WM_WQ(a, b)			wim_builder_run_wm_wq(a, b)
#define WIM_BUILDER_DESTROY(a)				wim_builder_destroy(a)
#define WIM_BUILDER_MAKE_HIF(a,b,c,d,e)		wim_builder_make_hif(a,b,c,d,e)
#define WIM_BUILDER_MAKE_EVENT(a,b,c,d)		wim_builder_make_event(a,b,c,d)
#endif /* defined(NRC_ROMLIB) */

struct wim_builder {
	uint16_t type;
	uint16_t n_tlvs;
	uint16_t remain_len;
	uint16_t len;
	int vif_id;
	struct nrc_tlv *tlv_map[WIM_TLV_MAX];
	uint8_t data[0];
}__packed;

#define WB_MIN_SIZE			(100)
#define WM_MAX_SIZE			(1024) //(2048) ?

#if defined(NRC_ROMLIB)
struct wim_builder * wim_builder_create_fw(int vif_id, uint16_t type, int tlvs_size);
bool wim_builder_run_wm_fw(struct wim_builder *wb, bool destroy);
void wim_builder_destroy_fw(struct wim_builder *wb);
SYS_BUF * wim_builder_make_hif_fw(SYS_BUF *packet, int hif_len, int vif_id, uint8_t type, uint8_t subtype);
SYS_BUF * wim_builder_make_event_fw(SYS_BUF *packet, int vif_id, uint16_t event_id, int (*cb)(SYS_BUF*));
#else
struct wim_builder * wim_builder_create(int vif_id, uint16_t type, int tlvs_size);
bool wim_builder_run_wm(struct wim_builder *wb, bool destroy);
void wim_builder_destroy(struct wim_builder *wb);
SYS_BUF * wim_builder_make_hif(SYS_BUF *packet, int hif_len, int vif_id, uint8_t type, uint8_t subtype);
SYS_BUF * wim_builder_make_event(SYS_BUF *packet, int vif_id, uint16_t event_id, int (*cb)(SYS_BUF*));
#endif /* defined(NRC_ROMLIB) */
bool wim_builder_append(struct wim_builder *wb, uint16_t id, uint16_t size, void* data);
bool wim_builder_run_wm_wq(struct wim_builder *wb, bool destroy);

#if defined(NRC7291_SDK_DUAL_CM0)||defined(NRC7291_SDK_DUAL_CM3)
void wim_builder_add_hif_header(struct wim_builder *wb);
#endif

#endif //__UMAC_WIM_BUILDER_H__
