/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * SCCB (I2C like) driver.
 *
 */
#ifndef __SCCB_H__
#define __SCCB_H__
#include <stdint.h>
int SCCB_Init(int pin_sda, int pin_scl);
int SCCB_Deinit();
uint8_t SCCB_Read(uint8_t slv_addr, uint8_t reg);
void SCCB_Write(uint8_t slv_addr, uint8_t reg, uint8_t data);
uint8_t SCCB_Read16(uint8_t slv_addr, uint16_t reg);
void SCCB_Write16(uint8_t slv_addr, uint16_t reg, uint8_t data);
uint16_t SCCB_Read_Addr16_Val16(uint8_t slv_addr, uint16_t reg);
void SCCB_Write_Addr16_Val16(uint8_t slv_addr, uint16_t reg, uint16_t data);
#endif // __SCCB_H__
