#ifndef __HAL_NADC_NRC7292_H__
#define __HAL_NADC_NRC7292_H__

#include "system.h"

enum {
    ADC_CH_0 = 0,
    ADC_CH_1,
    ADC_CH_2,
    ADC_CH_3,
};

typedef union {
	struct {
		uint32_t sw_en 		: 1;
		uint32_t sel		: 2;
		uint32_t reserved   : 29;
	} bit;
	uint32_t word;
} adc_ctrl_t;


typedef union {
	struct {
		uint32_t ch0 			: 9;
		uint32_t ch0_valid		: 1;
		uint32_t reserved1   	: 6;
		uint32_t ch1 			: 9;
		uint32_t ch1_valid		: 1;
		uint32_t reserved2   	: 6;
	} bit;
	uint32_t word;
} adc_data_t;


typedef struct {
	volatile adc_ctrl_t 						ctrl;
	volatile adc_data_t 						data01;
	volatile adc_data_t 						data23;
} adc_controller_t;

void nrc_nadc_init(void);
void nrc_nadc_deinit(void);

void nrc_nadc_sel(uint32_t id);
void nrc_nadc_enable(bool enable);

bool nrc_nadc_ch_valid(uint32_t id);
uint16_t nrc_nadc_data(uint32_t id);
uint16_t nrc_nadc_valid_with_data(uint32_t id);
bool nrc_nadc_valid(uint16_t value);
void nrc_nadc_show(void);
void nrc_nadc_test(void);

#define ADC_VALID(x) ((x & BIT9)? true: false)
#define ADC_VALUE(x) (x & 0x1FF)

#endif //__HAL_NADC_NRC7292_H__
