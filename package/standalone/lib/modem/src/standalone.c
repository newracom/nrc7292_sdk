#include "system_common.h"
#include "lmac_common.h"
#include "lmac_ps_common.h"
#include "driver_nrc_ps.h"
#include "lmac_util.h"
#include "standalone.h"

#include <string.h>
#include "nrc_lwip.h"
#include "global_const.h"

#if defined(SUPPORT_NVS_FLASH)
#include "nvs_flash.h"
#endif
#include "system_modem_api.h"
#include "util_version.h"

#define DECLARE_TASK(NAME, STACK_SIZE)									\
	struct NAME##_Task {												\
		StackType_t 	stack[STACK_SIZE / sizeof(StackType_t)];		\
		StaticTask_t	tcb;											\
		TaskHandle_t	handle; 										\
	};																	\
	struct NAME##_Task s_##NAME##_task;

#if defined(INCLUDE_MEASURE_AIRTIME) || !defined(INCLUDE_TRACK_WAKEUP)
#define CREATE_TASK(NAME, PRIO, FN)										\
	s_##NAME##_task.handle = xTaskCreateStatic(							\
		FN,#NAME, sizeof(s_##NAME##_task.stack) / sizeof(StackType_t),	\
		NULL, PRIO, &s_##NAME##_task.stack[0],							\
		&s_##NAME##_task.tcb);
