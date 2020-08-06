#ifndef UMAC_S1G_FRAMEHOOK_H
#define UMAC_S1G_FRAMEHOOK_H

#include "system_type.h"

typedef bool (*FRAME_HANDLER)(struct _SYS_BUF **buf, int8_t vif_id, bool uplink);

typedef struct {
	uint8_t type;
	uint8_t subtype;
	FRAME_HANDLER handler;
} framehook;

//////////////////////
// Public Functions //
//////////////////////

void umac_s1g_framehook_init(framehook *fh);
void umac_s1g_set_framehook(framehook *fh, uint8_t type, uint8_t subtype, FRAME_HANDLER fp);
bool umac_s1g_invoke_framehook(framehook *fh, struct _SYS_BUF **buf, int8_t vif_id, bool uplink);

#endif /* UMAC_S1G_FRAMEHOOK_H */