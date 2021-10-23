#ifndef UMAC_BEACON_BCMC_H
#define UMAC_BEACON_BCMC_H

#if defined(INCLUDE_BEACON_BCMC)
void umac_beacon_bcmc_init();
void umac_beacon_bcmc_deinit();
bool umac_beacon_bcmc_add(SYS_BUF *head);
bool umac_beacon_has_bcmc();
void umac_beacon_bcmc_tx(uint8_t vif_id, int ac);
void umac_beacon_bcmc_show(uint8_t vif_id);
#else
static inline void umac_beacon_bcmc_init() {};
static inline void umac_beacon_bcmc_deinit() {};
static inline bool umac_beacon_bcmc_add(SYS_BUF *head) {return false;};
static inline bool umac_beacon_has_bcmc() {return false;};
static inline void umac_beacon_bcmc_tx(uint8_t vif_id, int ac) {};
static inline void umac_beacon_bcmc_show() {};
#endif /* defined(INCLUDE_BEACON_BCMC) */

#endif /* UMAC_BEACON_BCMC_H*/
