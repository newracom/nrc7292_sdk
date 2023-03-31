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
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
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
 * @file sgp30_defs.h
 * @author James Ewing
 * @date 11 Apr 2022
 * @brief Teledatics I2C sgp30 Gas Sensor Library
 */

#ifndef SGP30_DEFS_H_
#define SGP30_DEFS_H_

/********************************************************/
/* header includes */
#include <math.h>
#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/types.h>
#else
#include <stddef.h>
#include <stdint.h>
#endif

/******************************************************************************/
/*! @name		Common macros					      */
/******************************************************************************/

#if !defined(UINT8_C) && !defined(INT8_C)
#define INT8_C(x) S8_C(x)
#define UINT8_C(x) U8_C(x)
#endif

#if !defined(UINT16_C) && !defined(INT16_C)
#define INT16_C(x) S16_C(x)
#define UINT16_C(x) U16_C(x)
#endif

#if !defined(INT32_C) && !defined(UINT32_C)
#define INT32_C(x) S32_C(x)
#define UINT32_C(x) U32_C(x)
#endif

#if !defined(INT64_C) && !defined(UINT64_C)
#define INT64_C(x) S64_C(x)
#define UINT64_C(x) U64_C(x)
#endif

#ifdef SGP30_LOOKUP_TABLE
// lookup table for CRC8
// http://www.sunshine2k.de/coding/javascript/crc/crc_js.html
const uint8_t crc8_lookup_tbl[16][16] = { { 0x00,
                                            0x31,
                                            0x62,
                                            0x53,
                                            0xC4,
                                            0xF5,
                                            0xA6,
                                            0x97,
                                            0xB9,
                                            0x88,
                                            0xDB,
                                            0xEA,
                                            0x7D,
                                            0x4C,
                                            0x1F,
                                            0x2E },
                                          { 0x43,
                                            0x72,
                                            0x21,
                                            0x10,
                                            0x87,
                                            0xB6,
                                            0xE5,
                                            0xD4,
                                            0xFA,
                                            0xCB,
                                            0x98,
                                            0xA9,
                                            0x3E,
                                            0x0F,
                                            0x5C,
                                            0x6D },
                                          { 0x86,
                                            0xB7,
                                            0xE4,
                                            0xD5,
                                            0x42,
                                            0x73,
                                            0x20,
                                            0x11,
                                            0x3F,
                                            0x0E,
                                            0x5D,
                                            0x6C,
                                            0xFB,
                                            0xCA,
                                            0x99,
                                            0xA8 },
                                          { 0xC5,
                                            0xF4,
                                            0xA7,
                                            0x96,
                                            0x01,
                                            0x30,
                                            0x63,
                                            0x52,
                                            0x7C,
                                            0x4D,
                                            0x1E,
                                            0x2F,
                                            0xB8,
                                            0x89,
                                            0xDA,
                                            0xEB },
                                          { 0x3D,
                                            0x0C,
                                            0x5F,
                                            0x6E,
                                            0xF9,
                                            0xC8,
                                            0x9B,
                                            0xAA,
                                            0x84,
                                            0xB5,
                                            0xE6,
                                            0xD7,
                                            0x40,
                                            0x71,
                                            0x22,
                                            0x13 },
                                          { 0x7E,
                                            0x4F,
                                            0x1C,
                                            0x2D,
                                            0xBA,
                                            0x8B,
                                            0xD8,
                                            0xE9,
                                            0xC7,
                                            0xF6,
                                            0xA5,
                                            0x94,
                                            0x03,
                                            0x32,
                                            0x61,
                                            0x50 },
                                          { 0xBB,
                                            0x8A,
                                            0xD9,
                                            0xE8,
                                            0x7F,
                                            0x4E,
                                            0x1D,
                                            0x2C,
                                            0x02,
                                            0x33,
                                            0x60,
                                            0x51,
                                            0xC6,
                                            0xF7,
                                            0xA4,
                                            0x95 },
                                          { 0xF8,
                                            0xC9,
                                            0x9A,
                                            0xAB,
                                            0x3C,
                                            0x0D,
                                            0x5E,
                                            0x6F,
                                            0x41,
                                            0x70,
                                            0x23,
                                            0x12,
                                            0x85,
                                            0xB4,
                                            0xE7,
                                            0xD6 },
                                          { 0x7A,
                                            0x4B,
                                            0x18,
                                            0x29,
                                            0xBE,
                                            0x8F,
                                            0xDC,
                                            0xED,
                                            0xC3,
                                            0xF2,
                                            0xA1,
                                            0x90,
                                            0x07,
                                            0x36,
                                            0x65,
                                            0x54 },
                                          { 0x39,
                                            0x08,
                                            0x5B,
                                            0x6A,
                                            0xFD,
                                            0xCC,
                                            0x9F,
                                            0xAE,
                                            0x80,
                                            0xB1,
                                            0xE2,
                                            0xD3,
                                            0x44,
                                            0x75,
                                            0x26,
                                            0x17 },
                                          { 0xFC,
                                            0xCD,
                                            0x9E,
                                            0xAF,
                                            0x38,
                                            0x09,
                                            0x5A,
                                            0x6B,
                                            0x45,
                                            0x74,
                                            0x27,
                                            0x16,
                                            0x81,
                                            0xB0,
                                            0xE3,
                                            0xD2 },
                                          { 0xBF,
                                            0x8E,
                                            0xDD,
                                            0xEC,
                                            0x7B,
                                            0x4A,
                                            0x19,
                                            0x28,
                                            0x06,
                                            0x37,
                                            0x64,
                                            0x55,
                                            0xC2,
                                            0xF3,
                                            0xA0,
                                            0x91 },
                                          { 0x47,
                                            0x76,
                                            0x25,
                                            0x14,
                                            0x83,
                                            0xB2,
                                            0xE1,
                                            0xD0,
                                            0xFE,
                                            0xCF,
                                            0x9C,
                                            0xAD,
                                            0x3A,
                                            0x0B,
                                            0x58,
                                            0x69 },
                                          { 0x04,
                                            0x35,
                                            0x66,
                                            0x57,
                                            0xC0,
                                            0xF1,
                                            0xA2,
                                            0x93,
                                            0xBD,
                                            0x8C,
                                            0xDF,
                                            0xEE,
                                            0x79,
                                            0x48,
                                            0x1B,
                                            0x2A },
                                          { 0xC1,
                                            0xF0,
                                            0xA3,
                                            0x92,
                                            0x05,
                                            0x34,
                                            0x67,
                                            0x56,
                                            0x78,
                                            0x49,
                                            0x1A,
                                            0x2B,
                                            0xBC,
                                            0x8D,
                                            0xDE,
                                            0xEF },
                                          { 0x82,
                                            0xB3,
                                            0xE0,
                                            0xD1,
                                            0x46,
                                            0x77,
                                            0x24,
                                            0x15,
                                            0x3B,
                                            0x0A,
                                            0x59,
                                            0x68,
                                            0xFF,
                                            0xCE,
                                            0x9D,
                                            0xAC } };
