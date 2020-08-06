#ifndef UMAC_S1G_TIM_HANDLER_H
#define UMAC_S1G_TIM_HANDLER_H
#if defined(INCLUDE_S1G_HOOK)
#include "system.h"
#include "umac_ieee80211_types.h"
#include "umac_s1g_config.h"


#define N_P			(1)		// TODO: [7292] currently, the maximum number of sta is 1024 in our system.
#define N_B			(32)
#define N_SB		(8)
#define N_BITMAP	(8)

typedef struct {
	uint8_t	tim_bits;
} s1g_tim_subblock;

typedef union {
	s1g_tim_subblock subblocks[N_SB];
	uint8_t	bytes[N_SB];
} s1g_tim_block;

typedef union {
	s1g_tim_block blocks[N_B];
	uint8_t bytes[(N_B * N_SB)];
} s1g_tim_page;

typedef union {
	s1g_tim_page pages[N_P];
	uint8_t bytes[(N_P * N_B * N_SB)];
} s1g_tim;

typedef struct {
	uint8_t		eid;
	uint8_t		length;
	uint8_t		dtim_count;
	uint8_t		dtim_period;
} s1g_tim_ie_basic;

typedef struct {
	uint8_t		traffic_indicator	: 1;
	uint8_t		page_slice_number	: 5;
	uint8_t		page_index			: 2;
} s1g_tim_bitmap_control;

typedef struct {
	uint8_t		encoding_mode		: 2;
	uint8_t		inverse_bitmap		: 1;
	uint8_t		block_offset		: 5;
} s1g_block_control_offset;

typedef union {
	uint8_t		block_bitmap;
	struct {
		uint8_t		single_aid 		: 6;
		uint8_t		rsv				: 2;
	};
	uint8_t		olb_length;
	struct {
		uint8_t		ewl				: 3;
		uint8_t		ade_length		: 5;
	};
} s1g_block_encoded_block_info;

///////////////////////////////
// Convert Handler Functions //
///////////////////////////////

///bool umac_s1g_convert_ie_tim_legacy_to_s1g(struct _SYS_BUF *buf, uint16_t offset, uint8_t eid);
void umac_s1g_tim_update_aid(uint16_t aid, bool flag);
void umac_s1g_convert_ie_tim_legacy_to_s1g_beacon();

bool umac_s1g_convert_ie_tim_s1g_to_legacy(struct _SYS_BUF *buf, int8_t vif_id, ie_general *_ie, bool is_tx, bool ap_sta);

//////////////////////
// Public Functions //
//////////////////////

void umac_s1g_tim_handler_init();
bool umac_s1g_tim_handler_reset(MAC_STA_TYPE type);

uint16_t umac_s1g_get_ie_s1g_tim_length();
uint8_t* umac_s1g_get_ie_s1g_tim();

void clear_legacy_tim();
uint16_t umac_s1g_get_ie_legacy_tim_length();
uint8_t* umac_s1g_get_ie_legacy_tim();
#else
static inline void umac_s1g_tim_update_aid(uint16_t aid, bool flag) {}
static inline void umac_s1g_convert_ie_tim_legacy_to_s1g_beacon() {}
static inline bool umac_s1g_convert_ie_tim_s1g_to_legacy(struct _SYS_BUF *buf,
												int8_t vif_id, ie_general *_ie,
												bool is_tx, bool ap_sta)
{
	return true;
}
#endif /* defined(INCLUDE_S1G_HOOK) */
#endif /* UMAC_S1G_TIM_HANDLER_H */