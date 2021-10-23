#ifndef UMAC_TSF_SYNC_H
#define UMAC_TSF_SYNC_H

#include "system.h"
#include "umac_ieee80211_types.h"
#if defined(STANDARD_11AH)
#include "umac_s1g_ie_manager.h"
#endif /* defined(STANDARD_11AH) */

void umac_rx_tsf_sync(int vif_id, GenericMacHeader* mh, LMAC_RXHDR* rx);
void umac_register_tsf_diff(int vif_id, void (*cb)(void* data, int64_t diff), void* data);
void umac_unregister_tsf_diff(int vif_id, void (*cb)(void* data, int64_t diff));
uint32_t umac_rx_tsf_last_beacon();
uint32_t umac_get_next_beacon(int vif_id);
#endif /* UMAC_TSF_SYNC_H */
