#ifndef __HAL_PWM_NRC7292_H__
#define __HAL_PWM_NRC7292_H__

#include "system.h"

enum {
    PWM0_A = 0,
    PWM0_B,
    PWM0_C,
    PWM0_D,
    PWM1_A,
    PWM1_B,
    PWM1_C,
    PWM1_D,
    PWM_MAX
};

enum {
    PWM_DIV2  = 0,
    PWM_DIV4  = 1,
    PWM_DIV8  = 2,
    PWM_DIV16 = 3,
};

enum {
    PWM_MODE_PHASE		= 1,
    PWM_MODE_REG_OUT	= 2,
    PWM_MODE_REG_OUT2	= 4,
    PWM_MODE_PHASE2		= 9,
};


typedef union {
	struct {
		uint32_t busy_a: 1;
		uint32_t busy_b: 1;
		uint32_t busy_c: 1;
		uint32_t busy_d: 1;
		uint32_t reserved: 28;
	} bit;
	uint32_t word;
} pwm_st_t;


typedef union {
	struct {
		uint32_t en_a: 1;
		uint32_t en_b: 1;
		uint32_t en_c: 1;
		uint32_t en_d: 1;
		uint32_t reserved1: 12;

		uint32_t trig_a: 1;
		uint32_t trig_b: 1;
		uint32_t trig_c: 1;
		uint32_t trig_d: 1;
		uint32_t reserved2: 12;
	} bit;
	uint32_t word;
} pwm_en_t;


typedef union {
	struct {
		uint32_t mode_a: 4;
		uint32_t mode_b: 4;
		uint32_t mode_c: 4;
		uint32_t mode_d: 4;
		uint32_t inv_a:  1;
		uint32_t inv_b:  1;
		uint32_t inv_c:  1;
		uint32_t inv_d:  1;
		uint32_t reserved: 4;
		uint32_t div_a:  2;
		uint32_t div_b:  2;
		uint32_t div_c:  2;
		uint32_t div_d:  2;
	} bit;
	uint32_t word;
} pwm_mode_t;


typedef union {
	struct {
		uint32_t a: 4;
		uint32_t b: 4;
		uint32_t c: 4;
		uint32_t d: 4;
		uint32_t reserved: 16;
	} bit;
	uint32_t word;
} pwm_loop_t;


typedef union {
	struct {
		uint32_t pstn1: 16;
		uint32_t pstn2: 16;
	} bit;
	uint32_t word;
} pwm_xp_t;


typedef union {
	struct {
		uint32_t out: 32;
	} bit;
	uint32_t word;
} pwm_out_reg_t;


typedef struct {
	volatile pwm_st_t st;
	volatile pwm_en_t en;
	volatile pwm_mode_t mode;
	volatile pwm_loop_t loop;

	volatile pwm_xp_t 		t[4][2];
	volatile pwm_out_reg_t 	reg[4][4];
} pwm_controller_t;

void nrc_pwm_init(uint32_t id);
void nrc_pwm_config(uint32_t id, uint32_t div, uint32_t mode, bool inverse);
void nrc_pwm_set(uint32_t id, uint32_t t0, uint32_t t1, uint32_t t2, uint32_t t3);
void nrc_pwm_enable(uint32_t id, bool enable);
bool nrc_pwm_busy(uint32_t id);
void nrc_pwm_show(uint32_t index);
void nrc_pwm_show_blockinfo(uint32_t id);
void nrc_pwm_loop(uint32_t id, uint32_t value);
#endif //__HAL_PWM_NRC7292_H__