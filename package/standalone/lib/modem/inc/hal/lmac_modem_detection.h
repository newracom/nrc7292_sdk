#ifndef LMAC_MODEM_DETECTION_H
#define LMAC_MODEM_DETECTION_H

#include "system.h"
#include "util_history_buffer.h"
#include "hal_timer.h"


#if defined(INCLUDE_MODEM_RECOVERY)
bool util_modem_rx_add_item(uint8_t vif_id, LMAC_RXHDR* vector, GenericMacHeader* mh, SYS_BUF *buffer);
void util_modem_detection_init();
void util_modem_tx_set_timer(uint64_t duration);
void util_modem_tx_clear_timer();
void util_modem_tx_reset_timer();
void util_modem_do_recovery();
#else
STATIC_INLINE_FUNC(util_modem_rx_add_item, bool, return true,
				uint8_t vif_id, LMAC_RXHDR* vector, GenericMacHeader* mh, SYS_BUF *buffer);
STATIC_INLINE_FUNC(util_modem_detection_init, void, return);
STATIC_INLINE_FUNC(util_modem_tx_set_timer, void, return);
STATIC_INLINE_FUNC(util_modem_tx_clear_timer, void, return);
STATIC_INLINE_FUNC(util_modem_tx_reset_timer, void, return);
STATIC_INLINE_FUNC(util_modem_do_recovery, void, return);
#endif /* defined(INCLUDE_MODEM_RECOVERY) */

#endif /* LMAC_MODEM_DETECTION_H */