#else
#define CREATE_TASK(NAME, PRIO, FN)										\
	s_##NAME##_task.handle = xTaskCreateStatic(							\
		FN,#NAME, sizeof(s_##NAME##_task.stack) / sizeof(StackType_t),	\
		NULL, PRIO, &s_##NAME##_task.stack[0],							\
		&s_##NAME##_task.tcb);											\
	if (s_##NAME##_task.handle)											\
		A(STRING_TASK_CREATED, #NAME);									\
	else																\
		A(STRING_FAILED_TO_CREATE_TASK, #NAME);
#endif /* defined(INCLUDE_MEASURE_AIRTIME) */

static const char* STRING_TASK_CREATED				= "%s task is created \n";
static const char* STRING_FAILED_TO_CREATE_TASK 	= "Failed to create %s task \n";

DECLARE_TASK(wpa_supplicant, WPA_SUPPLICANT_STACK_SIZE);

#include "nrc_user_app.h"

#if defined(INCLUDE_BCAST_FOTA_STA_SUPPORT)
#include "util_bcast_fota.h"
#endif /* defined(INCLUDE_BCAST_FOTA_STA_SUPPORT) */
extern void wpas_task_main(void *pvParams);
extern void net80211_newracom_preinit();

static const char *module_name()
{
	return "wpa: ";
}

#if defined (INCLUDE_PS_SCHEDULE)
static void ps_callback_task(void *arg)
{
	system_modem_api_run_retention_schedule();
}
#endif /* INCLUDE_PS_SCHEDULE */

int standalone_main()
{
	bool net_init = true;
	bool ps_callback = false;
	initVersion();

#if defined(SUPPORT_NVS_FLASH)
	nvs_handle_t tmp_nvs_handle;
	nvs_err_t err = NVS_OK;
	/* initialize Non-volatile storage key/value subsystem */
	/* Application will need to call nvs_open to utilize NVS. */
	/* There's no need to call nvs_flash_deinit() */
	/* since the system will never return. */
	I(TT_SYS, "Initializing NVS...\n");
	err = nvs_flash_init();
	if (err == NVS_ERR_NVS_NO_FREE_PAGES || err == NVS_ERR_NVS_NEW_VERSION_FOUND) {
		if ((err = nvs_flash_erase()) == NVS_OK) {
			err = nvs_flash_init();
		}
	} else {
		/* make sure that there's a default namespace set by opening with NVS_READWRITE, */
		/* so that subsequent nvs_open with NVS_READONLY for default name space succeeds. */
		I(TT_SYS, "Try opening NVS with read/write for default namespace (%s)...\n", NVS_DEFAULT_NAMESPACE);
		if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &tmp_nvs_handle) == NVS_OK) {
			I(TT_SYS, "NVS Open successful, continue...\n");
			nvs_close(tmp_nvs_handle);
		} else {
			E(TT_SYS, "Fatal error, nvs_open\n");
		}
	}
#endif

#if defined (INCLUDE_PS_SCHEDULE)
	system_modem_api_ps_check_network_init(&net_init, &ps_callback);
#endif /* INCLUDE_PS_SCHEDULE */

	if (net_init) {
		//get_standalone_macaddr(g_standalone_addr);
		CREATE_TASK(wpa_supplicant, NRC_TASK_PRIORITY, wpas_task_main);
		wifi_lwip_init();
	}

#if defined (INCLUDE_PS_SCHEDULE)
#if defined(INCLUDE_TRACE_WAKEUP)
	A("[%s] ps_callback = %d\n", __func__, ps_callback);
#endif /* INCLUDE_TRACE_WAKEUP */
	/* set the priority of ps_callback_task lower than other system tasks by */
	/* setting priority to NRC_TASK_PRIORITY - 1. */
	if (ps_callback) {
		extern void nrc_wifi_init (void);
		if (net_init) {
			A("[%s] nrc_wifi_init = %d\n", __func__, net_init);
			nrc_wifi_init();
		}
		TaskHandle_t callback_task_handle;
		xTaskCreate(ps_callback_task,
					"PS Schedule callback Task",
					2048,
					NULL,
					NRC_TASK_PRIORITY - 1,
					&callback_task_handle);
		return 0;
	} else {
		/* To be safe, check if the RTC timeout occured, but failed to find */
		/* the scheduled callback, then let it sleep again with new scheduled time */
		system_modem_api_ps_schedule_resume_for_timeout();
	}
#endif /* INCLUDE_PS_SCHEDULE */

	nrc_user_app_main();

	return 0;
}

#ifdef MONITOR
void monitor_task_main(void *pvParameters)
{
	extern void monitor_rtc_print();

	for ( ;; ) {
		/* Print out the name of this task. */
		monitor_rtc_print();
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
#endif

//////////////////////////////////////////////////////////////////////////
/// FreeRTOS helper functions (Timer, Dynamic allocation, ...)
//////////////////////////////////////////////////////////////////////////

void vApplicationMallocFailedHook( void )
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	   free memory available in the FreeRTOS heap.	pvPortMalloc() is called
	   internally by FreeRTOS API functions that create tasks, queues, software
	   timers, and semaphores.	The size of the FreeRTOS heap is set by the
	   configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	for( ;; );
}

static uint8_t hex2num(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

void get_standalone_macaddr(int vif_id, uint8_t *mac) {
	uint8_t *tmp = mac;
#if defined(MAC_ADDR_STANDALONE) && defined(MAC_ADDR_SEED)
#error "Consider one method to assign MAC address."
#endif

	/* Step 1. check mac address in serial flash for vif_id.
	 * Step 2. if vif_id's mac address doesn't exist in serial flash,
	 * then check the other vif_id's mac address.
	 * Step 3. both mac addresses are not written in serial flash,
	 * then use FW generated mac address.
	 * */
	if (lmac_check_sf_macaddr(mac, vif_id)) {
		I(TT_WPAS, "%s%d MAC address: "MACSTR"\n",
			module_name(), vif_id, MAC2STR(mac));
		return;
	} else if (lmac_check_sf_macaddr(mac, vif_id?0:1)) {
		if (!(mac[0]&BIT1)) {
			mac[0] |= BIT1;
			I(TT_WPAS, "%s%d MAC address: "MACSTR"\n",
				module_name(), vif_id, MAC2STR(mac));
			return;
		}
	}

#if defined(MAC_ADDR_STANDALONE)
	int i = 0;
	char sz_mac[18] = {0};
	strncpy(sz_mac, (const char*) MAC_ADDR_STANDALONE, (size_t)18);
	int idx = 0, szIdx = 0;

	for (i = 0; i < 6; ++i) {
		mac[i] += hex2num(sz_mac[szIdx++]) * 0x10;
		mac[i] += hex2num(sz_mac[szIdx++]);
		szIdx++;
	}
#endif

#if defined(MAC_ADDR_SEED)
	unsigned long s = 1212;
#if defined(CONFIG_AP)
	s = 1234;
#endif
	char sz_mac[6] = {0};
	int i = 0;
	int sz_mac_copy_len  = (sizeof(MAC_ADDR_SEED) < 6) ? sizeof(MAC_ADDR_SEED) : 6;
	memcpy(sz_mac, (const char*) MAC_ADDR_SEED, sz_mac_copy_len);
	for (i = 1, *mac++ = 0x20; i < sizeof(sz_mac); i++) {
		s = (s << 5) + s + sz_mac[i];
		*mac++ = (char) s;
	}
#endif
	tmp[4] += vif_id;
}

void set_standalone_ipaddr(int vif_id, uint32_t ipaddr, uint32_t netmask, uint32_t gwaddr)
{
	lmac_ps_set_ip_addr(ipaddr, netmask, gwaddr);
}

int set_standalone_hook_dhcp(int vif_id)
{
	WPA_DRIVER_PS_HOOK(WPA_PS_HOOK_TYPE_DHCP, &vif_id, NULL, 0);

	return -1;
}

int set_standalone_hook_static(int vif_id)
{
	WPA_DRIVER_PS_HOOK(WPA_PS_HOOK_TYPE_STATIC, &vif_id, NULL, 0);
	return -1;
}
