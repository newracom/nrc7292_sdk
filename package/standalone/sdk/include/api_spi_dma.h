/*
 * MIT License
 *
 * Copyright (c) 2024 Newracom, Inc.
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


#ifndef __API_SPI_DMA_H__
#define __API_SPI_DMA_H__

#include "api_spi.h"

/*****************************************************************************/
/**********************************************
 * @fn spi_dma_init(spi_device_t *spi_dma)
 *
 * @brief Initialize SPI using DMA.
 *        spi_device_t should be initialized for SPI configuration
 *        and supplied when calling spi_dma_init.
 *
 * @param spi_dma : user initialized SPI configuration.
 *
 * @return 0 if successful, -1 otherwise.
 ***********************************************/
int spi_dma_init(spi_device_t *spi_dma);

/**********************************************
 * @fn spi_dma_write(uint8_t *data, uint32_t size)
 *
 * @brief Write data to SPI using DMA. SPI related data should be prepared by user.
 *
 * @param data : to be transferred to SPI device.
 * @param size : size of data.
 *
 * @return NONE
 ***********************************************/
void spi_dma_write(uint8_t *data, uint32_t size);

/**********************************************
 * @fn spi_dma_read(uint8_t *addr, uint8_t *data, uint32_t size)
 *
 * @brief Read data from SPI interface using DMA for given address
 *        and save the data to user supplied buffer.
 *
 * @param addr : SPI peripheral address.
 * @param data : buffer to transferred data to be saved.
 * @param size : size for how much data to be read.
 *
 * @return NONE
 ***********************************************/
void spi_dma_read(uint8_t *addr, uint8_t *data, uint32_t size);


/*****************************************************************************/
#endif /* #ifndef __API_SPI_DMA_H__ */

