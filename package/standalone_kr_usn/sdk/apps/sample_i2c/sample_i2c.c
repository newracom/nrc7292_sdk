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
//#define BME680		  (0x76 << 1) /* VDD:3.3v */
//#define L3G4200D		(0xD0) /* VDD:3.3v */
//#define ADXL345		 (0x53 << 1) /* VDD:3.3v */

#if defined( LIS331HH )
#define TEST_SAD		LIS331HH
#elif defined( BME680 )
#define TEST_SAD		BME680
#elif defined( L3G4200D )
#define TEST_SAD		L3G4200D
#elif defined( ADXL345 )
#define TEST_SAD		ADXL345
#endif


void i2c_read_reg(uint8_t sad, uint8_t reg, uint8_t *value)
{
	nrc_i2c_start();
	nrc_i2c_writebyte(sad);
	nrc_i2c_waitack();
	nrc_i2c_writebyte(reg);
	nrc_i2c_waitack();

	nrc_i2c_start();
	nrc_i2c_writebyte(sad|0x1);
	nrc_i2c_waitack();
	nrc_i2c_readbyte(value, false);
	nrc_i2c_stop();
}

void i2c_read_regs(uint8_t sad, uint8_t reg, uint8_t *value, uint8_t length)
{
	nrc_i2c_start();
	nrc_i2c_writebyte(sad);
	nrc_i2c_waitack();
	nrc_i2c_writebyte(reg);
	nrc_i2c_waitack();

	nrc_i2c_start();
	nrc_i2c_writebyte(sad|0x1);
	nrc_i2c_waitack();
	for (int i = 0; i < length; i++) {
		nrc_i2c_readbyte(value + i, (length - 1) != i);
	}
	nrc_i2c_stop();
}

void i2c_write_reg(uint8_t sad, uint8_t reg, uint8_t value)
{
	nrc_i2c_start();
	nrc_i2c_writebyte(sad);
	nrc_i2c_waitack();
	nrc_i2c_writebyte(reg);
	nrc_i2c_waitack();
	nrc_i2c_writebyte(value);
	nrc_i2c_waitack();
	nrc_i2c_stop();
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
	nrc_usr_print("[%s] Sample App for I2C \n", __func__);

#if defined( LIS331HH )
	/* SCL = 200kHz */
	nrc_i2c_init(I2C_SCL, I2C_SDA, 200000);
#else
	/* SCL = 400kHz */
	nrc_i2c_init(I2C_SCL, I2C_SDA, 400000);
#endif

	nrc_i2c_enable(true);
	_delay_ms(interval);

	uint8_t value1, value2;

#if defined( L3G4200D )
	/*
	 *  Pin configuration: CS must be high, SDO must be low.
	 */

	/* WHO_AM_I default value : 0xD3 */
	nrc_usr_print("[%s] Slave Device: L3G4200D\n  - WHO_AM_I=0xD3\n", __func__);
	while (1)
	{
		i2c_read_reg(TEST_SAD, 0x0F, &value1);
		if (value1 == 0xD3) {
			nrc_usr_print("[%s] WORKS FINE!!!!!\n", __func__);
			break;
		} else {
			nrc_usr_print("[%s] ERROR..........(0x%02x)\n", __func__, value1);
			return NRC_FAIL
		}
		_delay_ms(interval);
	}

	i2c_write_reg(TEST_SAD, 0x20, 0x0F);
	i2c_read_reg(TEST_SAD, 0x20, &value2);
	nrc_usr_print("  - CTRL_REG1=0x%x\n", value2);
	_delay_ms(interval);

	short x, y, z;
	uint8_t data[6];
	for(i=0; i<count; i++)
	{
		i2c_read_regs(TEST_SAD, (0x28|0x80), data, 6);

		x = data[1];
		x = (x << 8) | data[0];
		y = data[3];
		y = (y << 8) | data[2];
		z = data[5];
		z = (z << 8) | data[4];

		nrc_usr_print("0x%04x 0x%04x 0x%04x\n", (0xffff & x), (0xffff & y), (0xffff & z));
		//nrc_usr_print("%05d\t%05d\t%05d\n", x, y, z);
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
		i2c_read_reg(TEST_SAD, 0x00, &value1);
		if (value1 == 0xE5) {
			nrc_usr_print("[%s] WORKS FINE!!!!!\n", __func__);
			break;
		} else {
			nrc_usr_print("[%s] ERROR..........(0x%02x)\n", __func__, value1);
			return NRC_FAIL;
		}
		_delay_ms(interval);
	}

	i2c_write_reg(TEST_SAD, 0x2D, 0x0F);
	i2c_read_reg(TEST_SAD, 0x2D, &value2);
	nrc_usr_print("  - POWER_CTL=0x%x\n", value2);
	_delay_ms(interval);

	short x, y, z;
	uint8_t data[6];

	for(i=0; i<count; i++)
	{
		i2c_read_regs(TEST_SAD, 0x32, data, 6);

		x = data[1];
		x = (x << 8) | data[0];
		y = data[3];
		y = (y << 8) | data[2];
		z = data[5];
		z = (z << 8) | data[4];

		nrc_usr_print("0x%04x 0x%04x 0x%04x\n", (0xffff & x), (0xffff & y), (0xffff & z));
		//nrc_usr_print("%05d\t%05d\t%05d\n", x, y, z);
		_delay_ms(10);
	}

#elif defined( LIS331HH )
	for(i=0; i<count; i++)
	{
		i2c_read_reg(TEST_SAD, 0x20, &value1);
		/* CTRL_REG1 default value : 0x07 */
		if (value1 == 0x07){
			nrc_usr_print("[%s] Slave Device: LIS331HH\n  - CTRL_REG1=0x%02x\n", __func__, value1);
		} else {
			nrc_usr_print("[%s] ERROR..........(0x%02x)\n", __func__, value1);
			return NRC_FAIL;
		}
		_delay_ms(interval);
	}

#elif defined( BME680 )
	/* Chip-DI default value : 0x61 */
	nrc_usr_print("[%s] Slave Device: BME680\n  - Chip-ID=0x61\n", __func__);
	for(i=0; i<count; i++)
	{
		i2c_read_reg(TEST_SAD, 0xD0, &value1);
		if (value1 == 0x61) {
			nrc_usr_print("[%s] WORKS FINE!!!!!\n", __func__);
		} else {
			nrc_usr_print("[%s] ERROR..........(0x%02x)\n", __func__, value1);
			return NRC_FAIL;
		}
		_delay_ms(interval);
	}
#else
	nrc_usr_print("[%s] There's no slave device definition.\n", __func__);
#endif
	nrc_i2c_enable(false);
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
	nrc_uart_console_enable();

	ret = run_sample_i2c(TEST_COUNT, TEST_INTERVAL);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
}
