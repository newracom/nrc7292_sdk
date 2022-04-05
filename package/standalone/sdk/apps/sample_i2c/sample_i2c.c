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

#include "nrc_sdk.h"

#define I2C_SCL 16
#define I2C_SDA 17

#define TEST_COUNT 10
#define TEST_INTERVAL 3000 /* msec */

#define LIS331HH		(0x30) /* VDD:3.3v, VIL(<0.2*VDD), VIH(>0.8*VDD), VOL(<0.1*VDD), VOH(>0.9*VDD) */
//#define L3G4200D		(0xD0) /* VDD:3.3v */
//#define ADXL345		 (0x53 << 1) /* VDD:3.3v */

#if defined( LIS331HH )
#define TEST_SAD		LIS331HH
#elif defined( L3G4200D )
#define TEST_SAD		L3G4200D
#elif defined( ADXL345 )
#define TEST_SAD		ADXL345
#endif

i2c_device_t i2c_dev;

void i2c_read_regs(i2c_device_t* i2c, uint8_t sad, uint8_t reg, uint8_t *value, uint8_t length)
{
	nrc_i2c_start(i2c);
	nrc_i2c_writebyte(i2c, sad);
	nrc_i2c_writebyte(i2c, reg);

	nrc_i2c_start(i2c);
	nrc_i2c_writebyte(i2c, sad|0x1);
	for (int i = 0; i < length; i++) {
		nrc_i2c_readbyte(i2c, value + i, (length - 1) != i);
	}
	nrc_i2c_stop(i2c);
}

void i2c_write_regs(i2c_device_t* i2c, uint8_t sad, uint8_t reg, uint8_t *value, uint8_t length)
{
	nrc_i2c_start(i2c);
	nrc_i2c_writebyte(i2c, sad);
	nrc_i2c_writebyte(i2c, reg);
	for(int i=0;i<length;i++)
		nrc_i2c_writebyte(i2c, *(value+i));

	nrc_i2c_stop(i2c);
}

void i2c_set_config(i2c_device_t* i2c)
{
	i2c->pin_sda = I2C_SDA;
	i2c->pin_scl = I2C_SCL;
	i2c->clock_source =0;
	i2c->controller = I2C_MASTER_0;
	i2c->width = I2C_8BIT;
#if defined( LIS331HH )
	i2c->clock = 200000;
#else
	i2c->clock = 400000;
#endif
	i2c->address = TEST_SAD;
}


/******************************************************************************
 * FunctionName : run_sample_i2c
 * Description  : sample test for i2c
 * Parameters   : count(test count), interval(test interval)
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_i2c(int count, int interval)
{
	int i = 0;
	uint8_t write_value = 0x0F;
	uint8_t read_value = 0;
	uint16_t x =0, y=0, z=0;
	uint8_t data[6];

	nrc_usr_print("[%s] Sample App for I2C \n", __func__);

	i2c_set_config(&i2c_dev);
	nrc_i2c_init(&i2c_dev);
	nrc_i2c_enable(&i2c_dev, true);
	_delay_ms(interval);

#if defined( L3G4200D )
	/*
	 *  Pin configuration: CS must be high, SDO must be low.
	 */

	/* WHO_AM_I default value : 0xD3 */
	nrc_usr_print("[%s] Slave Device: L3G4200D\n  - WHO_AM_I=0xD3\n", __func__);
	while (1)
	{
		i2c_read_regs(&i2c_dev, TEST_SAD, 0x0F, &read_value, 1);
		if (read_value == 0xD3) {
			nrc_usr_print("[%s] WORKS FINE!!!!!\n", __func__);
			break;
		} else {
			nrc_usr_print("[%s] ERROR..........(0x%02x)\n", __func__, read_value);
			return NRC_FAIL
		}
		_delay_ms(interval);
	}

	i2c_write_regs(&i2c_dev, TEST_SAD, 0x20, &write_value, 1);
	i2c_read_regs(&i2c_dev, TEST_SAD, 0x20, &read_value, 1);
	nrc_usr_print("  - CTRL_REG1=0x%x\n", read_value);
	_delay_ms(interval);


	for(i=0; i<count; i++)
	{
		i2c_read_regs(&i2c_dev, TEST_SAD, (0x28|0x80), data, 6);

		x = (data[1] << 8) | data[0];
		y = (data[3] << 8) | data[2];
		z = (data[5] << 8) | data[4];

		nrc_usr_print("0x%04x 0x%04x 0x%04x\n", x, y, z);
		_delay_ms(10);
	}

#elif defined( ADXL345 )
	/*
	 *  Pin configuration: CS must be high, SDO must be low.
	 */

	/* DEVID default value : 0xE5 */
	nrc_usr_print("[%s] Slave Device: ADXL345\n  - DEVID=0xE5\n", __func__);
	while (1)
	{
		i2c_read_regs(&i2c_dev, TEST_SAD, 0x00, &value1, 1);
		if (value1 == 0xE5) {
			nrc_usr_print("[%s] WORKS FINE!!!!!\n", __func__);
			break;
		} else {
			nrc_usr_print("[%s] ERROR..........(0x%02x)\n", __func__, value1);
			return NRC_FAIL;
		}
		_delay_ms(interval);
	}

	i2c_write_regs(&i2c_dev, TEST_SAD, 0x2D, &write_value, 1);
	i2c_read_regs(&i2c_dev, TEST_SAD, 0x2D, &read_value, 1);
	nrc_usr_print("  - POWER_CTL=0x%x\n", read_value);
	_delay_ms(interval);

	for(i=0; i<count; i++)
	{
		i2c_read_regs(&i2c_dev, TEST_SAD, 0x32, data, 6);

		x = (data[1]<< 8) | data[0];
		y = (data[3] << 8) | data[2];
		z = (data[5] << 8) | data[4];

		nrc_usr_print("0x%04x 0x%04x 0x%04x\n", x, y, z);
		_delay_ms(10);
	}

#elif defined( LIS331HH )
	for(i=0; i<count; i++)
	{
		i2c_read_regs(&i2c_dev, TEST_SAD, 0x20, &read_value, 1);
		/* CTRL_REG1 default value : 0x07 */
		if (read_value == 0x07){
			nrc_usr_print("[%s] Slave Device: LIS331HH\n  - CTRL_REG1=0x%02x\n", __func__, read_value);
		} else {
			nrc_usr_print("[%s] ERROR..........(0x%02x)\n", __func__, read_value);
			return NRC_FAIL;
		}
		_delay_ms(interval);
	}

#else
	nrc_usr_print("[%s] There's no slave device definition.\n", __func__);
#endif
	nrc_i2c_enable(&i2c_dev, false);
	return NRC_SUCCESS;
}


/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void user_init(void)
{
	nrc_err_t ret;

	//Enable Console for Debugging
	nrc_uart_console_enable(true);

	ret = run_sample_i2c(TEST_COUNT, TEST_INTERVAL);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
}
