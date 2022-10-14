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
 * @file sgp30.h
 * @author James Ewing
 * @date 7 Apr 2022
 * @brief Teledatics I2C sgp30 Gas Sensor Library
 */

#ifndef SGP30_H_
#define SGP30_H_

/*! CPP guard */
#ifdef __cplusplus
extern "C"
{
#endif

/* Header includes */
#include "sgp30_defs.h"

  /* function prototype declarations */
  /*!
   *  @brief This API is the entry point.
   *  It reads the chip-id and calibration data from the sensor.
   *
   *  @param[in,out] dev : Structure instance of sgp30_dev
   *
   *  @return Result of API execution status
   *  @retval zero -> Success / -ve value -> Error
   */
  int8_t sgp30_init(struct sgp30_dev* dev);

  /*!
   * @brief This API reads the serial ID from the sensor.
   *
   * @param[in] dev : Structure instance of sgp30_dev.
   *
   * @return Result of API execution status
   * @retval zero -> Success / -ve value -> Error
   */
  int8_t sgp30_get_serial_id(struct sgp30_dev* dev);

  /*!
   * @brief This API performs the soft reset of the sensor.
   *
   * @param[in] dev : Structure instance of sgp30_dev.
   *
   * @return Result of API execution status
   * @retval zero -> Success / -ve value -> Error.
   */
  int8_t sgp30_soft_reset(struct sgp30_dev* dev);

  /*!
   * @brief This API reads co2 and voc
   * from the sensor, compensates the data and store it in the sgp30_data
   * structure instance passed by the user.
   *
   * @param[in] dev : Structure instance of sgp30_dev.
   *
   * @return Result of API execution status
   * @retval zero -> Success / -ve value -> Error
   */
  int8_t sgp30_get_sensor_data(struct sgp30_dev* dev);

  /*!
   * @brief This API reads CO2 and VOC baseline values
   * from the sensor, compensates the data and stores it in the sgp30_dev
   * structure instance passed by the user.
   *
   * @param[in] dev : Structure instance of sgp30_dev.
   *
   * @return Result of API execution status
   * @retval zero -> Success / -ve value -> Error
   */
  int8_t sgp30_get_sensor_baseline(struct sgp30_dev* dev);

  /*!
   * @brief This API reads sensor feature values
   * from the sensor, compensates the data and stores it in the sgp30_dev
   * structure instance passed by the user.
   *
   * @param[in] dev : Structure instance of sgp30_dev.
   *
   * @return Result of API execution status
   * @retval zero -> Success / -ve value -> Error
   */
  int8_t sgp30_get_sensor_features(struct sgp30_dev* dev);

  /*!
   * @brief This API runs the sensor self-test
   * from the sensor, compensates the data and stores it in the sgp30_dev
   * structure instance passed by the user.
   *
   * @param[in] dev : Structure instance of sgp30_dev.
   *
   * @return Result of API execution status
   * @retval zero -> Success / -ve value -> Error
   */
  int8_t sgp30_self_test(struct sgp30_dev* dev);

  /*!
   * @brief This API reads sensor raw data values
   * from the sensor, compensates the data and stores it in the sgp30_dev
   * structure instance passed by the user.
   *
   * @param[in] dev : Structure instance of sgp30_dev.
   *
   * @return Result of API execution status
   * @retval zero -> Success / -ve value -> Error
   */
  int8_t sgp30_get_sensor_raw_data(struct sgp30_dev* dev);

  /*!
   * @brief This API sets CO2 and VOC baseline values
   * from the sensor, compensates the data and stores it in the sgp30_dev
   * structure instance passed by the user.
   *
   * @param[in] dev : Structure instance of sgp30_dev.
   *
   * @return Result of API execution status
   * @retval zero -> Success / -ve value -> Error
   */
  int8_t sgp30_set_sensor_baseline(struct sgp30_dev* dev);

  /*!
   * @brief This API reads co2 and voc
   * from the sensor, compensates the data and stores it in the sgp30_dev
   * structure instance passed by the user.
   *
   * @param[in] dev : Structure instance of sgp30_dev.
   *
   * @return Result of API execution status
   * @retval floating point temperature value
   */
  float sgp30_get_co2(struct sgp30_dev* dev);

  /*!
   * @brief This API reads co2 and voc
   * from the sensor, compensates the data and store it in the sgp30_data
   * structure instance passed by the user.
   *
   * @param[in] dev : Structure instance of sgp30_dev.
   *
   * @return Result of API execution status
   * @retval floating point humidity value
   */
  float sgp30_get_voc(struct sgp30_dev* dev);

  /*!
   * @brief This API reads CO2 and VOC
   * from the sensor, compensates the data and store it in the sgp30_data
   * structure instance passed by the user.
   *
   * @param[in] dev : Structure instance of sgp30_dev.
   * @param[in] temperature : float temp in deg C
   * @param[in] humidity : relative humidity in % RH
   *
   * @return Result of API execution status
   * @retval zero -> Success / -ve value -> Error
   */
  int8_t sgp30_set_absolute_humidity(struct sgp30_dev* dev,
                                     float temperature,
                                     float humidity);

#ifdef __cplusplus
}
#endif /* End of CPP guard */
#endif /* SGP30_H_ */
/** @}*/
