#ifndef __NRC_PHY_H__
#define __NRC_PHY_H__

////////////////////
void init_constructor();
void print_cli_title();
////////////////////
void hal_phy_init();
void hal_phy_post_init();
void phy_top_init();
void phy_dfe_init();
void phy_dbe_init();
void phy_dfe_init_ap();
void mdelay(uint32_t delay);

void phy_set_cfo2sfo_factor(uint32_t channel_freq, uint32_t op_mode);
void phy_arf_txgain_control(uint32_t arf_txgain);
void phy_arf_rxgain_control(uint32_t arf_rxgain);
void phy_arf_rxgain_fixed(uint32_t fixed_rxgain);
void phy_arf_rxgain_fixed_off();
#ifndef	RELEASE
void phy_state_print();
void phy_dfe_status_counter();
#endif
void phy_dfe_status_counter_rst();
void phy_gain_adap_on();
void phy_gain_adap_off();

void phy_op_bw(uint32_t bw);
void phy_field_test();
void phy_loopback_enable();
void phy_loopback_disable();
void phy_set_primary_1m_loc(uint8_t loc);
void phy_set_tx_primary_1m_loc(uint8_t loc);
void phy_set_rx_primary_1m_loc(uint8_t loc);
void phy_enable();
void phy_disable();
void phy_rfcal_enable();
void phy_rfcal_disable();
void phy_romgen_enable(uint32_t format, uint32_t count);
void phy_romgen_disable();
void phy_adc_iq_swap();
void phy_dac_iq_swap();
void phy_adc_iq_noswap();
void phy_dac_iq_noswap();
void phy_adc_twos();
void phy_dac_twos();
void phy_adc_offsetbin();
void phy_dac_offsetbin();
void phy_configure_txfifo_af_lvl(uint32_t txfifo_af_level);
void phy_configure_bssid(uint32_t bssid_0, uint32_t bssid_1, uint32_t bssid_2, uint32_t bssid_3, uint32_t bssid_4);
void phy_ignore_bssid_check();
void phy_ignore_ul_ind_bit_check();
void phy_enable_ul_ind_bit_check();
void phy_ignore_rsrvd_bit_check();
void phy_enable_rsrvd_bit_check();
void phy_init_rtl_func_sim();
void phy_agcout_nic(uint32_t n_tgt_power);
int phy_fftout_fd_nic();
void phy_fftout_fd_nic_show();
void swap_for_sort(int arr[][2], int first_idx, int second_idx);
int partition_for_sort(int arr[][2], int left, int right);
void quick_sort(int arr[][2], int left, int right);
void phy_rfloiq_cal(uint32_t cal_mode, uint32_t cal_range, uint32_t cal_step, uint32_t cal_search);
void phy_loiqcal_settings(uint32_t cal_mode);
void phy_rfloiq_calparam_search(uint32_t cal_mode, uint32_t cal_range, uint32_t cal_step, uint32_t cal_search);
void phy_arf_rxgain_lut_write(uint32_t ram_type);
void phy_arf_txgain_lut_write(uint32_t ram_type);
void phy_prf_rxgain_lut_write(uint32_t ram_type);
void phy_nrf_txgain_lut_write(uint32_t ram_type);
void phy_nrf_rxgain_lut_write(uint32_t ram_type);
void phy_nrf_txgain_control(uint32_t nrf_txgain);
void phy_nrf_rxgain_control(uint32_t nrf_rxgain);
void phy_nrf_rxgain_fixed(uint32_t fixed_rxgain);
void phy_nrf_rxgain_fixed_off();
void phy_inter_power_scan(uint8_t range, uint8_t log);
#endif //__NRC_PHY_H__


