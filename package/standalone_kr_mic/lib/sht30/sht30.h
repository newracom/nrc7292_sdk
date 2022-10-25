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
 * @file sht30.h
 * @author James Ewing
 * @date 7 Apr 2022
 * @brief Teledatics I2C sht30 Gas Sensor Library
 */
 
#ifndef SHT30_H_
#define SHT30_H_

/*! CPP guard */
#ifdef __cplusplus
extern "C"
{
#endif

/* Header includes */
#include "sht30_defs.h"

/* function prototype declarations */
/*!
 *  @brief This API is the entry point.
 *  It reads the chip-id and calibration data from the sensor.
 *
 *  @param[in,out] dev : Structure instance of sht30_dev
 *
 *  @return Result of API execution status
 *  @retval zero -> Success / -ve value -> Error
 */
int8_t sht30_init(struct sht30_dev *dev);

/*!
 * @brief This API reads the status from the sensor.
 *
 * @param[in] dev : Structure instance of sht30_dev.
 *
 * @return Result of API execution status
 * @retval zero -> Success / -ve value -> Error
 */
int8_t sht30_get_status(struct sht30_dev *dev);

/*!
 * @brief This API performs the soft reset of the sensor.
 *
 * @param[in] dev : Structure instance of sht30_dev.
 *
 * @return Result of API execution status
 * @retval zero -> Success / -ve value -> Error.
 */
int8_t sht30_soft_reset(struct sht30_dev *dev);

/*!
 * @brief This API reads temperature and humidity
 * from the sensor, compensates the data and store it in the sht30_data
 * structure instance passed by the user.
 *
 * @param[in] dev : Structure instance of sht30_dev.
 *
 * @return Result of API execution status
 * @retval zero -> Success / -ve value -> Error
 */
int8_t sht30_get_sensor_data(struct sht30_dev *dev);

/*!
 * @brief This API reads temperature and humidity
 * from the sensor, compensates the data and stores it in the sht30_dev
 * structure instance passed by the user.
 *
 * @param[in] dev : Structure instance of sht30_dev.
 *
 * @return Result of API execution status
 * @retval floating point temperature value
 */
float sht30_get_temp(struct sht30_dev *dev);

/*!
 * @brief This API reads temperature and humidity
 * from the sensor, compensates the data and store it in the sht30_data
 * structure instance passed by the user.
 *
 * @param[in] dev : Structure instance of sht30_dev.
 *
 * @return Result of API execution status
 * @retval floating point humidity value
 */
float sht30_get_humidity(struct sht30_dev *dev);

/*!
 * @brief This API reads temperature and humidity
 * from the sensor, compensates the data and store it in the sht30_data
 * structure instance passed by the user.
 *
 * @param[in] dev : Structure instance of sht30_dev.
 * @param[in] on : boolean to turn heater on or off.
 *
 * @return Result of API execution status
 * @retval zero -> Success / -ve value -> Error
 */
int8_t sht30_set_heater(struct sht30_dev *dev, uint8_t on);

/*!
 * @brief This API is used to set the oversampling, quality level
 * settings in the sensor.
 *
 * @param[in] dev : Structure instance of sht30_dev.
 * @param[in] level : sensor quality level
 * @return Result of API execution status
 * @retval zero -> Success / -ve value -> Error.
 */
int8_t sht30_set_sensor_quality(struct sht30_dev *dev, uint16_t level);

/*!
 * @brief This API is used to get the oversampling frequency, level
 * settings in the sensor.
 *
 * @param[in] dev : Structure instance of sht30_dev.
 * @param[out] level : sensor quality level
 * 
 * @return Result of API execution status
 * @retval zero -> Success / -ve value -> Error.
 */
int8_t sht30_get_sensor_quality(struct sht30_dev *dev, uint16_t* level);

#ifdef __cplusplus
}
#endif /* End of CPP guard */
#endif /* SHT30_H_ */
/** @}*/
