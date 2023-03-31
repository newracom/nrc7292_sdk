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

#include "sample_sgp30_sensor.h"
#include "sgp30.h"

typedef struct
{
	struct sgp30_dev sgp30;
	i2c_device_t dev_i2c;
} sht_sensor_t;

sht_sensor_t sensor;

static int8_t
i2c_read(uint8_t dev_id, uint8_t* data, uint16_t len)
{
	nrc_i2c_start(&sensor.dev_i2c);
	nrc_i2c_writebyte(&sensor.dev_i2c, (dev_id << 1) | 0x01);
	for (int i = 0; i < len; i++)
		nrc_i2c_readbyte(&sensor.dev_i2c, data++, i < len - 1);

	nrc_i2c_stop(&sensor.dev_i2c);
	_delay_ms(I2C_XACT_DELAY_MS);

	return SENSOR_OK;
}

static int8_t
i2c_write(uint8_t dev_id, uint8_t* data, uint16_t len)
{
	nrc_i2c_start(&sensor.dev_i2c);
	nrc_i2c_writebyte(&sensor.dev_i2c, (dev_id << 1) | 0x00);
	for (int i = 0; i < len; i++)
		nrc_i2c_writebyte(&sensor.dev_i2c, *data++);
	nrc_i2c_stop(&sensor.dev_i2c);
	_delay_ms(I2C_XACT_DELAY_MS);

	return SENSOR_OK;
}

static int
i2c_init(i2c_device_t* i2c)
{

	if (nrc_i2c_init(i2c) != NRC_SUCCESS) {
		return SENSOR_FAIL;
	}

	if (nrc_i2c_enable(i2c, true) != NRC_SUCCESS) {
		return SENSOR_FAIL;
	}

	return SENSOR_OK;
}

static void
delay_ms(uint32_t period)
{
	_delay_ms(period);
}

int
sensor_init(sht_sensor_t* sensor)
{
	uint8_t set_required_settings = 0;
	int8_t rslt = 0;

	memset(&sensor->sgp30, 0, sizeof(struct sgp30_dev));

	sensor->sgp30.chip_id = SGP30_CHIP_ID_PRIMARY;
	sensor->sgp30.read = i2c_read;
	sensor->sgp30.write = i2c_write;
	sensor->sgp30.delay_ms = delay_ms;

	/* Set i2c */
	sensor->dev_i2c.pin_sda = SENSOR_I2C_SDA;
	sensor->dev_i2c.pin_scl = SENSOR_I2C_SCL;
	sensor->dev_i2c.clock_source = SENSOR_I2C_CLOCK_SOURCE;
	sensor->dev_i2c.controller = I2C_MASTER_0;
	sensor->dev_i2c.clock = SENSOR_I2C_CLOCK;
	sensor->dev_i2c.width = I2C_8BIT;

	i2c_init(&sensor->dev_i2c);
	nrc_i2c_enable(&sensor->dev_i2c, true);
	_delay_ms(100);

	rslt = sgp30_init(&sensor->sgp30);

	return rslt;
}

int
sensor_deinit(sht_sensor_t* sensor)
{
	return nrc_i2c_enable(&sensor->dev_i2c, false);
}

/**
 * @brief Retrieve CO2 from air quality sensor hAT
 *
 * Get current CO2 levels from air quality hAT
 *
 * @returns floating point co2
 */
float
td_get_air_quality_co2(void)
{
	return sgp30_get_co2(&sensor.sgp30);
}

/**
 * @brief Retrieve VOC from air quality sensor hAT
 *
 * Get current VOC levels from air quality hAT
 *
 * @returns floating point voc
 */
float
td_get_air_quality_voc(void)
{
	return sgp30_get_voc(&sensor.sgp30);
}

/**
 * @brief Set absolute humidty in air quality sensor hAT
 *
 * Set absolute humidity from temp, humidity air quality hAT
 *
 * @param wifi configuration ptr
 * @returns nrc_err_t
 */
nrc_err_t
td_set_absolute_humidity(float temp, float humidity)
{
	if (sgp30_set_absolute_humidity(&sensor.sgp30, temp, humidity) == SGP30_OK)
		return NRC_SUCCESS;

	return NRC_FAIL;
}

/**
 * @brief Initialize air quality sensor hAT
 *
 * Initialize air quality sensor
 *
 * @returns nrc_err_t
 */
nrc_err_t
sensor_init_air_quality_hat(void)
{

	if (sensor_init(&sensor) != NRC_SUCCESS) {
		return NRC_FAIL;
	}

	nrc_usr_print("[%s] air quality sensor initialized\n", __func__);

	return NRC_SUCCESS;
}

/**
 * @brief Initialize air quality sensor hAT
 *
 * Initialize air quality sensor
 *
 * @param wifi configuration ptr
 * @returns nrc_err_t
 */
nrc_err_t
sensor_shutdown_air_quality_hat(void)
{

	if (sensor_deinit(&sensor) != NRC_SUCCESS) {
		return NRC_FAIL;
	}

	nrc_usr_print("[%s] air quality sensor de-initialized\n", __func__);

	return NRC_SUCCESS;
}

static void update_sensor_data()
{
	nrc_usr_print("{\"CO2\" : %.2f, \"Volatile Organic Compounds\" : %.2f}\n",
				  td_get_air_quality_co2(),
				  td_get_air_quality_voc());
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

	if (sensor_init_air_quality_hat() == NRC_FAIL) {
		nrc_usr_print("[%s] sensor init failed!!!\n", __func__);
		return;
	}

	nrc_usr_print("** Reading CO2 (ppm) and VOC (ppb) from SGP30 sensor... **\n");
	while(1) {
		update_sensor_data();
		_delay_ms(10000);
	}
}
