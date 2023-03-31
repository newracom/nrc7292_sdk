#ifndef __NRC_PS_TYPE_H__
#define __NRC_PS_TYPE_H__

#define PS_SIG_A    0x13579bdf
#define PS_SIG_B    0x2468ace0

enum ps_event {
	PS_EVT_COLDBOOT,
	PS_EVT_BEFORE_DEEPSLEEP,
	PS_EVT_WAKEUP_DEEPSLEEP,
	PS_EVT_BEFORE_WAKEUP,       /* UCODE only */
	PS_EVT_BEFORE_MODEMSLEEP,
	PS_EVT_WAKEUP_MODEMSLEEP,
#if defined(W_DISABLE_GPIO_PIN)
	PS_EVT_CHECK_W_DISABLE,
#endif
	PS_EVT_MAX
};

enum ps_ucode_wake_reason {
	PS_WAKE_NONTIM,
	PS_WAKE_RX_UNICAST,
	PS_WAKE_RX_BROADCAST,
	PS_WAKE_EXT_INT_0,
	PS_WAKE_EXT_INT_1,
	PS_WAKE_EXT_INT_2,
	PS_WAKE_EXT_INT_3,
	PS_WAKE_BROADCAST_FOTA,
	PS_WAKE_BEACON_LOSS,		//8 abnormal
	PS_WAKE_INVALID_RET_INFO,	//9 abnormal
	PS_WAKE_RTC_TIMEOUT,
	PS_WAKE_HSPI,
	PS_WAKE_USR_TIMER, //12 timer set by user
	PS_WAKE_RSN_MAX, //13
};

enum ps_ucode_wake_source {
	PS_WAKE_SRC_DTIM,
	PS_WAKE_SRC_TBTT,
	PS_WAKE_SRC_LI,
	PS_WAKE_SRC_USER_VAL,
};

enum sys_operation {
	SYS_OPER_FW,
	SYS_OPER_UCODE,
	SYS_OPER_MAX
};
#define WKUP_SRC_RTC     (0x00000001L << 0)
#define WKUP_SRC_GPIO    (0x00000001L << 1)
#define WKUP_SRC_HSPI    (0x00000001L << 2)
#define WKUP_SRC_MAX (3)

typedef enum {
	POWER_SAVE_MODEM_SLEEP_MODE,
	POWER_SAVE_DEEP_SLEEP_MODE
} POWER_SAVE_SLEEP_MODE;

typedef enum {
	POWER_SAVE_NON_TIM,
	POWER_SAVE_TIM
} POWER_SAVE_TIM_MODE;

/* function prototype for scheduled callbacks */
typedef void (*scheduled_callback)();
#if defined (INCLUDE_PS_SCHEDULE)
//65B
#define MAX_PS_SCHEDULES 4
/* data structure to support regularly scheduled deep sleep */
/* The data structure supports up to 4 individual callbacks with different durations */
struct ret_schedule_info {
	/* The durations for individual schedules */
	uint32_t duration[MAX_PS_SCHEDULES];
	/* function pointer to callbacks */
	scheduled_callback callback[MAX_PS_SCHEDULES];
	/* time indication when the callback started */
	uint32_t processed_at[MAX_PS_SCHEDULES];
	/* Whether the callback will require network support */
	scheduled_callback gpio_callback;
	uint64_t next_wakeup;
	bool net_init[MAX_PS_SCHEDULES];
	/* next scheduled bitmap */
	uint8_t next_schedule:4;
	/* number of scheduled callbacks configured */
	uint8_t scheduled:3;
	/* Whether the wake up after GPIO interrupt requires network support */
	uint8_t gpio_net_init:1;
} __attribute__ ((packed));
#define RET_SCHEDULE_INFO_SIZE sizeof(struct ret_schedule_info)
#endif /* INCLUDE_PS_SCHEDULE */

#endif /*__NRC_PS_TYPE_H__*/
