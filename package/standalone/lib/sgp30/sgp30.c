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
 * Adapted by Teledatics, Inc. from bme680 source code
 * Copyright (c) 2020 Bosch Sensortec GmbH. All rights reserved.
 *
 */

/**
 * @file sgp30.c
 * @author James Ewing
 * @date 11 Apr 2022
 * @brief Teledatics I2C sgp30 Gas Sensor Library
 */

#include "sgp30.h"
#include "nrc_sdk.h"

/*!
 *  @brief SGP30 send command utility
 *
 * Send a command to the SGP30 over I2C
 *
 *  @param[in] dev : Structure instance of sgp30_dev
 *  @param[in] cmd : command to send
 *
 *  @return Result of API execution status
 *  @retval zero -> Success / -ve value -> Error
 */
static int
sgp30_send_cmd(struct sgp30_dev* dev, uint16_t cmd)
{
  int rslt;
  uint8_t data[2];

  if (!dev)
    return SGP30_E_NULL_PTR;

  data[0] = (uint8_t)((cmd >> 8) & 0xFF);
  data[1] = (uint8_t)(cmd & 0xFF);
  rslt = dev->write(dev->chip_id, (uint8_t*)data, sizeof(data));

  if (rslt)
    return SGP30_E_COM_FAIL;

  return SGP30_OK;
}

/*!
 *  @brief SGP30 read data utility
 *
 * Read data from the SGP30 over I2C
 *
 *  @param[in] dev : Structure instance of sgp30_dev
 *  @param[in] data : buffer for I2C data received
 *  @param[in] len : buffer length
 *
 *  @return Result of API execution status
 *  @retval zero -> Success / -ve value -> Error
 */
static int
sgp30_read_data(struct sgp30_dev* dev, uint8_t* data, uint8_t len)
{
  int8_t rslt;

  if (!dev || !data)
    return SGP30_E_NULL_PTR;

  if (!len)
    return SGP30_E_INVALID_LENGTH;

  rslt = dev->read(dev->chip_id, data, len);

  if (rslt)
    return SGP30_E_COM_FAIL;

  return SGP30_OK;
}

/*!
 *  @brief SGP30 read data utility
 *
 * Write data to the SGP30 over I2C
 *
 *  @param[in] dev :  Structure instance of sgp30_dev
 *  @param[in] data : buffer for I2C data to be sent
 *  @param[in] len :  buffer length
 *
 *  @return Result of API execution status
 *  @retval zero -> Success / -ve value -> Error
 */
static int
sgp30_write_data(struct sgp30_dev* dev, uint8_t* data, uint8_t len)
{
  int8_t rslt;

  if (!dev || !data)
    return SGP30_E_NULL_PTR;

  if (!len)
    return SGP30_E_INVALID_LENGTH;

  rslt = dev->write(dev->chip_id, data, len);

  if (rslt)
    return SGP30_E_COM_FAIL;

  return SGP30_OK;
}

/*!
 *  @brief This API is the entry point.
 *  It reads the chip-id and calibration data from the sensor.
 *
 *  @param[in,out] dev : Structure instance of sgp30_dev
 *
 *  @return Result of API execution status
 *  @retval zero -> Success / -ve value -> Error
 */
int8_t
sgp30_init(struct sgp30_dev* dev)
{
  int8_t rslt;

  if (!dev)
    return SGP30_E_NULL_PTR;

  /* Soft reset */
  rslt = sgp30_soft_reset(dev);

  if (rslt == SGP30_OK) {

    rslt = sgp30_send_cmd(dev, SGP30_IAQ_INIT);

    if (rslt == SGP30_OK) {
      rslt = sgp30_self_test(dev);
    }

    if (rslt == SGP30_OK) {
      rslt = sgp30_get_sensor_features(dev);
    }

    if (rslt == SGP30_OK) {
      rslt = sgp30_get_sensor_baseline(dev);
    }

    if (rslt == SGP30_OK) {
      rslt = sgp30_get_serial_id(dev);
    }
  }

  return rslt;
}

