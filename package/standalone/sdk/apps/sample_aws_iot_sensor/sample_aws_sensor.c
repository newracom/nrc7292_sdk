/*
 * MIT License
 *
 * Copyright (c) 2022 Newracom, Inc.
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
#include "lwip/netif.h"
#include "lwip/errno.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include "nrc_lwip.h"

#include "wifi_config_setup.h"
#include "wifi_connect_common.h"

#include "bme680.h"
#include "mqtt_mutual_auth.h"

struct bme680_dev sensor;
struct bme680_field_data data;
static spi_device_t bme680_spi;

static volatile float g_temperature = 0;
static volatile float g_humidity    = 0;

static char payload[1024];
/**
 * @brief This parameter will avoid infinite loop of publish and exit the program after certain number of publishes
 */
uint32_t publishCount = 0;

//#define USE_BME680_SENSOR_DATA

static int8_t spi_read(uint8_t dev_id, uint8_t reg, uint8_t *data, uint16_t len)
{
	unsigned short i = 0;

	while (len--) {
		//nrc_usr_print("[%s] read reg=0x%02x (", __func__, reg + i);
		if (nrc_spi_readbyte_value(&bme680_spi, reg + i, data + i) != NRC_SUCCESS)
			return -1;
		//nrc_usr_print("0x%02x)\n", *(data + i));
		++i;
	}
	return BME680_OK;
}

static int8_t spi_write(uint8_t dev_id, uint8_t reg, uint8_t *data, uint16_t len)
{
	unsigned short i = 0;

	while (len--) {
		nrc_spi_writebyte_value(&bme680_spi, reg, *(data + i));
		++i;
		reg = *(data + i);
		++i;
	}
	return BME680_OK;
}

static int spi_init(void)
{
	int count = 0;
	uint8_t data;

	/* Set BME680 spi */
	bme680_spi.pin_miso = 12;
	bme680_spi.pin_mosi = 13;
	bme680_spi.pin_cs =14;
#ifdef NRC7292
	bme680_spi.pin_sclk = 15;
#else
	bme680_spi.pin_sclk = 17;
#endif
	bme680_spi.frame_bits = SPI_BIT8;
	bme680_spi.clock = 1000000;
	bme680_spi.mode = SPI_MODE3;
	bme680_spi.controller = SPI_CONTROLLER_SPI0;
	bme680_spi.irq_save_flag = 0;
	bme680_spi.isr_handler = NULL;

	nrc_spi_master_init(&bme680_spi);
	nrc_spi_enable(&bme680_spi, true);
	_delay_ms(100);

	while (1)
	{
		nrc_spi_writebyte_value(&bme680_spi, 0x73& BME680_SPI_WR_MSK, 0x00);

		if (nrc_spi_readbyte_value(&bme680_spi, 0x73|BME680_SPI_RD_MSK, &data) != NRC_SUCCESS)
			return -1;

		if (data == 0)	{
			nrc_usr_print("[%s] spi_mem_page changed successfully!!\n", __func__);
			break;
		} else {
			_delay_ms(300);
		}

		if(count++ == 10){
			nrc_usr_print("[%s] spi_mem_page change failed. retry......\n", __func__);
			return -1;
		}
	}

	if (nrc_spi_readbyte_value(&bme680_spi, 0x73|BME680_SPI_RD_MSK, &data) != NRC_SUCCESS)
		return -1;

	nrc_usr_print("[%s] spi_mem_page default value=0x%02x\n", __func__, data);

	return 0;
}

static void delay_ms(uint32_t period)
{
	_delay_ms(period);
}

static int bme680_setting(struct bme680_dev  *sensor)
{
	uint8_t set_required_settings = 0;
	int8_t rslt = 0;

	sensor->dev_id = 0;
	sensor->intf = BME680_SPI_INTF;
	sensor->read = spi_read;
	sensor->write = spi_write;
	sensor->delay_ms = delay_ms;
	sensor->amb_temp = 25;

	rslt = BME680_OK;
	rslt = bme680_init(sensor);
	nrc_usr_print("[%s] result=%d\n", __func__, rslt);

	_delay_ms(2000);

	set_required_settings = 0;

	/* Set the temperature, pressure and humidity settings */
	sensor->tph_sett.os_hum = BME680_OS_2X;
	sensor->tph_sett.os_pres = BME680_OS_4X;
	sensor->tph_sett.os_temp = BME680_OS_8X;
	sensor->tph_sett.filter = BME680_FILTER_SIZE_3;

	/* Set the remaining gas sensor settings and link the heating profile */
	sensor->gas_sett.run_gas = BME680_DISABLE_GAS_MEAS;
	/* Create a ramp heat waveform in 3 steps */
	sensor->gas_sett.heatr_temp = 320; /* degree Celsius */
	sensor->gas_sett.heatr_dur = 150; /* milliseconds */

	/* Select the power mode */
	/* Must be set before writing the sensor configuration */
	sensor->power_mode = BME680_FORCED_MODE;

	/* Set the required sensor settings needed */
	set_required_settings = BME680_OST_SEL | BME680_OSH_SEL | BME680_FILTER_SEL;
	//BME680_OSP_SEL | BME680_GAS_SENSOR_SEL;

	/* Set the desired sensor configuration */
	rslt = bme680_set_sensor_settings(set_required_settings,sensor);

	/* Set the power mode */
	rslt = bme680_set_sensor_mode(sensor);

	return 0;
}

