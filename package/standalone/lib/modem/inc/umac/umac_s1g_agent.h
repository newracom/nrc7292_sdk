#ifndef UMAC_S1G_AGENT_H
#define UMAC_S1G_AGENT_H

#include "system.h"
#include "umac_ieee80211_types.h"

#define SHORT_KERNEL_VERSION(a,b) (((a) << 8) + (b))

#if defined(INCLUDE_S1G_HOOK)
struct s1g_statistics {
	uint32_t	cnt_framelen_mismatch;
	uint32_t	cnt_elementhook_fail;
	uint32_t	cnt_txbuffer_shortage;
	uint32_t	cnt_rxbuffer_shortage;
	uint32_t	cnt_transform_fail;
};

typedef enum {
	AP_BCN,
	AP_SCAN,
	AP_JOIN,
	AP_RSV,
	STA_BCN,
	STA_SCAN,
	STA_JOIN,
	STA_RSV
} S1GHOOK;

/* 
 * 8bit variable
 * [3-0] : AP - bcn / scan / join / resv 
 * [7-4] : STA - bcn / scan / join / resv
 */

#define SET_S1GHOOK_MODE(variable, index, value)		\
	do {												\
		variable &= ~(0x1 << index);	 				\
		variable |= (value & 0x1) << index; 			\
	} while(0);

#define GET_S1GHOOK_MODE(variable, index)				\
	(variable & (0x1 << index)) >> index

//////////////////////
// Public Functions //
//////////////////////

void		umac_s1g_agent_init();
bool		umac_s1g_agent_activate(int8_t vif_id, MAC_STA_TYPE type);
void		umac_s1g_agent_reset();
bool		umac_s1g_agent_convert_legacy_to_s1g(struct _SYS_BUF **buf);
bool		umac_s1g_agent_convert_s1g_to_legacy(struct _SYS_BUF **buf);
int8_t		umac_s1g_agent_mp_vif_id();

#else
static inline void umac_s1g_agent_init() {}
static inline void umac_s1g_agent_reset() {}
static inline bool umac_s1g_agent_activate(int8_t vif_id, MAC_STA_TYPE type)
{
	return false;
}
static inline bool umac_s1g_agent_convert_legacy_to_s1g(struct _SYS_BUF **buf)
{
	return true;
}
static inline bool umac_s1g_agent_convert_s1g_to_legacy(struct _SYS_BUF **b)
{
	return true;
}
#endif /* defined(INCLUDE_S1G_HOOK) */
#endif /* UMAC_S1G_AGENT_H */