/*!
 * @brief CRC8 data integrity check
 *
 * CRC-8 formula from page 14 of SHT spec pdf
 *
 * Test data 0xBE, 0xEF should yield 0x92
 *
 * Initialization data 0xFF
 * Polynomial 0x31 (x8 + x5 +x4 +1)
 * Final XOR 0x00
 *
 * @param[in] data : data for CRC8 calculation
 * @param[in] len : data length
 *
 * @return Result of CRC8 calculation
 */
#ifndef SGP30_LOOKUP_TABLE
static uint8_t
crc8(const uint8_t* data, int len)
{
  const uint8_t polynomial = 0x31;
  uint8_t crc = 0xFF;

  for (int j = len; j; --j) {
    crc ^= *data++;

    for (int i = 8; i; --i) {
      crc = (crc & 0x80) ? (crc << 1) ^ polynomial : (crc << 1);
    }
  }

  return crc;
}
#else
static uint8_t
crc8(const uint8_t* data, int len)
{
  uint8_t crc = 0xFF;

  crc ^= (uint8_t)(data >> 8);
  crc = crc8_lookup_tbl[crc >> 4][crc & 0xF];
  crc ^= (uint8_t)data;
  crc = crc8_lookup_tbl[crc >> 4][crc & 0xF];

  return crc;
}
#endif /* SGP30_LOOKUP_TABLE */

/*!
 * @brief This API reads the serial ID from the sensor.
 *
 * @param[in] dev : Structure instance of sgp30_dev.
 *
 * @return Result of API execution
 * @retval zero -> Success / -ve value -> Error
 */
int8_t
sgp30_get_serial_id(struct sgp30_dev* dev)
{
  int8_t rslt;
  uint8_t data[9];

  if (!dev)
    return SGP30_E_NULL_PTR;

  rslt = sgp30_send_cmd(dev, SGP30_SERIAL_ID_CMD);

  if (rslt == SGP30_OK) {
    rslt = sgp30_read_data(dev, data, sizeof(data));

    if (rslt != SGP30_OK)
      return SGP30_E_COM_FAIL;

    if (data[2] != crc8(&data[0], 2) || data[5] != crc8(&data[3], 2) ||
        data[8] != crc8(&data[6], 2)) {
      return SGP30_E_COM_FAIL;
    }

    dev->serial_id = (uint64_t)data[0] << 40 | (uint64_t)data[1] << 32 |
                     (uint64_t)data[3] << 24 | (uint64_t)data[4] << 16 |
                     (uint64_t)data[6] << 8 | (uint64_t)data[7];
  }

  return rslt;
}

/*!
 * @brief This API performs the soft reset of the sensor.
 *
 * @param[in] dev : Structure instance of sgp30_dev.
 *
 * @return Result of API execution status
 * @retval zero -> Success / -ve value -> Error.
 */
int8_t
sgp30_soft_reset(struct sgp30_dev* dev)
{
  int8_t rslt;
  uint16_t cmd = SGP30_SOFT_RESET_CMD;

  if (!dev)
    return SGP30_E_NULL_PTR;

  rslt = sgp30_send_cmd(dev, cmd);

  if (rslt == SGP30_OK) {
    dev->delay_ms(SGP30_DELAY_MS);
  }

  return rslt;
}

/*!
 * @brief This API reads CO2 and VOC
 * from the sensor, compensates the data and stores it in the sgp30_dev
 * structure instance passed by the user.
 *
 * @param[in] dev : Structure instance of sgp30_dev.
 *
 * @return Result of API execution status
 * @retval zero -> Success / -ve value -> Error
 */