static void connect_to_ap(WIFI_CONFIG *param)
{
	SCAN_RESULTS results;
	int ret;

	nrc_usr_print("[%s] Trying to connect to AP with SSID %s \n",__func__, param->ssid);

	int i = 0;
	int ssid_found =false;

	/* set initial wifi configuration */
	while(1){
		if (wifi_init(param)== WIFI_SUCCESS) {
			break;
		} else {
			nrc_usr_print ("[%s] wifi_init Failed !! \n", __func__);
			_delay_ms(1000);
		}
	}

	/* wait for ip indefinitely */
	param->dhcp_timeout = 0;

	/* find AP */
	while(1){
		if (nrc_wifi_scan(0) == WIFI_SUCCESS){
			if (nrc_wifi_scan_results(0, &results)== WIFI_SUCCESS) {
				/* Find the ssid in scan results */
				for(i=0; i<results.n_result ; i++){
					if((strcmp((char*)param->ssid, (char*)results.result[i].ssid)== 0)
					   && (results.result[i].security == param->security_mode)){
						ssid_found = true;
						break;
					}
				}

				if(ssid_found){
					nrc_usr_print ("[%s] %s is found \n", __func__, param->ssid);
					break;
				}
			}
		} else {
			_delay_ms(1000);
		}
	}

	/* connect to AP */
	while(1) {
		if (wifi_connect(param)== WIFI_SUCCESS) {
			nrc_usr_print ("[%s] connect to %s successfully !! \n", __func__, param->ssid);
			break;
		} else{
			_delay_ms(1000);
		}
	}

	/* check if IP is ready */
	if (nrc_wait_for_ip(0, param->dhcp_timeout) == NRC_FAIL) {
		return;
	}
}

static void disconnect_from_ap()
{
	tWIFI_STATE_ID wifi_state = WIFI_STATE_INIT;

	if (nrc_wifi_get_state(0) == WIFI_STATE_CONNECTED) {
		if (nrc_wifi_disconnect(0, 5000) != WIFI_SUCCESS) {
			nrc_usr_print ("[%s] Fail for Wi-Fi disconnection (results:%d)\n", __func__);
		}
	}
}

static void update_sensor_data()
{
#ifdef USE_BME680_SENSOR_DATA
    if (sensor.power_mode == BME680_FORCED_MODE)
		bme680_set_sensor_mode(&sensor);

	bme680_get_sensor_data(&data, &sensor);
	g_temperature = ((float) data.temperature) / 100.0;
	g_humidity    = ((float) data.humidity)    / 1000.0;
#else
	g_temperature = ((int)g_temperature+1) % 100;
	g_humidity = ((int)g_humidity+1) % 100;
#endif
}

static int sensor_init()
{
	if(spi_init() != 0) {
		nrc_usr_print ("[%s] Fail to init SPI\n", __func__);
		return NRC_FAIL;
	}
	_delay_ms(100);
	if(bme680_setting(&sensor) != 0) {
		nrc_usr_print ("[%s] Fail to init BME680\n", __func__);
		return NRC_FAIL;
	}
	return NRC_SUCCESS;
}

static void mqtt_pub_task(void *voidParam)
{
	static uint32_t index = 1;
	while(1)
	{
		update_sensor_data();
		sprintf(payload, "{\"seq\" : \"%ld\", \"temperature\" : \"%.2f\", \"humidity\" : \"%.2f\"}", index, g_temperature, g_humidity);
		int payload_size = strlen(payload);
		nrc_usr_print("\t*** publish - (payload size=%d)\n", payload_size);
		nrc_usr_print("\t*** published message : %s\n", payload);
		publish(&mqttContext, payload, payload_size);
		index++;
		_delay_ms(5000);
	}
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
WIFI_CONFIG wifi_config;
WIFI_CONFIG* param = &wifi_config;

void user_init(void)
{
	nrc_err_t ret;

	nrc_uart_console_enable(true);

#ifdef USE_BME680_SENSOR_DATA
	if (sensor_init() == NRC_FAIL) {
		nrc_usr_print("[%s] sensor init failed!!!\n", __func__);
		return;
	}
#endif

	memset(param, 0x0, WIFI_CONFIG_SIZE);

	nrc_wifi_set_config(param);

	connect_to_ap(param);

	ret = init_mqtt();
	if (ret != EXIT_SUCCESS) {
		nrc_usr_print("[%s] MQTT connection failed!!!\n", __func__);
		return;
	}

	xTaskCreate(mqtt_pub_task, "mqtt_pub_task", 4096, (void*)param, 5, NULL);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
}