#endif

/**@}*/

/**\name C standard macros */
#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void*)0)
#endif
#endif

/** SGP30 configuration macros */
#define SGP30_IAQ_INIT UINT16_C(0X2003)
#define SGP30_MEASURE_IAQ UINT16_C(0x2008)
#define SGP30_GET_IAQ_BASELINE UINT16_C(0x2015)
#define SGP30_SET_IAQ_BASELINE UINT16_C(0X201E)
#define SGP30_SET_ABSOLUTE_HUMIDITY UINT16_C(0x2061)
#define SGP30_MEASURE_TEST UINT16_C(0x2032)
#define SGP30_GET_FEATURE_SET UINT16_C(0X202F)
#define SGP30_MEASURE_RAW UINT16_C(0x2050)
#define SGP30_GET_TVOC_INCEPTIVE_BASELINE UINT16_C(0X20B3)
#define SGP30_SET_TVOC_BASELINE UINT16_C(0x2077)

/** Soft reset command */
#define SGP30_SOFT_RESET_CMD UINT16_C(0x0006)

/** Get serial ID command */
#define SGP30_SERIAL_ID_CMD UINT16_C(0x3682)

/** SGP30 unique chip identifier */
#define SGP30_CHIP_ID_PRIMARY UINT8_C(0x58)

/** Error code definitions */
#define SGP30_OK INT8_C(0)

/* Errors */
#define SGP30_E_NULL_PTR INT8_C(-1)
#define SGP30_E_COM_FAIL INT8_C(-2)
#define SGP30_E_DEV_NOT_FOUND INT8_C(-3)
#define SGP30_E_INVALID_LENGTH INT8_C(-4)
#define SGP30_E_SELF_TEST_FAIL INT8_C(-5)

#define SGP30_SELF_TEST_REF UINT16_C(0xD400)

#define SGP30_SELF_TEST_DELAY 220

#define SGP30_DELAY_MS 20

#define UNKNOWN_VALUE NAN

/** Type definitions */
/*!
 * Generic communication function pointer
 * @param[in/out] reg_data: Data array to read/write
 * @param[in] len: Length of the data array
 */
typedef int8_t (*sgp30_com_fptr_t)(uint8_t chip_id,
                                   uint8_t* data,
                                   uint16_t len);

/*!
 * Delay function pointer
 * @param[in] period: Time period in milliseconds
 */
typedef void (*sgp30_delay_fptr_t)(uint32_t period);

/*!
 * @brief SGP30 device structure
 */
struct sgp30_dev
{

  /*! Chip Id */
  uint8_t chip_id;

  /*! Serial ID */
  uint64_t serial_id;

  /* feature set */
  uint16_t features;

  /*! H2 test value */
  uint16_t h2;

  /*! Ethanol test value */
  uint16_t ethanol;

  /*! CO2 concentration */
  uint16_t co2;
  uint16_t co2_baseline;

  /*! VOC concentration */
  uint16_t voc;
  uint16_t voc_baseline;

  /*! Bus read function pointer */
  sgp30_com_fptr_t read;

  /*! Bus write function pointer */
  sgp30_com_fptr_t write;

  /*! delay function pointer */
  sgp30_delay_fptr_t delay_ms;
};

#endif /* SGP30_DEFS_H_ */
/** @}*/
