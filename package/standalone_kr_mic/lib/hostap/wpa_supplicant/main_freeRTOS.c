/*
 * WPA Supplicant / Example program entrypoint
 * Copyright (c) 2003-2005, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"
#include "common.h"
#include "wpa_supplicant_i.h"
#include "FreeRTOS.h"
#include "system_common.h"
#include "lmac_common.h"
#include "driver_nrc.h"
#include "wpa_debug.h"

void wpas_task_main(void *pvParams)
{
	struct wpa_interface ifaces[NRC_WPA_NUM_INTERFACES];
	struct wpa_params params;
	struct wpa_global *global;
	int i = 0;
	memset(&params, 0, sizeof(params));
	params.wpa_debug_level = wpa_debug_level;

	global = wpa_supplicant_init(&params);

	if (global == NULL) {
		system_printf("Failed to init wpa_supplicant \n");
		return;
	}

	for (i = 0; i < NRC_WPA_NUM_INTERFACES; i++) {
		char *name = os_zalloc(os_strlen("wlanX"));
		os_memcpy(name, "wlanX", os_strlen("wlanX") + 1);
		name[4] = i + '0';
		memset(&ifaces[i], 0, sizeof(struct wpa_interface));
		ifaces[i].confname = ifaces[i].ifname = name;
		if (!wpa_supplicant_add_iface(global, &ifaces[i], NULL))
			system_printf("Failed to add iface(0)\n");
	}

#if !defined(NRC7291_SDK_DUAL_CM3) && !defined(NRC7292_SDK_DUAL_CM3)
		lmac_start();
#endif

	wpa_supplicant_run(global);
	wpa_supplicant_deinit(global);

	for (i = 0; i < NRC_WPA_NUM_INTERFACES; i++)
		os_free((void *) ifaces[i].ifname);
}
