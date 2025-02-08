#ifndef UMAC_S1G_HOOKMANAGER_H
#define UMAC_S1G_HOOKMANAGER_H

#include "system.h"
#include "umac_ieee80211_types.h"
#include "umac_s1g_framehook.h"
#include "umac_s1g_elementhook.h"
#include "umac_info.h"

#if ! defined(INCLUDE_IBSS)
#define MAX_HOOKS		18
#define MAX_ELEM_HOOKS	18
#else
#define MAX_HOOKS		20
#define MAX_ELEM_HOOKS	20
#endif

typedef struct {

	bool enabled;
	bool is_tx;
	bool has_no_ehook;
	int ie_offset;

	framehook fhook;
	elementhook ehooks[MAX_ELEM_HOOKS];

} hook;

//////////////////////
// Public Functions //
//////////////////////

void umac_s1g_hookmanager_init();

void umac_s1g_clear_hook(int i);
int	umac_s1g_get_hid(bool ul);

int umac_s1g_get_frame_ie_offset(int i);
int umac_s1g_get_ie_offset(struct _SYS_BUF *buf, bool is_tx, int frame_offset, bool s1gbeacon);
int umac_s1g_get_is_tx(int i);
int umac_s1g_get_is_enabled(int i);

void umac_s1g_pv1_register_framehook(int i, uint8_t type, uint8_t subtype, FRAME_HANDLER fp);
void umac_s1g_pv1_deregister_framehook(int i);

void umac_s1g_register_framehook(int i, uint8_t type, uint8_t subtype, FRAME_HANDLER fp);
void umac_s1g_deregister_framehook(int i);

void umac_s1g_register_elementhook(int i, uint8_t eid, ELEMENT_CHECKER fp, bool ap_sta);
void umac_s1g_deregister_elementhook(int i, uint8_t eid);
void umac_s1g_register_no_elementhook(int i, uint8_t eid, ELEMENT_CHECKER fp, bool ap_sta);

bool umac_s1g_process_ul(struct _SYS_BUF **buf);
bool umac_s1g_process_dl(struct _SYS_BUF **buf);

#endif /* UMAC_S1G_HOOKMANAGER_H */