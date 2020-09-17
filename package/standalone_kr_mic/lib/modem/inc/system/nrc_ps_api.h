#ifndef __NRC_PS_API_H__
#define __NRC_PS_API_H__

#include <stdbool.h>

#define PS_SIG_A    0x13579bdf
#define PS_SIG_B    0x2468ace0

enum ps_event {
	PS_EVT_COLDBOOT,
	PS_EVT_BEFORE_DEEPSLEEP,
	PS_EVT_WAKEUP_DEEPSLEEP,
	PS_EVT_BEFORE_WAKEUP,       /* UCODE only */
	PS_EVT_BEFORE_MODEMSLEEP,
	PS_EVT_WAKEUP_MODEMSLEEP,
	PS_EVT_MAX
};
enum sys_operation {
	SYS_OPER_FW,
	SYS_OPER_UCODE,
	SYS_OPER_MAX
};

#define WKUP_SRC_RTC BIT0
#define WKUP_SRC_GPIO BIT1
#define WKUP_SRC_HSPI BIT2
#define WKUP_SRC_MAX (3)

struct retention_info {
	uint8_t		mac_addr[6];
	uint16_t	aid;
	uint16_t	ch_freq;
	uint32_t	rf_reg_dump[92];
	uint8_t		ch_bw;
	int8_t		ch_offset;
	uint8_t		primary_loc;
	uint32_t	sig_a;
	uint32_t	sig_b;

	/*
	 * wakeup_gpio_pin
	 *  - [7, Reserved]
	 *  - [6, Enable debounce function for switch]
	 *  - [5, Reserved]
	 *  - [4:0, GPIO Number]
	 */
#define WKUP_PIN_DEBOUNCE 		(6)
#define WKUP_PIN_NUMBER_MASK 	(BIT4 | BIT3 | BIT2 | BIT1 | BIT0)
	uint8_t 	wakeup_gpio_pin;

	/*
	 * wakeup_source
	 *  - [3:7, reservedn]
	 *  - [2, RTC]
	 *  - [1, GPIO]
	 *  - [0, RTC]
	 */
	uint8_t 	wakeup_source;

	/*
	 * power_indication_pin
	 *  - [7, Reserved]
	 *  - [6, Enable power_indication_pin]
	 *  - [5, Reserved]
	 *  - [4:0, GPIO Number]
	 */
#define PWR_PIN_ENABLE			(6)
#define PWR_PIN_NUMBER_MASK 	(BIT4 | BIT3 | BIT2 | BIT1 | BIT0)
	uint8_t 	power_indication_pin;
	uint8_t		rf_efuse_revision;
	uint8_t		reserved[503];
	uint8_t 	userspace_area[64];
	uint8_t     reserved2[64];
} __attribute__ ((packed));

typedef void (*nrc_ps_cb_t)(enum ps_event event,
							enum sys_operation operation);

struct nrc_ps_ops {
	nrc_ps_cb_t cb;
};

struct retention_info* nrc_ps_get_retention_info();
struct nrc_ps_ops *nrc_ps_get_vendor(void);
struct nrc_ps_ops *nrc_ps_get(void);

#define PS_EVENT_CB(e,o) do { \
	struct nrc_ps_ops *ops = nrc_ps_get();\
	nrc_ps_cb_t cb = ops->cb;\
	if (ops && cb) \
		cb(e, o);\
} while(0)

#define PS_FILL_SIG() do { \
    struct retention_info* retent;\
    retent = nrc_ps_get_retention_info();\
	retent->sig_a 	= PS_SIG_A; \
	retent->sig_b 	= PS_SIG_B; \
} while(0)

#define PS_CLEAR_SIG() do { \
    struct retention_info* retent;\
    retent = nrc_ps_get_retention_info();\
    retent->sig_a = 0;\
    retent->sig_b = 0; \
} while(0)

static inline bool PS_CHECK_SIG() {
	struct retention_info* retent;
	retent = nrc_ps_get_retention_info();
	if (retent->sig_a == PS_SIG_A
		&& retent->sig_b == PS_SIG_B)
		return true;
	return false;
};

static inline uint8_t *nrc_ps_get_user_retention(size_t *size) {
	struct retention_info* retent;
	retent = nrc_ps_get_retention_info();
	*size = sizeof(retent->userspace_area);
	return retent->userspace_area;
}

void nrc_ps_gpio_init(enum sys_operation operation);
void nrc_ps_gpio_set_deepsleep(int brd_rev);
void nrc_ps_gpio_pre_config();
void nrc_ps_gpio_config();

/* ============================================================================== */
/* =================== USER INTERFACE =========================================== */
/* ============================================================================== */
int nrc_ps_config_wakeup_pin(bool check_debounce, int pin_number);
int nrc_ps_get_wakeup_pin(bool *check_debounce, int *pin_number);
int nrc_ps_config_power_indication_pin(bool enable, int pin_number);
int nrc_ps_get_power_indication_pin(bool *enable, int *pin_number);
int nrc_ps_config_wakeup_source(uint8_t wakeup_source);
int nrc_ps_get_wakeup_source(uint8_t *wakeup_source);
#endif /*__NRC_PS_API_H__*/