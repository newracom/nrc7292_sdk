/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * SCCB (I2C like) driver.
 *
 */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include "sensor.h"
#include "sccb.h"

#include "nrc_sdk.h"

#define LITTLETOBIG(x)          ((x<<8)|(x>>8))

#define SCCB_FREQ               100 * 1000            /*!< I2C master frequency*/

static i2c_device_t sccb_i2c;

int SCCB_Init(int pin_sda, int pin_scl)
{
	uint8_t slv_addr = 0;
#if CONFIG_OV2640_SUPPORT
	slv_addr = OV2640_SCCB_ADDR;
#elif CONFIG_OV5640_SUPPORT
	slv_addr = OV5640_SCCB_ADDR;
#else
	return -1;
#endif
	system_printf("pin_sda %d pin_scl %d\n", pin_sda, pin_scl);

	sccb_i2c.pin_sda = pin_sda;
	sccb_i2c.pin_scl = pin_scl;
	sccb_i2c.clock_source = 0;
	sccb_i2c.controller = I2C_MASTER_0;
	sccb_i2c.clock = SCCB_FREQ;
	sccb_i2c.width = I2C_8BIT;
	sccb_i2c.address = slv_addr;

	if (nrc_i2c_init(&sccb_i2c) != NRC_SUCCESS) {
		system_printf("Error nrc_i2c_init()...\n");
		return -1;
	}

	if (nrc_i2c_enable(&sccb_i2c, true) != NRC_SUCCESS) {
		system_printf("Error nrc_i2c_enable()...\n");
		return -1;
	}

	return 0;
}

int SCCB_Deinit(void)
{
	if (nrc_i2c_enable(&sccb_i2c, false) != NRC_SUCCESS) {
		system_printf("Error disabling i2c...\n");
		return -1;
	}
	return 0;
}

uint8_t SCCB_Read(uint8_t slv_addr, uint8_t reg)
{
	uint8_t data = 0;

	nrc_i2c_start(&sccb_i2c);
	nrc_i2c_writebyte(&sccb_i2c, slv_addr);
	_delay_ms(1);

	nrc_i2c_writebyte(&sccb_i2c, reg);
	nrc_i2c_stop(&sccb_i2c);
	_delay_ms(1);

	nrc_i2c_start(&sccb_i2c);
	nrc_i2c_writebyte(&sccb_i2c, slv_addr | 0x01);
	_delay_ms(1);

	nrc_i2c_readbyte(&sccb_i2c, &data, false);
	nrc_i2c_stop(&sccb_i2c);
	_delay_ms(1);

	return data;
}

void SCCB_Write(uint8_t slv_addr, uint8_t reg, uint8_t data)
{
	nrc_i2c_start(&sccb_i2c);
	nrc_i2c_writebyte(&sccb_i2c, slv_addr);
	_delay_ms(1);

	nrc_i2c_writebyte(&sccb_i2c, reg);
	_delay_ms(1);

	nrc_i2c_writebyte(&sccb_i2c, data);
	_delay_ms(1);

	nrc_i2c_stop(&sccb_i2c);
	_delay_ms(1);
}

uint8_t SCCB_Read16(uint8_t slv_addr, uint16_t reg)
{
	uint8_t data = 0;
	uint16_t reg_htons = LITTLETOBIG(reg);
	uint8_t *reg_u8 = (uint8_t *)&reg_htons;

	nrc_i2c_start(&sccb_i2c);
	nrc_i2c_writebyte(&sccb_i2c, slv_addr);
	_delay_ms(1);

	nrc_i2c_writebyte(&sccb_i2c, reg_u8[0]);
	nrc_i2c_writebyte(&sccb_i2c, reg_u8[1]);
	nrc_i2c_stop(&sccb_i2c);
	_delay_ms(1);

	nrc_i2c_start(&sccb_i2c);
	nrc_i2c_writebyte(&sccb_i2c, slv_addr | 0x01);
	_delay_ms(1);

	nrc_i2c_readbyte(&sccb_i2c, &data, false);
	nrc_i2c_stop(&sccb_i2c);
	_delay_ms(1);

	return data;
}

void SCCB_Write16(uint8_t slv_addr, uint16_t reg, uint8_t data)
{
	uint16_t reg_htons = LITTLETOBIG(reg);
	uint8_t *reg_u8 = (uint8_t *)&reg_htons;

	nrc_i2c_start(&sccb_i2c);
	nrc_i2c_writebyte(&sccb_i2c, slv_addr);
	_delay_ms(1);

	nrc_i2c_writebyte(&sccb_i2c, reg_u8[0]);
	nrc_i2c_writebyte(&sccb_i2c, reg_u8[1]);
	_delay_ms(1);

	nrc_i2c_writebyte(&sccb_i2c, data);
	_delay_ms(1);

	nrc_i2c_stop(&sccb_i2c);
	_delay_ms(1);
}

uint16_t SCCB_Read_Addr16_Val16(uint8_t slv_addr, uint16_t reg)
{
	uint16_t data = 0;
	uint8_t *data_u8 = (uint8_t *)&data;
	uint16_t reg_htons = LITTLETOBIG(reg);
	uint8_t *reg_u8 = (uint8_t *)&reg_htons;

	nrc_i2c_start(&sccb_i2c);
	nrc_i2c_writebyte(&sccb_i2c, slv_addr);
	_delay_ms(1);

	nrc_i2c_writebyte(&sccb_i2c, reg_u8[0]);
	nrc_i2c_writebyte(&sccb_i2c, reg_u8[1]);
	nrc_i2c_stop(&sccb_i2c);
	_delay_ms(1);

	nrc_i2c_start(&sccb_i2c);
	nrc_i2c_writebyte(&sccb_i2c, slv_addr | 0x01);
	_delay_ms(1);

	nrc_i2c_readbyte(&sccb_i2c, &data_u8[1], false);
	nrc_i2c_readbyte(&sccb_i2c, &data_u8[0], false);
	nrc_i2c_stop(&sccb_i2c);
	_delay_ms(1);

	return data;
}

void SCCB_Write_Addr16_Val16(uint8_t slv_addr, uint16_t reg, uint16_t data)
{
	uint16_t reg_htons = LITTLETOBIG(reg);
	uint8_t *reg_u8 = (uint8_t *)&reg_htons;
	uint16_t data_htons = LITTLETOBIG(data);
	uint8_t *data_u8 = (uint8_t *)&data_htons;

	nrc_i2c_start(&sccb_i2c);
	nrc_i2c_writebyte(&sccb_i2c, slv_addr);
	_delay_ms(1);

	nrc_i2c_writebyte(&sccb_i2c, reg_u8[0]);
	nrc_i2c_writebyte(&sccb_i2c, reg_u8[1]);
	_delay_ms(1);

	nrc_i2c_writebyte(&sccb_i2c, data_u8[0]);
	nrc_i2c_writebyte(&sccb_i2c, data_u8[1]);
	_delay_ms(1);

	nrc_i2c_stop(&sccb_i2c);
	_delay_ms(1);

}
