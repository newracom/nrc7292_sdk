#ifndef UMAC_CHANNEL_H
#define UMAC_CHANNEL_H
#include "nrc-wim-types.h"

#if defined(INCLUDE_NEW_CHANNEL_CTX)
bool umac_set_country(uint8_t *cc);
bool umac_set_channel(uint16_t freq, enum ch_width band);
bool umac_set_channel2(struct wim_s1g_channel_param *param);
#else
bool umac_set_country(uint8_t *cc) {return false;}
bool umac_set_channel(uint16_t freq, enum ch_width band) {return false;}
bool umac_set_channel2(struct wim_s1g_channel_param *param){return false;}
#endif /* INCLUDE_NEW_CHANNEL_CTX */
#endif /* UMAC_CHANNEL_H */