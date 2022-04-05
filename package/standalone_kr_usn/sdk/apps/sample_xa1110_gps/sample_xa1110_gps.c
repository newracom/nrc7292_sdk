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
#include "sample_xa1110_gps.h"

i2c_device_t xa1110_i2c;

void gps_i2c_set_config(i2c_device_t* i2c)
{
	i2c->pin_sda = GPS_I2C_SDA;
	i2c->pin_scl = GPS_I2C_SCL;
	i2c->clock_source =GPS_I2C_CLOCK_SOURCE;
	i2c->controller = GPS_I2C_CHANNEL;
	i2c->clock = GPS_I2C_CLOCK;
	i2c->width = GPS_I2C_DATA_WIDTH;
	i2c->address = GPS_I2C_ADDRESS;
}

static void gps_init(i2c_device_t* i2c)
{
	nrc_i2c_init(i2c);
	nrc_i2c_enable(i2c, true);
}

static int gps_read_byte(i2c_device_t* i2c)
{
	uint8_t value;
	nrc_i2c_start(i2c);
	nrc_i2c_writebyte(i2c, (GPS_I2C_ADDRESS<<1)|0x01);
	nrc_i2c_readbyte(i2c, &value, false);
	nrc_i2c_stop(i2c);
	_delay_ms(1);
	return value;
}

static void gps_handle_headered_line(char *s)
{
	nrc_usr_print("%s\n", s);
}

static void gps_handle_parsed_field(char *header, int field_index, char *data)
{
#if GPS_HANDLE_PARSED_FIELD_ENABLED
	nrc_usr_print("[%s-%02d:%s]\n", header, field_index, data);
#endif
}

static void gps_process_byte(int b)
{
	if (b == '\n') {
		return;
	} else if (b == '\r') {
		gps_line_buf[gps_line_buf_index] = '\0';
		gps_process_line(gps_line_buf);
		gps_line_buf[0] = '\0';
		gps_line_buf_index = 0;
	} else if (gps_line_buf_index < GPS_LINE_BUF_SIZE) {
		gps_line_buf[gps_line_buf_index++] = b;
	}
}

static void gps_process_line(char *s)
{
	if(*s != '$')
		return;
	int checksum = 0;
	char *x = s + 1;
	char c;
	while ((c = *x++) != '\0' && c != '*')
		checksum ^= c;
	if (c != '*')
		return;
	if (*x == '\0' || *(x+1) == '\0')
		return;
	if (checksum != strtol(x, '\0', 16))
		return;
	if (1
#ifdef GPS_SINGLE_TARGET_HEADER
		&& strncmp(GPS_SINGLE_TARGET_HEADER, s, strlen(GPS_SINGLE_TARGET_HEADER)) == 0
#endif
	) {
		gps_handle_headered_line(s);
		gps_parse_csv_fields(s);
	}
}

static void gps_parse_csv_fields(char *s)
{
	char c;
	int field_index = 1;
	char *header = s;
	char *field_buf_start = s;
	int i = 0;
	while(1)
	{
		c = s[i];
		if (c == ',' || c == '\0') {
			if (c == ',')
				s[i] = '\0';
			gps_handle_parsed_field(header, field_index++, field_buf_start);
			if(c == '\0')
				return;
			field_buf_start = &s[i+1];
		}
		i++;
	}
}

/******************************************************************************
 * FunctionName : run_sample_i2c
 * Description  : sample test for i2c
 * Parameters   : count(test count), interval(test interval)
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_i2c()
{
	int i = 0;
	nrc_usr_print("[%s] Sample App for I2C \n", __func__);

	gps_i2c_set_config(&xa1110_i2c);
	gps_init(&xa1110_i2c);
	while(1)
		gps_process_byte(gps_read_byte(&xa1110_i2c));
	return NRC_SUCCESS;
}


/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void user_init(void)
{
	nrc_err_t ret;
	nrc_uart_console_enable(1);
	ret = run_sample_i2c();
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
}
