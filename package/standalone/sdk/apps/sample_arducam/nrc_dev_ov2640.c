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

#include "nrc_sdk.h"
#include "ov2640_regs.h"
#include "nrc_dev_ov2640.h"
#include "api_dma.h"
#include "api_spi_dma.h"

#define OV2640_USE_DMA 1

spi_device_t ov2640_spi;
i2c_device_t ov2640_i2c;

static void ov2640_init_spi()
{
	ov2640_spi.pin_miso = OV2640_SPI_MISO;
	ov2640_spi.pin_mosi = OV2640_SPI_MOSI;
	ov2640_spi.pin_cs   = OV2640_SPI_CS;
	ov2640_spi.pin_sclk = OV2640_SPI_SCLK;
	ov2640_spi.frame_bits = OV2640_SPI_BIT;
	ov2640_spi.clock = OV2640_SPI_CLOCK;
	ov2640_spi.mode = OV2640_SPI_MODE;
	ov2640_spi.controller = SPI_CONTROLLER_SPI0;
	ov2640_spi.irq_save_flag = 0;
	ov2640_spi.isr_handler = NULL;

#if OV2640_USE_DMA
	spi_dma_init(&ov2640_spi);
#else
	nrc_spi_master_init(&ov2640_spi);
	nrc_spi_enable(&ov2640_spi, true);
	_delay_ms(100);
#endif
}

static void ov2640_read_spi(uint8_t reg, uint8_t *value)
{
#if OV2640_USE_DMA
	uint8_t buf[2] = {0, 0};
	/* read 2 bytes to take care of reg size */
	spi_dma_read(&reg, buf, 2);
	*value = buf[1];
#else
	nrc_spi_readbyte_value(&ov2640_spi, reg, value);
#endif
}

static void ov2640_write_spi(uint8_t reg, uint8_t value)
{
#if OV2640_USE_DMA
	uint8_t spi_buf[2] = {reg | 0x80, value};
	spi_dma_write(spi_buf, 2);
#else
	reg |= 0x80;
	nrc_spi_writebyte_value(&ov2640_spi, reg, value);
#endif
}

static void ov2640_init_i2c()
{
	ov2640_i2c.pin_sda      = OV2640_I2C_SDA;
	ov2640_i2c.pin_scl      = OV2640_I2C_SCL;
	ov2640_i2c.clock_source = OV2640_I2C_CLOCK_SOURCE;
	ov2640_i2c.controller   = I2C_MASTER_0;
	ov2640_i2c.clock        = OV2640_I2C_CLOCK;
	ov2640_i2c.width        = I2C_8BIT;
	ov2640_i2c.address      = OV2640_ADDR;

	nrc_i2c_init(&ov2640_i2c);
	nrc_i2c_enable(&ov2640_i2c, true);
}

static void ov2640_read_i2c(uint8_t sad, uint8_t reg, uint8_t *value)
{
	nrc_i2c_start(&ov2640_i2c);
	nrc_i2c_writebyte(&ov2640_i2c, sad);
	nrc_i2c_writebyte(&ov2640_i2c, reg);
	nrc_i2c_stop(&ov2640_i2c);
	_delay_ms(OV2640_I2C_DELAY_MS);

	nrc_i2c_start(&ov2640_i2c);
	nrc_i2c_writebyte(&ov2640_i2c, sad | 0x1);
	nrc_i2c_readbyte (&ov2640_i2c, value, false);
	nrc_i2c_stop(&ov2640_i2c);
	_delay_ms(OV2640_I2C_DELAY_MS);
}

static void ov2640_write_i2c(uint8_t sad, uint8_t reg, uint8_t value)
{
	nrc_i2c_start(&ov2640_i2c);
	nrc_i2c_writebyte(&ov2640_i2c, sad);
	nrc_i2c_writebyte(&ov2640_i2c, reg);
	nrc_i2c_writebyte(&ov2640_i2c, value);
	nrc_i2c_stop(&ov2640_i2c);
	_delay_ms(OV2640_I2C_DELAY_MS);
}

static void ov2640_write_i2c_multi(uint8_t sad, const struct sensor_reg *reg_vals)
{
	struct sensor_reg reg_val;
	uint8_t reg, val;
	while(1)
	{
		reg_val = *reg_vals++;
		if(reg_val.reg == 0xFF && reg_val.val == 0xFF)
			return;
		ov2640_write_i2c(OV2640_ADDR, reg_val.reg, reg_val.val);
	}
}

void set_resolution(int index)
{
	//Uncomment the line below to induce JPEG image corruption!
	//nrc_usr_print("[SET RESOLUTION: %d]\n", index);
	if(index < 0 || 8 < index)
	{
		nrc_usr_print("[ERROR: INVALID RESOLUTION]\n");
		return;
	}
	switch(index)
	{
		case 0: nrc_usr_print("160x120_JPEG\n");   break;
		case 1: nrc_usr_print("176x144_JPEG\n");   break;
		case 2: nrc_usr_print("320x240_JPEG\n");   break;
		case 3: nrc_usr_print("352x288_JPEG\n");   break;
		case 4: nrc_usr_print("640x480_JPEG\n");   break;
		case 5: nrc_usr_print("800x600_JPEG\n");   break;
		case 6: nrc_usr_print("1024x768_JPEG\n");  break;
		case 7: nrc_usr_print("1280x1024_JPEG\n"); break;
		case 8: nrc_usr_print("1600x1200_JPEG\n"); break;
	}
	ov2640_write_i2c_multi(OV2640_ADDR, resolutions[index]);
}

