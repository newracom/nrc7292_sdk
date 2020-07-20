#ifndef UMAC_PROBE_RESP_H
#define UMAC_PROBE_RESP_H
#if defined(INCLUDE_UMAC_PRESP)
#include "protocol.h"

#define UMAC_NDP_FRAME_PROBE_REQ_TYPE	7

int umac_presp_offl_process(GenericMacHeader *gmh, struct _SYS_BUF *buffer,
                            bool ndp_enable);
void umac_presp_offl_update(uint8_t* frame, uint16_t len);
void umac_presp_start();
void umac_presp_stop();
#else
static inline int umac_presp_offl_process(GenericMacHeader *gmh,
                                    struct _SYS_BUF *buffer, bool ndp_enable)
{
    return 0;
}
static inline void umac_presp_offl_update(uint8_t* frame, uint16_t len) {}
static inline void umac_presp_start() {}
static inline void umac_presp_stop() {}

#endif /* defined(INCLUDE_UMAC_PRESP) */
#endif /* UMAC_PROBE_RESP_h */
