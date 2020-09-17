#ifndef __NRC_RF_H__
#define __NRC_RF_H__

void hal_rf_init();
void arf_reg_init();
bool arf_channel(uint32_t channel_freq);
void arf_filter_bw(uint32_t bw);
void arf_self_trx_cal();
void arf_self_rxdc_cal();
void arf_self_txlo_cal();
void arf_spi_write(uint32_t addr, uint32_t wdata);
uint32_t arf_channel_freq_find(uint32_t ch_idx);


uint32_t arf_spi_read(uint32_t addr);
uint32_t arf_pll_lock_flag();
void arf_loiqcal_loopback(uint32_t cal_mode);
void arf_reg_ram_lut_wr(uint32_t cmd_num);

///////////////////////////////////////////////////////
/////       PCS RF related function               /////
///////////////////////////////////////////////////////

void prf_spi_write(uint32_t addr, uint32_t wdata);
uint32_t prf_spi_read(uint32_t addr);
void rf_spi_mod_write(uint32_t addr, uint32_t bit_end, uint32_t bit_start, uint32_t value);
uint32_t rf_spi_bit_read(uint32_t addr, uint32_t bit_end, uint32_t bit_start);
uint32_t bit_mask(uint32_t bit_end, uint32_t bit_start);

void initialize_prf();
void prf_reg_init();
void prf_pll_cal(uint32_t channel_freq);
void prf_lpf_cal();
void prf_dcoc_cal();
void prf_filter_bw(uint32_t bw);
void prf_auxadc();
void prf_txlo_cal();
void prf_txgain(uint32_t gain);
void prf_auxpll_cal();
void prf_reg_ram_lut_wr(uint32_t cmd_num);

void prf_spi_test();
void prf_read_regall();





#endif// __NRC_RF_H__


