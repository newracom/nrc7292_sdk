#ifndef __DRV_RTC_H__
#define __DRV_RTC_H__

#include "system.h"

enum {
    RTC_DUR_1024 = 0,
    RTC_DUR_2048,
    RTC_DUR_4096,
    RTC_DUR_8192,
};


enum {
    RTC_FREERUN_MODE = 0,
    RTC_BOMB_MODE,
};

typedef union {
	struct {
		uint32_t start 		: 1;
		uint32_t stop		: 1;
		uint32_t reserved1	: 1;
		uint32_t mode		: 1;
		uint32_t int_en		: 1;
		uint32_t load_freerun : 1;
		uint32_t reserved   : 26;
	} bit;
	uint32_t word;
} rtc_ctrl_t;

typedef union {
	struct {
		uint32_t value: 32;
	} bit;
	uint32_t word;
} rtc_cnt_t;

typedef union {
	struct {
		uint32_t reserved   : 5;
		uint32_t value		: 27;
	} bit;
	uint32_t word;
} rtc_offset_compensation_t;

typedef union {
	struct {
		uint32_t start   		: 1;
		uint32_t reserved1		: 2;
		uint32_t int_en			: 1;
		uint32_t reserved2		: 12;
		uint32_t duration		: 16;
	} bit;
	uint32_t word;
} rtc_cal_freq_offset_estimator_ctrl_t;

typedef union {
	struct {
		uint32_t status_clear	: 1;		///<< write clear, read status
		uint32_t busy			: 1;		
		uint32_t reserved   	: 30;
	} bit;
	uint32_t word;
} rtc_int_t;

typedef union {
	struct {
		uint32_t value   		: 16;
		uint32_t reserved		: 16;
	} bit;
	uint32_t word;
} rtc_cal_tracking_increment;

typedef struct {
	volatile rtc_ctrl_t 						ctrl;
	volatile rtc_cnt_t 							set_cnt_h;
	volatile rtc_cnt_t 							set_cnt_l;
	volatile rtc_cnt_t 							set_curr_h;
	volatile rtc_cnt_t 							set_curr_l;
	volatile uint32_t 							reserved1;
	volatile uint32_t							cnt_reload_h;
	volatile uint32_t							cnt_reload_l;
	volatile uint32_t							reserved[4];
	volatile rtc_int_t							intr;
} rtc_controller_t;

typedef struct {
	volatile rtc_cal_freq_offset_estimator_ctrl_t 	offset_estimator_ctrl;
	volatile rtc_int_t 							intr;
	volatile uint32_t 							ref_cnt;
	volatile uint32_t 							residual;
	volatile uint32_t 							ref_clk_cnt;

} rtc_cal_controller_t;

typedef union {
	struct {
		uint32_t en 			: 1;
		uint32_t duration 		: 2;
		uint32_t reserved1   	: 12;
		uint32_t fst_en			: 1;
		uint32_t reserved2   	: 16;
	} bit;
	uint32_t word;
} lpo_ctrl_t;


typedef union {
	struct {
		uint32_t value 			: 13;
		uint32_t reserved   	: 19;
	} bit;
	uint32_t word;
} lpo_cmp_t;


typedef union {
	struct {
		uint32_t man 			: 13;
		uint32_t man_en			: 1;
		uint32_t reserved   	: 18;
	} bit;
	uint32_t word;
} lpo_ctrl1_t;


typedef struct {
	volatile lpo_ctrl_t 					ctrl;
	volatile lpo_cmp_t 						cmp;
	volatile lpo_ctrl1_t 					ctrl1;
} lpo_controller_t;

void drv_rtc_init(uint8_t mode);
void drv_rtc_deinit(void);

void drv_rtc_set_int_clr();
void drv_rtc_set_start(bool start);
bool drv_rtc_get_start();
void drv_rtc_set_stop();
void drv_rtc_set_mode(int mode);
int drv_rtc_get_mode();
void drv_rtc_clear_count(uint8_t mode);

void drv_rtc_set_int(uint64_t after_ms);
void drv_rtc_set_int_en(bool enable);
void drv_rtc_set_cnt(uint64_t value);
void drv_rtc_get_cnt(uint64_t *value);
uint64_t drv_rtc_get_ms(void);
void drv_rtc_set_ofst_comp(uint32_t value);
void drv_rtc_set_freq_ofst_est_done_clr();
bool drv_rtc_get_freq_ofst_est_done();

void drv_rtc_set_freq_ofst_est_dur(uint32_t dur);
void drv_rtc_set_freq_ofst_est_mode(uint32_t mode);
void drv_rtc_set_freq_ofst_est_start();

void drv_rtc_freq_ofst_est_int_clr();
bool drv_rtc_get_freq_ofst_est_busy();
uint32_t drv_rtc_get_residual();
uint8_t drv_rtc_get_lpo_cal_wait_duration();
uint32_t drv_rtc_get_freq_ofst_in_ppm();
uint32_t drv_rtc_get_freq_ofst_in_onbd();
uint32_t drv_rtc_get_ofst_comp();

void drv_rtc_set_cal_int_en(bool enable);
uint32_t drv_rtc_set_residual();

void drv_rtc_set_tracking_increment(uint32_t value);
void drv_rtc_set_freq_ofst_in_ppm(uint32_t value);
void drv_rtc_set_freq_ofst_in_onbd(uint32_t value);
void drv_rtc_set_measured_refclk_cnt(uint32_t value);
void drv_rtc_show();
void drv_rtc_test();

bool drv_rtc_get_lpo_cal_enable();
void drv_rtc_set_lpo_cal_enable(bool enable);

void drv_rtc_set_lpo_cal_wait_duration(uint8_t wait_dur);
uint8_t drv_rtc_get_lpo_cal_wait_duration();
void drv_rtc_int_en(bool enable);
void drv_rtc_int_clr();

#endif //__DRV_RTC_H__
