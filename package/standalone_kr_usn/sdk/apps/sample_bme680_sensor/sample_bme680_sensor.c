/*
 * MIT License
 *
 * Copyright (c) 2022 Newracom, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "bme680.h"

//#define BME680_SPI
#define BME680_I2C

struct bme680_dev sensor;
struct bme680_field_data data;

static uint16_t bme680_meas_period;
static int bme680_sequence_number = 0;

#if defined(BME680_SPI)
static int8_t spi_read(uint8_t dev_id, uint8_t reg, uint8_t *data, uint16_t len)
{
	unsigned short i = 0;

	while (len--) {
		if (nrc_spi_readbyte_value(reg + i, data + i) != NRC_SUCCESS)
			return -1;
		++i;
	}
	return BME680_OK;
}

static int8_t spi_write(uint8_t dev_id, uint8_t reg, uint8_t *data, uint16_t len)
{
	unsigned short i = 0;

	while (len--) {
		nrc_spi_writebyte_value(reg, *(data + i));
		++i;
		reg = *(data + i);
		++i;
	}
	return BME680_OK;
}

static int spi_init(void)
{
	int count = 0;
	uint8_t data;

	nrc_spi_init(SPI_MODE3, SPI_BIT8, 1000000);
	nrc_spi_enable(true);
	_delay_ms(100);

	while (1)
	{
		nrc_spi_writebyte_value(0x73, 0x00);

		if (nrc_spi_readbyte_value(0x73|0x80, &data) != NRC_SUCCESS)
			return -1;

		if (data == 0)	{
			nrc_usr_print("[%s] spi_mem_page changed successfully!!\n", __func__);
			break;
		} else {
			_delay_ms(300);
		}

		if(count++ == 10){
			nrc_usr_print("[%s] spi_mem_page change failed. retry......\n", __func__);
			return -1;
		}
	}

	if (nrc_spi_readbyte_value(0x73|0x80, &data) != NRC_SUCCESS)
		return -1;

	nrc_usr_print("[%s] spi_mem_page default value=0x%02x\n", __func__, data);

	return BME680_OK;
}
#elif defined(BME680_I2C)

#define BME680_I2C_SCL    16
#define BME680_I2C_SDA    17
#define BME680_I2C_CLOCK  100000
#define I2C_XACT_DELAY_MS 1

static int8_t i2c_read(uint8_t dev_id, uint8_t reg, uint8_t *data, uint16_t len)
{
    nrc_i2c_start();
    nrc_i2c_writebyte((dev_id<<1) | 0x00);
    nrc_i2c_writebyte(reg);
    nrc_i2c_stop();
    _delay_ms(I2C_XACT_DELAY_MS);

    nrc_i2c_start();
    nrc_i2c_writebyte((dev_id<<1) | 0x01);
    for(int i=0;i<len;i++)
        nrc_i2c_readbyte(data++, i < len - 1);

    nrc_i2c_stop();
    _delay_ms(I2C_XACT_DELAY_MS);
	return BME680_OK;
}

static int8_t i2c_write(uint8_t dev_id, uint8_t reg, uint8_t *data, uint16_t len)
{
    nrc_i2c_start();
    nrc_i2c_writebyte((dev_id<<1) | 0x00);
    nrc_i2c_writebyte(reg);
    for(int i=0;i<len;i++)
        nrc_i2c_writebyte(*data++);
    nrc_i2c_stop();
    _delay_ms(I2C_XACT_DELAY_MS);
	return BME680_OK;
}


static int i2c_init(void)
{
	nrc_i2c_init(BME680_I2C_SCL, BME680_I2C_SDA, BME680_I2C_CLOCK);
	nrc_i2c_enable(true);
    return BME680_OK;
}
#endif

static void delay_ms(uint32_t period)
{
	_delay_ms(period);
}

static int bme680_setting(struct bme680_dev  *sensor)
{
	uint8_t set_required_settings = 0;
	int8_t rslt = 0;

	sensor->dev_id = BME680_I2C_ADDR_PRIMARY;
#if defined(BME680_SPI)
	sensor->intf = BME680_SPI_INTF;
	sensor->read = spi_read;
	sensor->write = spi_write;
#elif defined(BME680_I2C)
	sensor->intf = BME680_I2C_INTF;
	sensor->read = i2c_read;
	sensor->write = i2c_write;
#endif
	sensor->delay_ms = delay_ms;
	sensor->amb_temp = 25;

	rslt = BME680_OK;
	rslt = bme680_init(sensor);
	nrc_usr_print("[%s] result=%d\n", __func__, rslt);

	_delay_ms(2000);

	set_required_settings = 0;

	/* Set the temperature, pressure and humidity settings */
	sensor->tph_sett.os_hum = BME680_OS_2X;
	sensor->tph_sett.os_pres = BME680_OS_4X;
	sensor->tph_sett.os_temp = BME680_OS_8X;
	sensor->tph_sett.filter = BME680_FILTER_SIZE_3;

	/* Set the remaining gas sensor settings and link the heating profile */
	sensor->gas_sett.run_gas = BME680_DISABLE_GAS_MEAS;
	/* Create a ramp heat waveform in 3 steps */
	sensor->gas_sett.heatr_temp = 320; /* degree Celsius */
	sensor->gas_sett.heatr_dur = 150; /* milliseconds */

	/* Select the power mode */
	/* Must be set before writing the sensor configuration */
	sensor->power_mode = BME680_FORCED_MODE;

	/* Set the required sensor settings needed */
	set_required_settings = BME680_OST_SEL | BME680_OSH_SEL | BME680_FILTER_SEL;
	//BME680_OSP_SEL | BME680_GAS_SENSOR_SEL;

	/* Set the desired sensor configuration */
	rslt = bme680_set_sensor_settings(set_required_settings,sensor);

	/* Set the power mode */
	rslt = bme680_set_sensor_mode(sensor);

	return 0;
}

static int sensor_init()
{
#if defined(BME680_SPI)
	if(spi_init() != 0) {
		nrc_usr_print ("[%s] Failed to init SPI\n", __func__);
		return NRC_FAIL;
	}
#elif defined(BME680_I2C)
	if(i2c_init() != 0) {
		nrc_usr_print ("[%s] Failed to init I2C\n", __func__);
		return NRC_FAIL;
	}
#endif
	_delay_ms(100);
	if(bme680_setting(&sensor) != 0) {
		nrc_usr_print ("[%s] Failed to init BME680\n", __func__);
		return NRC_FAIL;
	}
	return NRC_SUCCESS;
}

static void update_sensor_data()
{
	if (sensor.power_mode == BME680_FORCED_MODE)
		bme680_set_sensor_mode(&sensor);

	_delay_ms(bme680_meas_period);
	bme680_get_sensor_data(&data, &sensor);

	nrc_usr_print("{\"seq\" : \"%ld\",\t\"T\" : \"%.2f\", \"H\" : \"%.2f\"}\n",
		bme680_sequence_number++,
		((float) data.temperature) / 100.0,
		((float) data.humidity) / 1000.0);
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void user_init(void)
{
    nrc_uart_console_enable(true);

    if(sensor_init() == NRC_FAIL)   {
        nrc_usr_print("[%s] sensor init failed!!!\n");
        return;
    }

    bme680_get_profile_dur(&bme680_meas_period, &sensor);
    nrc_usr_print("meas period = %dms\n", bme680_meas_period);
    while(1)
    {
        update_sensor_data();
        _delay_ms(1000);
    }
}
