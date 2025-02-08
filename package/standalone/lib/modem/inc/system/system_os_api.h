#ifndef	SYSTEM_OS_API_H
#define SYSTEM_OS_API_H

#include "system.h"
#include "system_macro.h"
#include "system_freertos.h"

typedef enum {
	WAIT_EVENT_PM1_ACK_SUCCESS      = BIT(0),
	WAIT_EVENT_PM1_ACK_FAILURE      = BIT(1),
	WAIT_EVENT_EAPOL_M4_ACK_SUCCESS = BIT(2),
	WAIT_EVENT_EAPOL_M4_ACK_FAILURE = BIT(3),
#if ( ConfigUSE_16_BIT_TICKS == 1 )
	WAIT_EVENT_MAX                  = BIT(8)
#else
	WAIT_EVENT_MAX                  = BIT(24)
#endif
} WAIT_EVENT;

typedef enum {
	NOTIFY_EVENT_CONNECTED,
	NOTIFY_EVENT_DISCONNECTED,
	NOTIFY_EVENT_PM1_ACK_SUCCESS,
	NOTIFY_EVENT_PM1_ACK_FAILURE,
	NOTIFY_EVENT_EAPOL_M4_ACK_SUCCESS,
	NOTIFY_EVENT_EAPOL_M4_ACK_FAILURE,
	NOTIFY_EVENT_MAX
} NOTIFY_EVENT;

int system_event_init( void );
uint32_t system_event_wait( uint32_t ulEventMask, uint32_t ulTimeoutMs );
int system_event_notify( NOTIFY_EVENT ulEvent );

#if !(defined(BOOT_LOADER) || defined(UCODE))
typedef int (*system_cv_callback_t)(void *cookie );

typedef struct system_cv_handle {
	SemaphoreHandle_t sem;
	//uint32_t value;
	system_cv_callback_t callback;
	void *cookie;
} system_cv_handle_t;

void system_cv_init (system_cv_handle_t *handle, system_cv_callback_t callback, void *cookie);
int system_cv_wait (system_cv_handle_t *handle);
void system_cv_signal (system_cv_handle_t *handle);
#endif

#endif //SYSTEM_OS_API_H
