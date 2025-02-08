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

#define OV2640_ADDR 0x60

#ifdef NRC7394
#define OV2640_SPI_MISO 29
#define OV2640_SPI_MOSI 6
#define OV2640_SPI_CS 28
#define OV2640_SPI_SCLK 7
#else
#define OV2640_SPI_MISO 12
#define OV2640_SPI_MOSI 13
#define OV2640_SPI_CS 14
#define OV2640_SPI_SCLK 15
#endif

#define OV2640_SPI_MODE  SPI_MODE0
#define OV2640_SPI_BIT   SPI_BIT8
#define OV2640_SPI_CLOCK 6000000

#ifdef NRC7394
#define OV2640_I2C_SCL          24
#define OV2640_I2C_SDA          18
#else
#define OV2640_I2C_SCL          16
#define OV2640_I2C_SDA          17
#endif
#define OV2640_I2C_CLOCK        100000
#define OV2640_I2C_CLOCK_SOURCE 0
#define OV2640_I2C_DELAY_MS     1

void ov2640_init();
int  ov2640_capture_and_wait(); // Returns the image size.
void ov2640_read_captured_bytes(uint8_t *buf, uint32_t size);
void set_resolution(int index);
