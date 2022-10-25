#ifndef DRV_CSPI_NEWRACOM_H
#define DRV_CSPI_NEWRACOM_H

#include "system.h"

bool drv_cspi_init();
void drv_cspi_deinit();
void drv_cspi_que_config(int que, bool bit_msb, bool byte_msb, bool header_on);
void drv_cspi_int_config(int que, bool active_hi, bool edge, bool enable);
void drv_cspi(int que, bool enable);
void drv_cspi_mode(int cpol, int cpha);
void drv_cspi_reset(int que);
bool drv_cspi_set(int que, uint32_t address, uint16_t size, bool interrupt, bool event);
uint32_t drv_cspi_status(int que);
void drv_cspi_int_clr(int que);
uint32_t drv_cspi_que_total(int que);
uint32_t drv_cspi_que_count(int que);
bool drv_cspi_que_error(int que);


#endif //DRV_CSPI_NEWRACOM_H
