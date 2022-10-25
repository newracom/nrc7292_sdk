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

#define GPS_USE_RPI_PINS 0 // 0: Standalone Mode, 1: Raspberry Pi Test Mode

#define GPS_I2C_CHANNEL I2C_MASTER_0
#define GPS_I2C_ADDRESS 0x10
#define GPS_I2C_SCL     16
#define GPS_I2C_SDA     17
#define GPS_I2C_CLOCK   400000
#define GPS_I2C_CLOCK_SOURCE 0 /* 0:Clock controller 1:PCLK */
#define GPS_I2C_DATA_WIDTH I2C_8BIT

#define GPS_LINE_BUF_SIZE 255

//#define GPS_HANDLE_PARSED_FIELD_ENABLED 1
//#define GPS_SINGLE_TARGET_HEADER  "$GNGGA"  // Comment this out to handle all headers.

static void gps_init(i2c_device_t* i2c);
static int gps_read_byte(i2c_device_t* i2c);
static void gps_handle_headered_line(char *s);
static void gps_handle_parsed_field(char *header, int field_index, char *data);

static void gps_process_byte(int b);
static void gps_process_line(char *s);
static void gps_parse_csv_fields(char *s);

static int  gps_line_buf_index;
static char gps_line_buf[GPS_LINE_BUF_SIZE+1];
