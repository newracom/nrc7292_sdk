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
enum spi_mode {
	SPI_MODE0 = 0,	/**< SPI mode 0	*/
	SPI_MODE1,		/**< SPI mode 1	*/
	SPI_MODE2,		/**< SPI mode 2	*/
	SPI_MODE3,		/**< SPI mode 3	*/
};

/** @brief mode number of SPI */
enum spi_frame_bits {
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
};

/**********************************************
 * @fn void nrc_spi_init(enum spi_mode mode, enum spi_frame_bits bits, uint32_t clock)
 *
 * @brief initialize SPI with the specified mode and bits
 *
 * @param mode: SPI mode
 *
 * @param bits: SPI frame bits
 *
 * @param clock: SPI clock frequency
 *
 * @return N/A
 ***********************************************/
void nrc_spi_init(enum spi_mode mode, enum spi_frame_bits bits, uint32_t clock);


/**********************************************
 * @fn void nrc_spi_enable(bool enable)
 *
 * @brief Enable or disable SPI
 *
 * @param enable: true(enable) or false(disable) SPI
 *
 * @return N/A
 ***********************************************/
void nrc_spi_enable(bool enable);


/**********************************************
 * @fn uint32_t nrc_spi_xfer(uint8_t *wbuffer, uint8_t *rbuffer, uint32_t size)
 *
 * @brief Transfer the data between master and slave
 *
 * @param wbuffer: write buffer pointer
 *
 * @param rbuffer: read buffer pointer
 *
 * @param size: number of bytes to transfer
 *
 * @return Number of transferred bytes
 ***********************************************/
uint32_t nrc_spi_xfer(uint8_t *wbuffer, uint8_t *rbuffer, uint32_t size);


/**********************************************
 * @fn void nrc_spi_writebyte(uint8_t reg, uint8_t data)
 *
 * @brief Write one-byte data to the specified register
 *
 * @param reg: register to write
 *
 * @param data: data to write
 *
 * @return N/A
 ***********************************************/
void nrc_spi_writebyte(uint8_t reg, uint8_t data);


/**********************************************
 * @fn uint8_t nrc_spi_readbyte(uint8_t reg)
 *
 * @brief Read one-byte data from register
 *
 * @param reg: register to read
 *
 * @return Read data
 ***********************************************/
uint8_t nrc_spi_readbyte(uint8_t reg);

#endif /* __NRC_API_SPI_H__ */