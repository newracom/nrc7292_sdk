/*
 * MIT License
 *
 * Copyright (c) 2020 Newracom, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef __NRC_API_SPI_H__
#define __NRC_API_SPI_H__

/** @brief mode number of SPI */
typedef enum {
	SPI_MODE0 = 0,	/**< SPI mode 0	*/
	SPI_MODE1,		/**< SPI mode 1	*/
	SPI_MODE2,		/**< SPI mode 2	*/
	SPI_MODE3,		/**< SPI mode 3	*/
}SPI_MODE;

/** @brief mode number of SPI */
typedef enum {
	SPI_BIT4 = 3,	/**< SPI 4-bit frame */
	SPI_BIT5 = 4,	/**< SPI 5-bit frame */
	SPI_BIT6 = 5,	/**< SPI 6-bit frame */
	SPI_BIT7 = 6,	/**< SPI 7-bit frame */
	SPI_BIT8 = 7,	/**< SPI 8-bit frame */
	SPI_BIT9 = 8,	/**< SPI 9-bit frame */
	SPI_BIT10 = 9,	/**< SPI 10-bit frame */
	SPI_BIT11 = 10,	/**< SPI 11-bit frame */
	SPI_BIT12 = 11,	/**< SPI 12-bit frame */
	SPI_BIT13 = 12,	/**< SPI 13-bit frame */
	SPI_BIT14 = 13,	/**< SPI 14-bit frame */
	SPI_BIT15 = 14,	/**< SPI 15-bit frame */
	SPI_BIT16 = 15,	/**< SPI 16-bit frame */
}SPI_FRAME_BITS;

typedef enum {
	SPI_LSB_ORDER = 0,
	SPI_MSB_ORDER = 1
}SPI_ORDER;

typedef enum {
	SPI_CONTROLLER_SPI0,
	SPI_CONTROLLER_SPI1,
	SPI_CONTROLLER_MAX
}SPI_CONTROLLER_ID;

typedef void (*spi_isr_handler_fn)(int vector);

typedef struct spi_device {
	uint8_t pin_miso; /* SPI MISO pin */
	uint8_t pin_mosi; /* SPI MOSI pin */
	uint8_t pin_cs; /* SPI Chip Select pin */
	uint8_t pin_sclk; /* SPI SCLK pin */
	uint32_t frame_bits; /* SPI frame bits */
	uint32_t clock; /* SPI clock */
	uint8_t mode; /* SPI mode */
	uint8_t controller; /* ID of SPI controller to use */
	uint8_t bit_order; /* LSB(0) or MSB(1) */
	unsigned long irq_save_flag; /*irq save flag */
	spi_isr_handler_fn isr_handler; /* isr handler */
} spi_device_t;


/**********************************************
 * @fn nrc_err_t nrc_spi_master_init(spi_device_t* spi)
 *
 * @brief initialize SPI master with the specified mode and bits
 *
 * @param spi_device_t: A pointer for spi configuration
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_spi_master_init(spi_device_t* spi);


/**********************************************
 * @fn nrc_err_t nrc_spi_init_cs(uint8_t pin_cs)
 *
 * @brief initialize spi chip select pin
 *
 * @param pin_cs: chip select pin number
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_spi_init_cs(uint8_t pin_cs);


/**********************************************
 * @fn nrc_err_t nrc_spi_enable(bool enable)
 *
 * @brief Enable or disable SPI
 *
 * @param spi_device_t: A pointer for spi configuration
 *
 * @param enable: true(enable) or false(disable) SPI
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_spi_enable(spi_device_t* spi, bool enable);

/**********************************************
 * @fn  void nrc_spi_start_xfer(void)
 *
 * @brief Enable CS to continuously transfer data
 *
 * @param spi_device_t: A pointer for spi configuration
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_spi_start_xfer(spi_device_t* spi);

/**********************************************
 * @fn  void nrc_spi_stop_xfer(void)
 *
 * @brief Disable CS to continuously transfer data
 *
 * @param spi_device_t: A pointer for spi configuration
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_spi_stop_xfer(spi_device_t* spi);

/**********************************************
 * @fn  void nrc_spi_dma_start_xfer(spi_device_t* spi)
 *
 * @brief Enable CS without disabling interrupt
 *
 * @param spi_device_t: A pointer for spi configuration
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_spi_dma_start_xfer(spi_device_t* spi);

/**********************************************
 * @fn  void nrc_spi_dma_stop_xfer(spi_device_t* spi)
 *
 * @brief Disable CS without disabling interrupt
 *
 * @param spi_device_t: A pointer for spi configuration
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_spi_dma_stop_xfer(spi_device_t* spi);

/**********************************************
 * @fn nrc_err_t_t nrc_spi_xfer(uint8_t *wbuffer, uint8_t *rbuffer, uint32_t size)
 *
 * @brief Transfer the data between master and slave
 *         Should run inside nrc_spi_start_xfer() and nrc_spi_stop_xfer()
 *         User can call nrc_spi_xfer multiple times to transmit data
 *
 * @param spi_device_t: A pointer for spi configuration
 *
 * @param wbuffer: write buffer pointer
 *
 * @param rbuffer: read buffer pointer
 *
 * @param size: number of bytes to transfer
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_spi_xfer(spi_device_t* spi, uint8_t *wbuffer, uint8_t *rbuffer, uint32_t size);


/**********************************************
 * @fn nrc_err_t nrc_spi_writebyte_value(uint8_t addr, uint8_t data)
 *
 * @brief Write one-byte data to the specified register address
 *
 * @param spi_device_t: A pointer for spi configuration
 *
 * @param addr: register address to write data
 *
 * @param data: data to write
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_spi_writebyte_value(spi_device_t* spi, uint8_t addr, uint8_t data);

/**********************************************
 * @fn nrc_err_t nrc_spi_readbyte_value(uint8_t addr, uint8_t *data)
 *
 * @brief Read one-byte data from register address
 *
 * @param spi_device_t: A pointer for spi configuration
 *
 * @param addr: register address to read data
 *
 * @param data: A pointer to read data
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_spi_readbyte_value(spi_device_t* spi, uint8_t addr, uint8_t *data);

/**********************************************
 * @fn  nrc_err_t nrc_spi_write_values(uint8_t addr, uint8_t *data, int size)
 *
 * @brief Write bytes data to the specified register address
 *
 * @param spi_device_t: A pointer for spi configuration
 *
 * @param addr: register address to write data
 *
 * @param data: A pointer data to write
 *
 * @param size: write size(Btyes)
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_spi_write_values(spi_device_t* spi, uint8_t addr, uint8_t *data, int size);

/**********************************************
 * @fn  nrc_err_t nrc_spi_read_values(uint8_t addr, uint8_t *data, int size)
 *
 * @brief Read bytes data from register address
 *
 * @param spi_device_t: A pointer for spi configuration
 *
 * @param addr: register address to read data
 *
 * @param data: A pointer to read data
 *
 * @param size: read size(Btyes)
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_spi_read_values(spi_device_t* spi, uint8_t addr, uint8_t *data, int size);

#endif /* __NRC_API_SPI_H__ */