int8_t
sgp30_get_sensor_data(struct sgp30_dev* dev)
{
  int8_t rslt;
  uint8_t data[6];

  if (!dev)
    return SGP30_E_NULL_PTR;

  rslt = sgp30_send_cmd(dev, SGP30_MEASURE_IAQ);

  if (rslt == SGP30_OK) {
    dev->delay_ms(SGP30_DELAY_MS);
  } else {
    return rslt;
  }

  rslt = sgp30_read_data(dev, data, sizeof(data));

  if (rslt != SGP30_OK)
    return rslt;

  if (data[2] != crc8(&data[0], 2) || data[5] != crc8(&data[3], 2)) {
    return SGP30_E_COM_FAIL;
  }

  dev->co2 = ((data[0] << 8) | data[1]);

  dev->voc = ((data[3] << 8) | data[4]);

  return SGP30_OK;
}

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
int8_t
sgp30_get_sensor_baseline(struct sgp30_dev* dev)
{
  int8_t rslt;
  uint8_t data[6];

  if (!dev)
    return SGP30_E_NULL_PTR;

  rslt = sgp30_send_cmd(dev, SGP30_GET_IAQ_BASELINE);

  if (rslt == SGP30_OK) {
    dev->delay_ms(SGP30_DELAY_MS);
  } else {
    return rslt;
  }

  rslt = sgp30_read_data(dev, data, sizeof(data));

  if (rslt != SGP30_OK)
    return rslt;

  if (data[2] != crc8(&data[0], 2) || data[5] != crc8(&data[3], 2)) {
    return SGP30_E_COM_FAIL;
  }

  dev->co2_baseline = ((data[0] << 8) | data[1]);

  dev->voc_baseline = ((data[3] << 8) | data[4]);

  return SGP30_OK;
}

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
int8_t
sgp30_get_sensor_features(struct sgp30_dev* dev)
{
  int8_t rslt;
  uint8_t data[3];

  if (!dev)
    return SGP30_E_NULL_PTR;

  rslt = sgp30_send_cmd(dev, SGP30_GET_FEATURE_SET);

  if (rslt == SGP30_OK) {
    dev->delay_ms(SGP30_DELAY_MS);
  } else {
    return rslt;
  }

  rslt = sgp30_read_data(dev, data, sizeof(data));

  if (rslt != SGP30_OK)
    return rslt;

  if (data[2] != crc8(data, 2)) {
    return SGP30_E_COM_FAIL;
  }

  dev->features = ((data[0] << 8) | data[1]);

  return SGP30_OK;
}

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
int8_t
sgp30_self_test(struct sgp30_dev* dev)
{
  int8_t rslt;
  uint8_t data[3];

  if (!dev)
    return SGP30_E_NULL_PTR;

  rslt = sgp30_send_cmd(dev, SGP30_MEASURE_TEST);

  if (rslt == SGP30_OK) {
    dev->delay_ms(SGP30_SELF_TEST_DELAY);
  } else {
    return rslt;
  }

  rslt = sgp30_read_data(dev, data, sizeof(data));

  if (rslt != SGP30_OK) {
    return rslt;
  }

  if (data[2] != crc8(data, 2)) {
    return SGP30_E_COM_FAIL;
  }

  if (SGP30_SELF_TEST_REF != ((data[0] << 8) | data[1])) {
    return SGP30_E_SELF_TEST_FAIL;
  }

  return SGP30_OK;
}

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
int8_t
sgp30_get_sensor_raw_data(struct sgp30_dev* dev)
{
  int8_t rslt;
  uint8_t data[6];

  if (!dev)
    return SGP30_E_NULL_PTR;

  rslt = sgp30_send_cmd(dev, SGP30_MEASURE_RAW);

  if (rslt == SGP30_OK) {
    dev->delay_ms(SGP30_DELAY_MS);
  } else {
    return rslt;
  }

  rslt = sgp30_read_data(dev, data, sizeof(data));

  if (rslt != SGP30_OK)
    return rslt;

  if (data[2] != crc8(&data[0], 2) || data[5] != crc8(&data[3], 2)) {
    return SGP30_E_COM_FAIL;
  }

  dev->h2 = ((data[0] << 8) | data[1]);

  dev->ethanol = ((data[3] << 8) | data[4]);

  return SGP30_OK;
}

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
int8_t
sgp30_set_sensor_baseline(struct sgp30_dev* dev)
{
  int8_t rslt;
  uint8_t data[6];

  if (!dev)
    return SGP30_E_NULL_PTR;

  rslt = sgp30_send_cmd(dev, SGP30_SET_IAQ_BASELINE);

  if (rslt == SGP30_OK) {
    dev->delay_ms(SGP30_DELAY_MS);
  } else {
    return rslt;
  }

  data[0] = (uint8_t)dev->co2_baseline >> 8;
  data[1] = (uint8_t)dev->co2_baseline & 0xFF;
  data[2] = crc8(&data[0], 2);
  data[3] = (uint8_t)dev->voc_baseline >> 8;
  data[4] = (uint8_t)dev->voc_baseline & 0xFF;
  data[5] = crc8(&data[3], 2);

  rslt = sgp30_write_data(dev, data, sizeof(data));

  return rslt;
}

