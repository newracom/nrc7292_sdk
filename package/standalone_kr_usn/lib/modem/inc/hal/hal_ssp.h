#ifndef __HAL_SSP_NRC7292_H__
#define __HAL_SSP_NRC7292_H__

#include "system.h"

#define SSP_READ_BIT  1
#define SSP_WRITE_BIT 0

enum spi_mode_e {
    SPI_SLAVE = 0,
    SPI_MASTER = 1
};

enum spi_cpol_e {
    CPOL_LO = 0,
    CPOL_HI = 1
};

enum spi_cpha_e {
    CPHA_LO = 0,
    CPHA_HI = 1
};

enum spi_order_e {
    SPI_LSB = 0,
    SPI_MSB = 1
};

bool nrc_ssp_init(uint8_t ch, enum spi_cpol_e cpol, enum spi_cpha_e cpha, enum spi_order_e order, uint8_t bits, uint32_t freq_hz);
void nrc_ssp(uint8_t ch, bool enable);
void nrc_ssp_deinit(uint8_t ch);
uint32_t nrc_ssp_xfer(uint8_t ch, uint8_t *wbuffer, uint8_t *rbuffer, size_t size);
uint32_t nrc_ssp_xfer_custom(uint8_t ch, uint32_t wr, uint32_t addr, uint8_t addr_size, uint32_t *data, size_t data_size_in_bytes);
void nrc_ssp_dma(uint8_t ch, uint32_t id, bool enable);
uint32_t nrc_ssp_int_status(uint8_t ch);
void nrc_ssp_int(uint8_t ch, uint32_t id, bool enable);
void nrc_ssp_int_clr(uint8_t ch, uint32_t id);
void nrc_ssp_lb(uint8_t ch, bool enable);
bool nrc_ssp_rx_filled(uint8_t ch);
bool nrc_ssp_rx_fulled(uint8_t ch);
bool nrc_ssp_tx_fulled(uint8_t ch);
bool nrc_ssp_tx_empty(uint8_t ch);
uint8_t nrc_ssp_rx(uint8_t ch);
void nrc_ssp_tx(uint8_t ch, uint8_t data);
void nrc_ssp_flush(uint8_t ch);
void nrc_ssp_test_xfer();

#endif //__HAL_SSP_NRC7292_H__
