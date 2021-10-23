#ifndef UMAC_S1G_SCAN_HANDLER_H
#define UMAC_S1G_SCAN_HANDLER_H

#include "system.h"
#include "umac_ieee80211_types.h"
#include "umac_s1g_hookmanager.h"
#include "umac_s1g_ie_manager.h"
#include "umac_s1g_config.h"

#define BEACON_SIZE	(512)

#define NDP_INDICATION					1
#define NDP_CSSID_PRESENT				0
#define NDP_ANO_PRESENT					1
#define NDP_PROBE_RESPONSE_TYPE_PV1		0
#define NDP_PROBE_RESPONSE_TYPE_PV0		1

///////////////////////
// Handler Functions //
///////////////////////

// STA
bool handle_sta_rx_s1gbeacon(SYS_BUF **buf, int8_t vif_id, bool is_tx);
bool handle_sta_tx_s1gndpprobereq(SYS_BUF **buf, int8_t vif_id, bool is_tx);
bool handle_sta_rx_s1gpv1proberes(SYS_BUF **buf, int8_t vif_id, bool is_tx);
bool handle_sta_tx_probereq(SYS_BUF **buf, int8_t vif_id, bool is_tx);
bool handle_sta_rx_s1gproberes(SYS_BUF **buf, int8_t vif_id, bool is_tx);

// AP
#if defined(INCLUDE_ACCESS_POINT)
bool handle_ap_tx_beacon(SYS_BUF **buf, int8_t vif_id, bool is_tx);
bool handle_ap_rx_s1gprobereq(SYS_BUF **buf, int8_t vif_id, bool is_tx);
bool handle_ap_rx_s1gndpprobereq(SYS_BUF **buf, int8_t vif_id, bool is_tx);
bool handle_ap_tx_proberes(SYS_BUF **buf, int8_t vif_id, bool is_tx);
bool handle_ap_tx_pv1proberes(SYS_BUF **buf, int8_t vif_id, bool is_tx);
#endif

//////////////////////
// Public Functions //
//////////////////////

void umac_s1g_scan_handler_init();
bool umac_s1g_scan_handler_activate(MAC_STA_TYPE type);
bool umac_s1g_scan_handler_deactivate(MAC_STA_TYPE type);

#endif /* UMAC_S1G_SCAN_HANDLER_H */
