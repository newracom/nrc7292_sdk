/**
 * Copyright (c) 2020 Bosch Sensortec GmbH. All rights reserved.
 *
 * BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @file	bme680_selftest.h
 * @date	23 Jan 2020
 * @version	3.5.10
 *
 */

/*!
 * @addtogroup bme680_selftest
 * @brief
 * @{*/


#ifndef BME680_SELFTEST_H_
#define BME680_SELFTEST_H_

#include "bme680.h"

/*! CPP guard */
#ifdef __cplusplus
extern "C"
{
#endif

#define BME680_W_SELF_TEST_FAILED 3

/*!
 * @brief       Self-test API for the BME680
 *
 * @param[in]   Device structure containing relevant information on how
 * 					to communicate with the sensor
 *
 * @return      Error code
 * @retval		0	Success
 * @retval      < 0	Error
 * @retval      > 0	Warning	
 */
int8_t bme680_self_test(struct bme680_dev *dev);

/*! CPP guard */
#ifdef __cplusplus
}
#endif

#endif /* BME680_SELFTEST_H_ */

/** @}*/
