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
 * Adapted by Teledatics, Inc. from bme680 source code
 * Copyright (c) 2020 Bosch Sensortec GmbH. All rights reserved.
 *
 */

 /**
 * @file sht30.c
 * @author James Ewing
 * @date 7 Apr 2022
 * @brief Teledatics I2C sht30 Gas Sensor Library
 */
 
 #include "nrc_sdk.h"
 #include "sht30.h"

/*!
 *  @brief SHT30 send command utility
 *  
 * Send a command to the SHT30 over I2C
 *
 *  @param[in] dev : Structure instance of sht30_dev
 *  @param[in] cmd : command to send
 *
 *  @return Result of API execution status
 *  @retval zero -> Success / -ve value -> Error
 */
static int sht30_send_cmd(struct sht30_dev *dev, uint16_t cmd)
{
        int rslt;
        uint8_t data[2];

        if(!dev)
                return SHT30_E_NULL_PTR;
        
        data[0] = (uint8_t)((cmd >> 8) & 0xFF); 
        data[1] = (uint8_t)(cmd & 0xFF);
        rslt = dev->write(dev->chip_id, (uint8_t*)data, sizeof(data));
        
        if(rslt)
                return SHT30_E_COM_FAIL;
        
        return SHT30_OK;
        
}

/*!
 *  @brief SHT30 read data utility
 *  
 * Read data from the SHT30 over I2C
 *
 *  @param[in] dev : Structure instance of sht30_dev
 *  @param[in] data : buffer for I2C data received
 *  @param[in] len : buffer length
 *
 *  @return Result of API execution status
 *  @retval zero -> Success / -ve value -> Error
 */
static int sht30_read_data(struct sht30_dev *dev, uint8_t* data, uint8_t len)
{
        int8_t rslt;

        if(!dev || !data)
                return SHT30_E_NULL_PTR;
        
        if(!len)
         
        rslt = dev->read(dev->chip_id, data, len);
        
        if(rslt)
                return SHT30_E_COM_FAIL;
        
        return SHT30_OK;
}

/*!
 *  @brief This API is the entry point.
 *  It reads the chip-id and calibration data from the sensor.
 *
 *  @param[in,out] dev : Structure instance of sht30_dev
 *
 *  @return Result of API execution status
 *  @retval zero -> Success / -ve value -> Error
 */
int8_t sht30_init(struct sht30_dev *dev)
{
	int8_t rslt;
        
        if(!dev)
                return SHT30_E_NULL_PTR;
        
        /* Soft reset to restore it to default values*/
        rslt = sht30_soft_reset(dev);
        
        if (rslt == SHT30_OK) {
                rslt = sht30_get_sensor_data(dev);

                if (rslt == SHT30_OK) {
                        rslt = sht30_get_status(dev);
                }
        }

	return rslt;
}

/*!
 * @brief This API reads the status from the sensor.
 *
 * @param[in] dev : Structure instance of sht30_dev.
 *
 * @return Result of API execution status
 * @retval zero -> Success / -ve value -> Error
 */
int8_t sht30_get_status(struct sht30_dev *dev)
{
        int8_t rslt;
        uint8_t data[3];
        
        if(!dev)
                return SHT30_E_NULL_PTR;
        
        rslt = sht30_send_cmd(dev, SHT30_READ_STATUS_CMD);

        if(rslt == SHT30_OK) {
                rslt = dev->read(dev->chip_id, data, sizeof(data));
                
                if(rslt)
                        return SHT30_E_COM_FAIL;

                dev->status = (uint16_t)(data[0] << 8 | data[1]) ;
                
                rslt = SHT30_OK;
        }
        
        return rslt;
}

/*!
 * @brief This API performs the soft reset of the sensor.
 *
 * @param[in] dev : Structure instance of sht30_dev.
 *
 * @return Result of API execution status
 * @retval zero -> Success / -ve value -> Error.
 */
int8_t sht30_soft_reset(struct sht30_dev *dev)
{
	int8_t rslt;
	uint16_t cmd = SHT30_SOFT_RESET_CMD;
        
	if(!dev)
                return SHT30_E_NULL_PTR;
	
        
        rslt = sht30_send_cmd(dev, cmd);
        
        if(rslt == SHT30_OK) {
                dev->delay_ms(SHT30_DELAY_MS);
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
static uint8_t crc8(const uint8_t *data, int len) 
{
        const uint8_t polynomial=0x31;
        uint8_t crc=0xFF;

        for (int j = len; j; --j) {
                crc ^= *data++;

                for (int i = 8; i; --i) {
                        crc = (crc & 0x80) ? (crc << 1) ^ polynomial : (crc << 1);
                }
        }
        
        return crc;
}

/*!
 * @brief This API reads temperature and humidity
 * from the sensor, compensates the data and stores it in the sht30_dev
 * structure instance passed by the user.
 *
 * @param[in] dev : Structure instance of sht30_dev.
 *
 * @return Result of API execution status
 * @retval zero -> Success / -ve value -> Error
 */
int8_t sht30_get_sensor_data(struct sht30_dev *dev)
{
        int8_t rslt;
        uint8_t data[6];
        
	if(!dev)
                return SHT30_E_NULL_PTR;
        
        rslt = sht30_send_cmd(dev, SHT30_MEAS_HIGHREP);
        
        if(rslt == SHT30_OK) {
                dev->delay_ms(SHT30_DELAY_MS);
        }
        else {
                return rslt;
        }
        
        dev->read(dev->chip_id, data, sizeof(data));
        
        if (data[2] != crc8(data, 2) ||
            data[5] != crc8(data + 3, 2)) {
                return SHT30_E_COM_FAIL;
        }

        dev->temperature = ((data[0] << 8) | data[1]);

        dev->humidity = ((data[3] << 8) | data[4]);
        
        return SHT30_OK;
}
/*!
 * @brief This API reads temperature and humidity
 * from the sensor, compensates the data and store it in the sht30_data
 * structure instance passed by the user.
 *
 * @param[in] dev : Structure instance of sht30_dev.
 *
 * @return Result of API execution status
 * @retval floating point temperature value
 */
float sht30_get_temp(struct sht30_dev *dev)
{
        float temp;
        uint32_t stemp;
        
        if(!dev)
                return UNKNOWN_VALUE;
        
        if(sht30_get_sensor_data(dev) != SHT30_OK){
                return UNKNOWN_VALUE;
        }
        
        stemp = dev->temperature;
        
        temp = (stemp * 175.0f) / 65535.0f - 45.0f;
        stemp = ((4375 * stemp) >> 14) - 4500;
        temp = (float)stemp / 100.0f;
        
        return temp;
}

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
float sht30_get_humidity(struct sht30_dev *dev)
{
        float humidity;
        uint32_t shum;
        
        if(!dev)
                return UNKNOWN_VALUE;
        
        if(sht30_get_sensor_data(dev) != SHT30_OK){
                return UNKNOWN_VALUE;
        }
        
        shum = dev->humidity;
        
        humidity = (shum * 100.0f) / 65535.0f;
        shum = (625 * shum) >> 12;
        humidity = (float)shum / 100.0f;
        
        return humidity;
}

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
int8_t sht30_set_heater(struct sht30_dev *dev, uint8_t on)
{
	int8_t rslt;

        nrc_usr_print("[%s]\n", __func__);
        
        if(!dev)
                return SHT30_E_NULL_PTR;

        if(on) {
                rslt = sht30_send_cmd(dev, SHT30_HEATER_ON_CMD);
        }
        else {
                rslt = sht30_send_cmd(dev, SHT30_HEATER_OFF_CMD);
        }
        
        return rslt;
}
