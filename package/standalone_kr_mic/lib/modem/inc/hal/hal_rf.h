#ifndef __NRC_RF_H__
#define __NRC_RF_H__

/*----------------------------------------------------------------*/
/*----------------- Indirect  System Reg define ------------------*/
/*----------------------------------------------------------------*/
#if 0
#define SYSTEM_SSP_CH3_SSPCR0 	0x4000C000
#define SYSTEM_SSP_CH3_SSPCR1 	0x4000C004
#define SYSTEM_SSP_CH3_SSPDR 	0x4000C008
#define SYSTEM_SSP_CH3_SSPCPSR	0x4000C010
#else
#define PMS_SPI_CFG_CONFIG		0x4004E000
#define PMS_SPI_CFG_TXDATA		0x4004E004
#define PMS_SPI_CFG_WRDATA		0x4004E008
#define PMS_SPI_CFG_RDDATA		0x4004E00C
#define PMS_SPI_CFG_STATUS		0x4004E010
#endif
#define PRF_GPIO_DATA 			0x40005040
#define PRF_GPIO_DIR 			0x40005044
#define PRF_GPIO_ALT0 			0x40005048
#define PRF_GPIO_ALT1 			0x4000504C
#define PRF_GPIO_ALT2 			0x40005050
#define PRF_GPIO_PULLUP		0x40005058
/*----------------------------------------------------------------*/

#define PD_SEL_MASK			0x00000001
#define TXRX_MASK			0x00080000

#define SIZE_OF_TX_PWR_TBL      30
#define SIZE_OF_FREQ_DELTA_TBL  30
#define TX_POWER_MIN            1		/* minimum value of tx power input range */
#define TX_POWER_MAX            30		/* maximum value of tx power input range */

#define RegLPF_GAIN (*((volatile uint32_t *)(0x4008A018)))

typedef struct {
	uint8_t valid;
	uint8_t nv_version;
    uint8_t country[2]; /* two ASCII characters */
    /*
     * phy txgain index from 1 to 30.
     * the values of the array were multiplied by 100 to avoid the usage of float type.
     */
    int16_t tx_pwr_tbl[SIZE_OF_TX_PWR_TBL];
    /*
     * Delta power of center frequency table of 1Mhz bw for the configured conuntry code.
     * the values of the array were multiplied by 100 to avoid the usage of float type.
     */
    int16_t freq_delta_tbl[SIZE_OF_FREQ_DELTA_TBL];
} TX_PWR_CAL_PARAM;

void hal_rf_init(void);
void rf_init_dump(void);
void rf_init_dump_ucode(void);
void nrf_reg_init(void);
bool nrf_channel_test(uint32_t channel_freq);
bool nrf_channel(uint32_t channel_freq);
bool nrf_channel_idx(uint8_t ch_index);
bool nrf_channel_fine(uint32_t channel_freq);
void nrf_filter_bw(uint32_t bw);
void nrf_self_trx_cal(void);
void nrf_self_rxdc_cal(void);
void nrf_self_txlo_cal(void);
void nrf_self_tx_cal(uint8_t);
void nrf_spi_write(uint32_t addr, uint32_t wdata);
void nrf_pms_spi_write(uint32_t addr, uint32_t wdata);
void nrf_pms_spi_write_7292(uint32_t addr, uint32_t wdata, uint8_t* result);
uint32_t nrf_channel_freq_find(uint32_t ch_idx);
uint32_t nrf_spi_read(uint32_t addr);
uint32_t nrf_pms_spi_read(uint32_t addr);
uint32_t nrf_pms_spi_read_7292(uint32_t addr, uint8_t* result);
uint8_t nrf_pll_lock_flag(void);
void nrf_loiqcal_loopback(uint32_t cal_mode);
void nrf_cfo_cal(double cfo_ppm, uint32_t *cfo_reg);
void nrf_reg_ram_lut_wr(uint32_t cmd_num);
void nrf_rffe_config(uint8_t rffe);
void nrf_dcoc_config(uint8_t mode);
uint8_t hal_rf_set_txpwr(uint8_t txpwr);
uint8_t hal_rf_get_txpwr(void);
void hal_rf_update_txpwr(void);
uint8_t* hal_rf_get_country(void);
uint8_t hal_get_rf_cal_use(void);

void load_cal_info(void);
void store_cal_info(void);
void clear_cal_info(void);
void update_cal_info_valid(uint8_t valid);
void update_cal_info_nv_version(uint8_t version);
void update_cal_info_tx_pwr_tbl(uint8_t index, int16_t value);
void update_cal_info_freq_pwr_delta_tbl(uint8_t index, int16_t value);
void update_cal_info_country(uint8_t* code);
void show_cal_info(void);

#if defined( NRC7292_LMACTEST_FREERTOS )
void nrf_set_test_mode(int mode);
uint32_t nrf_ssp_read(uint32_t addr, uint8_t* result);
void nrf_ssp_write(uint32_t addr, uint32_t wdata, uint8_t* result);
#endif
#endif// __NRC_RF_H__
