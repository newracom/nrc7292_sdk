#ifndef UMAC_S1G_ELEMENTHOOK_H
#define UMAC_S1G_ELEMENTHOOK_H

#include "umac_ieee80211_types.h"

typedef bool (*ELEMENT_CHECKER)(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);

typedef struct {

	uint8_t eid;
	ELEMENT_CHECKER checker;
	bool enabled;
	bool ap_sta;

} elementhook;

//////////////////////
// Public Functions //
//////////////////////

void umac_s1g_elementhook_init(elementhook *eh);
void umac_s1g_set_elementhook(elementhook *eh, uint8_t eid, ELEMENT_CHECKER ec, bool ap_sta);
bool umac_s1g_has_elementhook(elementhook *eh, uint8_t eid);
bool umac_s1g_invoke_elementhook(elementhook *eh, struct _SYS_BUF *buf, ie_general *ie,
															int8_t vif_id, bool is_tx);
#endif /* UMAC_S1G_ELEMENTHOOK_H */