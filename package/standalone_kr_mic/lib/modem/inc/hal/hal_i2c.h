#ifndef __HAL_I2C_NRC7292_H__
#define __HAL_I2C_NRC7292_H__

#include "system.h"

enum {
    I2C_0 = 0,
    I2C_1,
    I2C_2,
    I2C_3,
    I2C_MAX
};

enum {
    I2C_8BIT = 0,
    I2C_16BIT,
};

enum {
    I2C_CLK_CONTRLLER = 0,
    I2C_PCLK
};

void nrc_i2c_config(uint32_t id, uint32_t clk_sel, uint32_t freq_hz, uint32_t width);
void nrc_i2c_timming(uint32_t id, uint32_t fc, uint32_t rc);
void nrc_i2c_enable_ch(uint32_t id, bool enable);
void nrc_i2c_int_enable(uint32_t id, bool enable);
void nrc_i2c_tx(uint32_t id, uint32_t value);
uint32_t nrc_i2c_rx(uint32_t id);
void nrc_i2c_iack(uint32_t id);
void nrc_i2c_cmd_ack(uint32_t id, bool ack);
void nrc_i2c_cmd_write(uint32_t id);
void nrc_i2c_cmd_read(uint32_t id);
void nrc_i2c_cmd_stop(uint32_t id);
void nrc_i2c_cmd_start(uint32_t id);
bool nrc_i2c_status_iflag(uint32_t id);
bool nrc_i2c_status_tip(uint32_t id);
bool nrc_i2c_status_al(uint32_t id);
bool nrc_i2c_status_busy(uint32_t id);
bool nrc_i2c_status_rxack(uint32_t id);
void nrc_i2c_reset_ch(uint32_t id);
bool nrc_i2c_status_int(uint32_t id);
void nrc_i2c_wait_progress(uint32_t id);
bool nrc_i2c_write_byte(uint32_t id, uint8_t value);
bool nrc_i2c_read_byte(uint32_t id, uint8_t *value, bool ack);
void nrc_i2c_show(uint32_t id);
void nrc_i2c_test();

#endif //__HAL_I2C_NRC7292_H__
