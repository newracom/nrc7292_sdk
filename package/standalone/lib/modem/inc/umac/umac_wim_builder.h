#ifndef __UMAC_WIM_BUILDER_H__
#define __UMAC_WIM_BUILDER_H__

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

struct wim_builder * wim_builder_create(int vif_id, uint16_t type, int tlvs_size);
bool wim_builder_append(struct wim_builder *wb, uint16_t id, void* data, uint16_t size);
bool wim_builder_append_u8(struct wim_builder *wb, uint16_t id, uint8_t data);
bool wim_builder_append_u16(struct wim_builder *wb, uint16_t id, uint16_t data);
bool wim_builder_append_u32(struct wim_builder *wb, uint16_t id, uint32_t data);
bool wim_builder_append_bool(struct wim_builder *wb, uint16_t id, bool data);
bool wim_builder_run_wm(struct wim_builder *wb, bool destroy);
bool wim_builder_run_wm_wq(struct wim_builder *wb, bool destroy);
void wim_builder_destroy(struct wim_builder *wb);
SYS_BUF * wim_builder_make_hif(SYS_BUF *packet, int hif_len, int vif_id, uint8_t type, uint8_t subtype);
SYS_BUF * wim_builder_make_event(SYS_BUF *packet, int vif_id, uint16_t event_id, int (*cb)(SYS_BUF*));
#if defined(NRC7291_SDK_DUAL_CM0)||defined(NRC7291_SDK_DUAL_CM3)
void wim_builder_add_hif_header(struct wim_builder *wb);
#endif

#endif //__UMAC_WIM_BUILDER_H__
