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

#ifndef __NRC_API_I2C_H__
#define __NRC_API_I2C_H__

typedef enum {
	I2C_MASTER_0,
	I2C_MASTER_1,
	I2C_MASTER_2,
	I2C_MASTER_MAX
}I2C_CONTROLLER_ID;

typedef enum {
	I2C_WIDTH_8BIT,
	I2C_WIDTH_16BIT
}I2C_WIDTH;

typedef enum {
	I2C_CLOCK_CONTROLLER,
	I2C_CLOCK_PCLK
}I2C_CLOCK_SOURCE;

typedef struct i2c_device {
	uint8_t pin_sda; /* SDA pin */
	uint8_t pin_scl; /* SCL pin */
	uint8_t clock_source; /* clock source, 0:clock controller, 1:PCLK */
	uint8_t controller; /* ID of i2c controller to use */
	uint32_t clock; /* i2c clock (hz) */
	uint32_t width; /* i2c data width */
	uint32_t address; /* i2c address */
} i2c_device_t;

/**********************************************
 * @fn nrc_err_t nrc_i2c_init(i2c_device_t* i2c)
 *
 * @brief Initialize I2C controller
 *
 * @param i2c_device_t* : i2c device config
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_i2c_init(i2c_device_t* i2c);


/**********************************************
 * @fn nrc_err_t nrc_i2c_enable(i2c_device_t* i2c, bool enable)
 *
 * @brief Enable or disable I2C. And, DO NOT disable I2C while a transaction is in progress.
 * Please disable I2C only after a transaction is stopped.
 *
 * @param i2c_device_t* : i2c device config
 *
 * @param enable: true(enable) or false(disable)
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_i2c_enable(i2c_device_t* i2c, bool enable);


/**********************************************
 * @fn nrc_err_t nrc_i2c_reset(i2c_device_t* i2c)
 *
 * @brief Reset I2C controller
 *
 * @param i2c_device_t* : i2c device config
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_i2c_reset(i2c_device_t* i2c);


/**********************************************
 * @fn nrc_err_t nrc_i2c_start(i2c_device_t* i2c)
 *
 * @param i2c_device_t* : i2c device config
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_i2c_start(i2c_device_t* i2c);


/**********************************************
 * @fn nrc_err_t nrc_i2c_stop(i2c_device_t* i2c)
 *
 * @brief Stop I2C operation
 *
 * @param i2c_device_t* : i2c device config
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_i2c_stop(i2c_device_t* i2c);


/**********************************************
 * @fn nrc_err_t nrc_i2c_writebyte(i2c_device_t* i2c, uint8_t data)
 *
 * @brief Write data
 *
 * @param i2c_device_t* : i2c device config
 *
 * @param data: data to write
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_i2c_writebyte(i2c_device_t* i2c, uint8_t data);


/**********************************************
 * @fn bool nrc_i2c_readbyte(i2c_device_t* i2c, uint8_t *data, bool ack)
 *
 * @brief Read data using I2C
 *
 * @param i2c_device_t* : i2c device config
 *
 * @param data: data to read
 *
 * @param ack: if there's no further reading registers, then false. otherwise, true.
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_i2c_readbyte(i2c_device_t* i2c, uint8_t *data, bool ack);
#endif /* __NRC_API_I2C_H__ */
