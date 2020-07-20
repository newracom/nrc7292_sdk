#ifndef __HAL_ADC_SFC_NRC7292_H__
#define __HAL_ADC_SFC_NRC7292_H__

#include "system.h"

typedef union {
	struct {
		uint32_t read_mode 					: 2;
		uint32_t rserved1 					: 1;
		uint32_t bus_rdy_ctrl 				: 1;
		uint32_t performance_enhance_mode 	: 1;
		uint32_t eqio_mode_flag 			: 1;
		uint32_t reserved2					: 1;
		uint32_t bus_err_en		 			: 1;
		uint32_t cs_ctrl 					: 1;
		uint32_t reserved 					: 23;
	} bit;
	uint32_t word;
} sfc_mode_t;

typedef union {
	struct {
		uint32_t cs_high_pulse_width		: 4;
		uint32_t reserved 					: 28;
	} bit;
	uint32_t word;
} sfc_baud_t;

void nrc_adc_sfc_init(uint32_t clock_hz, sfc_mode_t mode);
void nrc_adc_sfc_deinit(void);
uint32_t nrc_adc_sfc_read_jedec_id(void);
//void nrc_adc_sfc_write(uint8_t *wbuffer, size_t size);
void nrc_adc_sfc_write(uint32_t data);
uint32_t nrc_adc_sfc_write_mem(uint32_t offset, uint8_t *wbuffer, size_t size);
void nrc_adc_sfc_cmd(uint8_t cmd);
void nrc_adc_sfc_set_status(uint8_t status);
void nrc_adc_sfc_set_status2(uint8_t status);
uint8_t nrc_adc_sfc_read_status(void);
void nrc_adc_sfc_set_mode(sfc_mode_t mode);
void nrc_adc_sfc_get_mode(sfc_mode_t *mode);
uint32_t nrc_adc_sfc_read(uint8_t *rbuffer, size_t size);
uint32_t nrc_adc_sfc_read_mem(uint32_t offset, uint8_t *rbuffer, size_t size);
//uint32_t nrc_adc_sfc_read(void);
void nrc_adc_sfc_erase_sector(uint32_t addr);
void nrc_adc_sfc_erase_block(uint32_t addr);
void nrc_adc_sfc_erase_all();
void nrc_adc_sfc_cs(bool enable);
void nrc_adc_sfc_enable_quad_mode(void);
void nrc_adc_sfc_enable_quad_mode_w25q80ew(void);
void nrc_adc_sfc_set_burst_len(uint8_t len);
void nrc_adc_sfc_cache_ctrl(uint32_t ctrl);
#endif //__HAL_ADC_SFC_NRC7292_H__
