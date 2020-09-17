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


/**********************************************
 * @fn void nrc_i2c_init(uint32_t clk)
 *
 * @brief Initialize I2C controller
 *
 * @param clk: Clock speed of I2C controller. It can be set up to 400,000Hz.
 *
 * @return N/A
 ***********************************************/
void nrc_i2c_init(uint32_t clk);


/**********************************************
 * @fn void nrc_i2c_enable(bool enable)
 *
 * @brief Enable or disable I2C. And, DO NOT disable I2C while a transaction is in progress.
 * Please disable I2C only after a transaction is stopped.
 *
 * @param enable: true(enable) or false(disable)
 *
 * @return N/A
 ***********************************************/
void nrc_i2c_enable(bool enable);


/**********************************************
 * @fn void nrc_i2c_reset(void)
 *
 * @brief Reset I2C controller
 *
 * @return N/A
 ***********************************************/
void nrc_i2c_reset(void);


/**********************************************
 * @fn void nrc_i2c_start(void)
 *
 * @brief Start I2C operation
 *
 * @return N/A
 ***********************************************/
void nrc_i2c_start(void);


/**********************************************
 * @fn void nrc_i2c_stop(void)
 *
 * @brief Stop I2C operation
 *
 * @return N/A
 ***********************************************/
void nrc_i2c_stop(void);


/**********************************************
 * @fn bool nrc_i2c_waitack(void)
 *
 * @brief Wait ACK or NACK of I2C
 *
 * @return true(ACK) false(NACK)
 ***********************************************/
bool nrc_i2c_waitack(void);


/**********************************************
 * @fn bool nrc_i2c_writebyte(uint8_t data)
 *
 * @brief Write data
 *
 * @param data: data to write
 *
 * @return true or false
 ***********************************************/
bool nrc_i2c_writebyte(uint8_t data);


/**********************************************
 * @fn bool nrc_i2c_readbyte(uint8_t *data, bool ack)
 *
 * @brief Read data using I2C
 *
 * @param data: data to read
 *
 * @param ack: if there's no further reading registers, then false. otherwise, true.
 *
 * @return N/A
 ***********************************************/
bool nrc_i2c_readbyte(uint8_t *data, bool ack);
#endif /* __NRC_API_I2C_H__ */
