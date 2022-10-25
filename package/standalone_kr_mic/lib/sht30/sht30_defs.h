/*
 * MIT License
 *
 * Copyright (c) 2022 Teledatics, Inc.
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

 /**
 * @file sht30_defs.h
 * @author James Ewing
 * @date 7 Apr 2022
 * @brief Teledatics I2C sht30 Gas Sensor Library
 */
 
#ifndef SHT30_DEFS_H_
#define SHT30_DEFS_H_

/********************************************************/
/* header includes */
#include <math.h>
#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/kernel.h>
#else
#include <stdint.h>
#include <stddef.h>
#endif

/******************************************************************************/
/*! @name		Common macros					      */
/******************************************************************************/

#if !defined(UINT8_C) && !defined(INT8_C)
#define INT8_C(x)       S8_C(x)
#define UINT8_C(x)      U8_C(x)
#endif

#if !defined(UINT16_C) && !defined(INT16_C)
#define INT16_C(x)      S16_C(x)
#define UINT16_C(x)     U16_C(x)
#endif

#if !defined(INT32_C) && !defined(UINT32_C)
#define INT32_C(x)      S32_C(x)
#define UINT32_C(x)     U32_C(x)
#endif

#if !defined(INT64_C) && !defined(UINT64_C)
#define INT64_C(x)      S64_C(x)
#define UINT64_C(x)     U64_C(x)
#endif

/**@}*/

/**\name C standard macros */
#ifndef NULL
#ifdef __cplusplus
#define NULL   0
#else
#define NULL   ((void *) 0)
#endif
#endif

/** SHT30 configuration macros */
#define SHT30_MEAS_HIGHREP_STRETCH	UINT16_C(0x2C06) /* Measurement High Repeatability with Clock Stretch Enabled */
#define SHT30_MEAS_MEDREP_STRETCH 	UINT16_C(0x2C0D) /* Measurement Medium Repeatability with Clock Stretch Enabled */
#define SHT30_MEAS_LOWREP_STRETCH 	UINT16_C(0x2C10) /* Measurement Low Repeatability with Clock Stretch Enabled*/
#define SHT30_MEAS_HIGHREP 		UINT16_C(0x2400) /* Measurement High Repeatability with Clock Stretch Disabled */
#define SHT30_MEAS_MEDREP 		UINT16_C(0x240B) /* Measurement Medium Repeatability with Clock Stretch Disabled */
#define SHT30_MEAS_LOWREP 		UINT16_C(0x2416) /* Measurement Low Repeatability with Clock Stretch Disabled */

#define SHT30_POLL_ART_4MHZ	UINT16_C(0x2B32)
#define SHT30_POLL_STOP		UINT16_C(0x3093)

/** SHT30 unique chip identifier */
#define SHT30_CHIP_ID_PRIMARY	UINT8_C(0x44)
#define SHT30_CHIP_ID_SECONDARY	UINT8_C(0x45)

/** Soft reset command */
#define SHT30_SOFT_RESET_CMD   UINT16_C(0x30A2)

/** Read status command */
#define SHT30_READ_STATUS_CMD   UINT16_C(0xF32D)

/** Clear status command */
#define SHT30_CLEAR_STATUS_CMD	UINT16_C(0x3041)

/** Heater enable command */
#define SHT30_HEATER_ON_CMD	UINT16_C(0x306D)

/** Heater disable command */
#define SHT30_HEATER_OFF_CMD	UINT16_C(0x3066)

#define SHT31_REG_HEATER_BIT 	UINT8_C(0x0d)

/** Error code definitions */
#define SHT30_OK		INT8_C(0)

/* Errors */
#define SHT30_E_NULL_PTR		INT8_C(-1)
#define SHT30_E_COM_FAIL		INT8_C(-2)
#define SHT30_E_DEV_NOT_FOUND		INT8_C(-3)
#define SHT30_E_INVALID_LENGTH		INT8_C(-4)

#define SHT30_DELAY_MS 20

#define UNKNOWN_VALUE NAN

/** Type definitions */
/*!
 * Generic communication function pointer
 * @param[in/out] reg_data: Data array to read/write
 * @param[in] len: Length of the data array
 */
typedef int8_t (*sht30_com_fptr_t)(uint8_t chip_id, uint8_t *data, uint16_t len);

/*!
 * Delay function pointer
 * @param[in] period: Time period in milliseconds
 */
typedef void (*sht30_delay_fptr_t)(uint32_t period);

/*!
 * @brief SHT30 device structure
 */
struct	sht30_dev {
	
	/*! Chip Id */
	uint8_t chip_id;
	
	/*! Status new_data */
	uint64_t status;
	
	/*! Status new_data */
	uint8_t heater;
	
	/*! Temperature in degree celsius x100 */
	int32_t temperature;
	
	/*! Humidity */
	uint32_t humidity;
	
	/*! Bus read function pointer */
	sht30_com_fptr_t read;
	
	/*! Bus write function pointer */
	sht30_com_fptr_t write;
	
	/*! delay function pointer */
	sht30_delay_fptr_t delay_ms;

};

#endif /* SHT30_DEFS_H_ */
/** @}*/