/*!
 * @brief This API reads CO2 and VOC
 * from the sensor, compensates the data and store it in the sgp30_data
 * structure instance passed by the user.
 *
 * @param[in] dev : Structure instance of sgp30_dev.
 *
 * @return Result of API execution status
 * @retval floating point temperature value
 */
float
sgp30_get_co2(struct sgp30_dev* dev)
{

  if (!dev)
    return UNKNOWN_VALUE;

  if (sgp30_get_sensor_data(dev) != SGP30_OK) {
    return UNKNOWN_VALUE;
  }

  return (float)dev->co2;
}

/*!
 * @brief This API reads CO2 and VOC
 * from the sensor, compensates the data and store it in the sgp30_data
 * structure instance passed by the user.
 *
 * @param[in] dev : Structure instance of sgp30_dev.
 *
 * @return Result of API execution status
 * @retval floating point humidity value
 */
float
sgp30_get_voc(struct sgp30_dev* dev)
{

  if (!dev)
    return UNKNOWN_VALUE;

  if (sgp30_get_sensor_data(dev) != SGP30_OK) {
    return UNKNOWN_VALUE;
  }

  return (float)dev->voc;
}

/*!
 * @brief This API sets the absolute humidity value
 * on the sensor, compensates the data and store it in the sgp30_data
 * structure instance passed by the user.
 *
 * taken from
 * https://carnotcycle.wordpress.com/2012/08/04/how-to-convert-relative-humidity-to-absolute-humidity/
 * precision is about 0.1°C in range -30 to 35°C
 *
 * August-Roche-Magnus 	6.1094 exp(17.625 x T)/(T + 243.04)
 * Buck (1981) 		6.1121 exp(17.502 x T)/(T + 240.97)
 * reference
 * https://www.eas.ualberta.ca/jdwilson/EAS372_13/Vomel_CIRES_satvpformulae.html
 *
 * @param[in] dev : Structure instance of sgp30_dev.
 * @param[in] temperature : float temp in deg C
 * @param[in] humidity : relative humidity in % RH
 *
 * @return Result of API execution status
 * @retval zero -> Success / -ve value -> Error
 */
int8_t
sgp30_set_absolute_humidity(struct sgp30_dev* dev,
                            float temperature,
                            float humidity)
{
  int8_t rslt;
  const float mw = 18.01534;  // molar mass of water g/mol
  const float r = 8.31447215; // Universal gas constant J/mol/K
  float temp;
  float ah;
  uint16_t ah_scaled;
  uint8_t ah_data[3];

  if (!dev)
    return SGP30_E_NULL_PTR;

  temp = pow(2.718281828, (17.67 * temperature) / (temperature + 243.5));
  ah = (6.112 * temp * humidity * mw) / ((273.15 + temperature) * r) * 1000;

  ah_scaled = (uint16_t)(((uint64_t)ah * 256 * 16777) >> 24);

  rslt = sgp30_send_cmd(dev, SGP30_SET_ABSOLUTE_HUMIDITY);

  if (rslt == SGP30_OK) {
    dev->delay_ms(SGP30_DELAY_MS);
  } else {
    return rslt;
  }

  ah_data[0] = ah_scaled >> 8;
  ah_data[1] = ah_scaled & 0xFF;
  ah_data[2] = crc8(&ah_data[0], 2);

  rslt = sgp30_write_data(dev, ah_data, sizeof(ah_data));

  if (rslt != SGP30_OK) {
    return SGP30_E_COM_FAIL;
  }

  return rslt;
}
