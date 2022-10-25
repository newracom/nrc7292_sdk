#ifndef UMAC_S1G_JOIN_HANDLER_H
#define UMAC_S1G_JOIN_HANDLER_H

#include "system.h"
#include "umac_ieee80211_types.h"
#include "umac_s1g_hookmanager.h"
#include "umac_s1g_ie_manager.h"

///////////////////////
// Handler Functions //
///////////////////////

// STA
bool handle_sta_tx_assocreq(struct _SYS_BUF **buf, int8_t vif_id, bool is_tx);
bool handle_sta_tx_reassocreq(struct _SYS_BUF **buf, int8_t vif_id, bool is_tx);
bool handle_sta_rx_s1gassocres(struct _SYS_BUF **buf, int8_t vif_id, bool is_tx);

#if defined(INCLUDE_ACCESS_POINT)
// AP
bool handle_ap_rx_s1gassocreq(struct _SYS_BUF **buf, int8_t vif_id, bool is_tx);
bool handle_ap_rx_s1greassocreq(struct _SYS_BUF **buf, int8_t vif_id, bool is_tx);
bool handle_ap_tx_assocres(struct _SYS_BUF **buf, int8_t vif_id, bool is_tx);
#endif

//////////////////////
// Public Functions //
//////////////////////

void umac_s1g_join_handler_init();
bool umac_s1g_join_handler_activate(MAC_STA_TYPE type);
bool umac_s1g_join_handler_deactivate(MAC_STA_TYPE type);

#endif /* UMAC_S1G_JOIN_HANDLER_H */