/*
 * hostapd / IEEE 802.11n HT
 * Copyright (c) 2002-2009, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2007-2008, Intel Corporation
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "utils/includes.h"

#include "utils/common.h"
#include "utils/eloop.h"
#include "common/ieee802_11_defs.h"
#include "hostapd.h"
#include "ap_config.h"
#include "sta_info.h"
#include "beacon.h"
#include "ieee802_11.h"
#include "hw_features.h"
#include "ap_drv_ops.h"


u8 * hostapd_eid_s1g_capabilities(struct hostapd_data *hapd, u8 *eid)
{
	struct ieee80211_s1g_capabilities *cap;
	u8 *pos = eid;

	if (!hapd->iconf->ieee80211ah || !hapd->iface->current_mode)
		return eid;

	*pos++ = WLAN_EID_S1G_CAP;
	*pos++ = sizeof(*cap);

	cap = (struct ieee80211_s1g_capabilities *) pos;
	os_memset(cap, 0, sizeof(*cap));
	pos += sizeof(*cap);

	return pos;
}


u8 * hostapd_eid_s1g_operation(struct hostapd_data *hapd, u8 *eid)
{
	struct ieee80211_s1g_operation *oper;
	u8 *pos = eid;

	if (!hapd->iconf->ieee80211ah || !hapd->iface->current_mode)
		return eid;

	*pos++ = WLAN_EID_S1G_OPERATION;
	*pos++ = sizeof(*oper);

	oper = (struct ieee80211_s1g_operation *) pos;
	os_memset(oper, 0, sizeof(*oper));
	pos += sizeof(*oper);

#if 0
	oper->chan_width = (hapd->iconf->s1g_prim_chwidth & 0x1) |
	 				   (hapd->iconf->s1g_oper_chwidth & 0x1F) << 1;
#endif
	oper->operating_class = 0xFF;
	oper->primary_chan = 1;
	oper->center_freq = 1;
	//iper->basic_s1g_mcs_and_nss_set

	return pos;
}