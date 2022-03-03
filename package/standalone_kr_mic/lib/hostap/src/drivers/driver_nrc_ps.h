#ifndef DRIVER_NRC_PS_H
#define DRIVER_NRC_PS_H

typedef enum {
	WPA_PS_HOOK_TYPE_SCAN = 0,
	WPA_PS_HOOK_TYPE_INIT,
	WPA_PS_HOOK_TYPE_PMK,
	WPA_PS_HOOK_TYPE_SET_KEY,
	WPA_PS_HOOK_TYPE_AUTH,
	WPA_PS_HOOK_TYPE_ASSOC,	
	WPA_PS_HOOK_TYPE_PORT,
	WPA_PS_HOOK_TYPE_DHCP,
	WPA_PS_HOOK_TYPE_STATIC,
	WPA_PS_HOOK_TYPE_MAX,
} DRV_PS_HOOK_TYPE;

typedef enum {
	WPA_PS_HOOK_RET_SUCCESS = 0,
	WPA_PS_HOOK_RET_FAIL = -1,
	WPA_PS_HOOK_RET_FAIL_NO_RETENT = -2,
	WPA_PS_HOOK_RET_FAIL_NOT_RECOVERED = -3,
	WPA_PS_HOOK_RET_FAIL_NO_AP_INFO = -4,
	WPA_PS_HOOK_RET_FAIL_MEM_ALLOC = -5,
	WPA_PS_HOOK_RET_FAIL_NO_IP = -6,
	WPA_PS_HOOK_RET_FAIL_NO_HANDLER = -7,
} DRV_PS_HOOK_RET_TYPE;

int wpa_driver_ps_hook_handle(DRV_PS_HOOK_TYPE type, void *param1, void *param2);

#if defined (INCLUDE_WPS_PS_HOOK) && defined (INCLUDE_UCODE)
#define WPA_DRIVER_PS_HOOK(type, param1, param2, ret)										\
	do {																				\
		if(wpa_driver_ps_hook_handle(type, param1, param2) == WPA_PS_HOOK_RET_SUCCESS)	 \
			return ret;																	\
	} while(0)
int8_t wpa_driver_ps_get_recovered();
#else
#define WPA_DRIVER_PS_HOOK(type, param1, param2, ret)
static inline int8_t wpa_driver_ps_get_recovered(){return -1;};
#endif
#endif // DRIVER_NRC_PS_H

