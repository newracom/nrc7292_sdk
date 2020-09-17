#ifndef DRIVER_NRC_DEBUG_H
#define DRIVER_NRC_DEBUG_H

#include "system.h"
#include "system_common.h"
#include "nrc-wim-types.h"
#include "driver_nrc.h"

void wpa_driver_debug_frame(uint8_t* frame, uint16_t len);
const char* wpa_driver_alg_str(enum wpa_alg alg);
void wpa_driver_debug_assoc_params(struct wpa_driver_associate_params *params);
void wpa_driver_debug_key(struct nrc_wpa_key *key);

#endif // DRIVER_NRC_DEBUG_H