void ov2640_init()
{
	nrc_usr_print("<OV2640 INIT>\n");

	//SPI & I2C INIT
	nrc_usr_print("[SPI/I2C INIT]\n");
	ov2640_init_spi();
	ov2640_init_i2c();

	//SPI: TEST REGISTER WRITE/READ TEST 0x55
	nrc_usr_print("[SPI TEST]\n");
	while(1)
	{
		uint8_t wspi_test_value = 0x55, rspi_test_value;
		ov2640_write_spi(ARDUCHIP_TEST1, wspi_test_value);
		ov2640_read_spi(ARDUCHIP_TEST1, &rspi_test_value);
		if(wspi_test_value == rspi_test_value)
			break;
		_delay_ms(1000);
	}
	nrc_usr_print("=> SUCCESS\n");

	//SPI: MCU MODE
	nrc_usr_print("[MCU MODE SELECTION]\n");
	ov2640_write_spi(ARDUCHIP_MODE, 0x00);

	//I2C: ID READ TEST
	nrc_usr_print("[I2C IDENTIFICATION]\n");
	/* bank sensor */
	ov2640_write_i2c(OV2640_ADDR, 0xff, 0x01);
	uint8_t id_msb, id_lsb;
	ov2640_read_i2c(OV2640_ADDR, 0x0A, &id_msb);
	ov2640_read_i2c(OV2640_ADDR, 0x0B, &id_lsb);
	uint16_t id = id_msb << 8 | id_lsb;
	nrc_usr_print("ID(I2C)=0x%x\n", id);
	if(id != 0x2642)
	{
		nrc_usr_print("Error: ID mismatch (expected: 0x2642)\n");
		return;
	}
	nrc_usr_print("=> SUCCESS\n");

	//I2C: Init CAM
	nrc_usr_print("[CAMERA INITIALIZATION]\n");
	ov2640_write_i2c(OV2640_ADDR, 0xff, 0x01);
	ov2640_write_i2c(OV2640_ADDR, 0x12, 0x80);
	_delay_ms(100);
	ov2640_write_i2c_multi(OV2640_ADDR, OV2640_JPEG_INIT);
	ov2640_write_i2c_multi(OV2640_ADDR, OV2640_YUV422);
	ov2640_write_i2c_multi(OV2640_ADDR, OV2640_JPEG);
	ov2640_write_i2c(OV2640_ADDR, 0xff, 0x01);
	ov2640_write_i2c(OV2640_ADDR, 0x15, 0x00);

	//ov2640_write_i2c_multi(OV2640_ADDR, OV2640_320x240_JPEG);
	//ov2640_write_i2c_multi(OV2640_ADDR, OV2640_640x480_JPEG);
	//ov2640_write_i2c_multi(OV2640_ADDR, OV2640_800x600_JPEG);
	ov2640_write_i2c_multi(OV2640_ADDR, OV2640_1600x1200_JPEG);
	_delay_ms(1000);
}

// Returns the image size.
int ov2640_capture_and_wait()
{
	ov2640_write_spi(ARDUCHIP_FIFO, FIFO_CLEAR_MASK);
	ov2640_write_spi(ARDUCHIP_FIFO, FIFO_START_MASK);
	uint8_t trigger_value;
	do {ov2640_read_spi(ARDUCHIP_TRIG, &trigger_value);} while(!(trigger_value & CAP_DONE_MASK));
	uint8_t fifo_size1, fifo_size2, fifo_size3;
	ov2640_read_spi(0x42, &fifo_size1);
	ov2640_read_spi(0x43, &fifo_size2);
	ov2640_read_spi(0x44, &fifo_size3);
	return (fifo_size1 | (fifo_size2 << 8) | (fifo_size3 << 16)) & 0x07fffff;
}

#define CHUNK_SIZE (2048 + 1)
void ov2640_read_captured_bytes(uint8_t *buf, uint32_t size)
{
#if OV2640_USE_DMA
	int iter = size / (CHUNK_SIZE - 1);
	int remainder = size % (CHUNK_SIZE - 1);
	uint8_t addr = BURST_FIFO_READ;
	uint8_t chunk[CHUNK_SIZE];
	int i = 0;

	if (iter > 0) {
		for (i = 0; i < iter; i++) {
			spi_dma_read(&addr, chunk, CHUNK_SIZE);
			/* remove 1 dummy byte */
			memcpy(buf + (i * (CHUNK_SIZE - 1)), chunk + 1, CHUNK_SIZE - 1);
		}
	}

	if (remainder) {
		spi_dma_read(&addr, chunk, remainder);
		/* remove 1 dummy byte */
		memcpy(buf + (i * (CHUNK_SIZE - 1)), chunk + 1, remainder);
	}
#else
	for(int i = 0; i < size; i++)
		ov2640_read_spi(SINGLE_FIFO_READ, buf++);
#endif
}
