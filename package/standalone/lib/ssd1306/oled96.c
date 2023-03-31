// OLED SSD1306 using the I2C interface
// Written by Larry Bank (bitbank@pobox.com)
// Copyright (c) 2017 BitBank Software, Inc.
// Project started 1/15/2017
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//===========================================================================

// The I2C writes (through a file handle) can be single or multiple bytes.
// The write mode stays in effect throughout each call to write()
// To write commands to the OLED controller, start a byte sequence with 0x00,
// to write data, start a byte sequence with 0x40,
// The OLED controller is set to "page mode". This divides the display
// into 8 128x8 "pages" or strips. Each data write advances the output
// automatically to the next address. The bytes are arranged such that the LSB
// is the topmost pixel and the MSB is the bottom.
// The font data comes from another source and must be rotated 90 degrees
// (at init time) to match the orientation of the bits on the display memory.
// A copy of the display memory is maintained by this code so that single pixel
// writes can occur without having to read from the display controller.

#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "oled96.h"
#include "nrc_types.h"
#include "api_i2c.h"
#include "nrc_types.h"
#include "FreeRTOS.h"
#include "task.h"

extern unsigned char ucFont[], ucSmallFont[];
static int iScreenOffset; // current write offset of screen data
static unsigned char ucScreen[1024]; // local copy of the image buffer
static i2c_device_t* ssd1360_i2c;
static int oled_type, oled_flip;

static void oledWriteCommand(unsigned char);
//
// Opens a file system handle to the I2C device
// Initializes the OLED controller into "page mode"
// Prepares the font data for the orientation of the display
// Returns 0 for success, 1 for failure
//
#define _delay_ms(x)   vTaskDelay(pdMS_TO_TICKS(x))

static bool i2c_write(i2c_device_t* i2c, const uint8_t* data, uint32_t size)
{
	nrc_i2c_start(i2c);
	nrc_i2c_writebyte(i2c, ((i2c->address)<<1) | 0x00);
	for(int i=0;i<size;i++)
		nrc_i2c_writebyte(i2c, *data++);
	nrc_i2c_stop(i2c);
	_delay_ms(1);
	return true;
}

int oledInit(uint8_t sda, uint8_t scl, int iChannel, int iAddr, int iType, int bFlip, int bInvert)
{
	const unsigned char oled64_initbuf[]={0x00,0xae,0xa8,0x3f,0xd3,0x00,0x40,0xa1,0xc8,
		0xda,0x12,0x81,0xff,0xa4,0xa6,0xd5,0x80,0x8d,0x14,
		0xaf,0x20,0x02};
	const unsigned char oled32_initbuf[] = {
		0x00,0xae,0xd5,0x80,0xa8,0x1f,0xd3,0x00,0x40,0x8d,0x14,0xa1,0xc8,0xda,0x02,
		0x81,0x7f,0xd9,0xf1,0xdb,0x40,0xa4,0xa6,0xaf};

	char filename[32];
	unsigned char uc[4];

	oled_type = iType;
	oled_flip = bFlip;

	ssd1360_i2c = pvPortMalloc(sizeof(i2c_device_t));

	/* Set BME680 i2c */
	ssd1360_i2c->pin_sda = sda;
	ssd1360_i2c->pin_scl = scl;
	ssd1360_i2c->clock_source =0;
	ssd1360_i2c->controller = iChannel;
	ssd1360_i2c->clock = 400000;
	ssd1360_i2c->width = I2C_WIDTH_8BIT;
	ssd1360_i2c->address = 0x3c;

	nrc_i2c_init(ssd1360_i2c);
	nrc_i2c_enable(ssd1360_i2c, true);

	if (iType == OLED_128x32)
	{
		i2c_write(ssd1360_i2c, oled32_initbuf, sizeof(oled32_initbuf));
	}
	else
	{
		i2c_write(ssd1360_i2c, oled64_initbuf, sizeof(oled64_initbuf));
	}
	if (bInvert)
	{
		uc[0] = 0; // command
		uc[1] = 0xa7; // invert command
		i2c_write(ssd1360_i2c, uc, 2);
	}
	if (bFlip) // rotate display 180
	{
		uc[0] = 0; // command
		uc[1] = 0xa0;
		i2c_write(ssd1360_i2c, uc, 2);
		uc[1] = 0xc0;
		i2c_write(ssd1360_i2c, uc, 2);
	}
	return 0;
} /* oledInit() */

// Sends a command to turn off the OLED display
// Closes the I2C file handle
void oledShutdown()
{
	if (ssd1360_i2c != 0)
	{
		oledWriteCommand(0xaE); // turn off OLED
		vPortFree(ssd1360_i2c);
		ssd1360_i2c = 0;
	}
}

// Send a single byte command to the OLED controller
static void oledWriteCommand(unsigned char c)
{
	unsigned char buf[2];
	int rc;

	buf[0] = 0x00; // command introducer
	buf[1] = c;
	rc = i2c_write(ssd1360_i2c, buf, 2);
	if (rc) {} // suppress warning
} /* oledWriteCommand() */

static void oledWriteCommand2(unsigned char c, unsigned char d)
{
	unsigned char buf[3];
	int rc;

	buf[0] = 0x00;
	buf[1] = c;
	buf[2] = d;
	rc = i2c_write(ssd1360_i2c, buf, 3);
	if (rc) {} // suppress warning
} /* oledWriteCommand2() */

int oledSetContrast(unsigned char ucContrast)
{
	if (ssd1360_i2c == 0)
		return -1;

	oledWriteCommand2(0x81, ucContrast);
	return 0;
} /* oledSetContrast() */

//static void oledWriteData(unsigned char c)
//{
//unsigned char buf[2];
//        buf[0] = 0x40; // data introducer
//        buf[1] = c;
//        i2c_write(ssd1360_i2c, buf, 2);
//} /* oledWriteData() */

// Send commands to position the "cursor" to the given
// row and column
static void oledSetPosition(int x, int y)
{
	iScreenOffset = (y*128)+x;
	if (oled_type == OLED_64x32) // visible display starts at column 32, row 4
	{
		x += 32; // display is centered in VRAM, so this is always true
		if (oled_flip == 0) // non-flipped display starts from line 4
		y += 4;
	}
	else if (oled_type == OLED_132x64) // SH1106 has 128 pixels centered in 132
	{
		x += 2;
	}

	oledWriteCommand(0xb0 | y); // go to page Y
	oledWriteCommand(0x00 | (x & 0xf)); // // lower col addr
	oledWriteCommand(0x10 | ((x >> 4) & 0xf)); // upper col addr
}

//static void oledWrite(unsigned char c)
//{
//	i2c_write(ssd1360_i2c, &c, 1);
//} /* oledWrite() */

// Write a block of pixel data to the OLED
// Length can be anything from 1 to 1024 (whole display)
static void oledWriteDataBlock(unsigned char *ucBuf, int iLen)
{
	unsigned char ucTemp[129];
	int rc;

	ucTemp[0] = 0x40; // data command
	memcpy(&ucTemp[1], ucBuf, iLen);
	rc = i2c_write(ssd1360_i2c, ucTemp, iLen+1);
	if (rc) {} // suppress warning
	// Keep a copy in local buffer
	memcpy(&ucScreen[iScreenOffset], ucBuf, iLen);
	iScreenOffset += iLen;
}

// Set (or clear) an individual pixel
// The local copy of the frame buffer is used to avoid
// reading data from the display controller
int oledSetPixel(int x, int y, unsigned char ucColor)
{
int i;
unsigned char uc, ucOld;

	if (ssd1360_i2c == 0)
		return -1;

	i = ((y >> 3) * 128) + x;
	if (i < 0 || i > 1023) // off the screen
		return -1;
	uc = ucOld = ucScreen[i];
	uc &= ~(0x1 << (y & 7));
	if (ucColor)
	{
		uc |= (0x1 << (y & 7));
	}
	if (uc != ucOld) // pixel changed
	{
		oledSetPosition(x, y>>3);
		oledWriteDataBlock(&uc, 1);
	}
	return 0;
} /* oledSetPixel() */
//
// Draw a string of small (8x8), large (16x24), or very small (6x8)  characters
// At the given col+row
// The X position is in character widths (8 or 16)
// The Y position is in memory pages (8 lines each)
//
int oledWriteString(int x, int y, char *szMsg, int iSize)
{
int i, iLen;
unsigned char *s;

	if (ssd1360_i2c == 0) return -1; // not initialized
	if (iSize < FONT_NORMAL || iSize > FONT_SMALL)
		return -1;

	iLen = strlen(szMsg);
	if (iSize == FONT_BIG) // draw 16x32 font
	{
		if (iLen+x > 8) iLen = 8-x;
		if (iLen < 0) return -1;
		x *= 16;
		for (i=0; i<iLen; i++)
		{
			s = &ucFont[9728 + (unsigned char)szMsg[i]*64];
			oledSetPosition(x+(i*16), y);
			oledWriteDataBlock(s, 16);
			oledSetPosition(x+(i*16), y+1);
			oledWriteDataBlock(s+16, 16);
			oledSetPosition(x+(i*16), y+2);
			oledWriteDataBlock(s+32, 16);
//			oledSetPosition(x+(i*16), y+3);
//			oledWriteDataBlock(s+48, 16);
		}
	}
	else if (iSize == FONT_NORMAL) // draw 8x8 font
	{
		oledSetPosition(x*8, y);
		if (iLen + x > 16) iLen = 16 - x; // can't display it
		if (iLen < 0)return -1;

		for (i=0; i<iLen; i++)
		{
			s = &ucFont[(unsigned char)szMsg[i] * 8];
			oledWriteDataBlock(s, 8); // write character pattern
		}
	}
	else // 6x8
	{
		oledSetPosition(x*6, y);
		if (iLen + x > 21) iLen = 21 - x;
		if (iLen < 0) return -1;
		for (i=0; i<iLen; i++)
		{
			s = &ucSmallFont[(unsigned char)szMsg[i]*6];
			oledWriteDataBlock(s, 6);
		}
	}
	return 0;
} /* oledWriteString() */

// Fill the frame buffer with a byte pattern
// e.g. all off (0x00) or all on (0xff)
int oledFill(unsigned char ucData)
{
	int y;
	unsigned char temp[128];
	int iLines, iCols;

	if (ssd1360_i2c == 0) return -1; // not initialized

	iLines = (oled_type == OLED_128x32 || oled_type == OLED_64x32) ? 4:8;
	iCols = (oled_type == OLED_64x32) ? 4:8;

	memset(temp, ucData, 128);
	for (y=0; y<iLines; y++)
	{
		oledSetPosition(0,y); // set to (0,Y)
		oledWriteDataBlock(temp, iCols*16); // fill with data byte
	} // for y
	return 0;
} /* oledFill() */

//
// Write a block of flash memory to the display
//
void oledWriteFlashBlock(uint8_t *s, int iLen)
{
	int y;
	uint8_t temp[128];
	int iLines, iCols;
	uint32_t idx = 0;
	int remain = iLen;

	iLines = (oled_type == OLED_128x32 || oled_type == OLED_64x32) ? 4:8;
	iCols = (oled_type == OLED_64x32) ? 4:8;

	for (y=0; y<iLines; y++) {
		memcpy(temp, (uint8_t*)(s + idx),(remain>=128)?128:remain);
		oledSetPosition(0,y);
		oledWriteDataBlock(temp, iCols*16);
		idx += 128;
		remain -= 128;
		if (remain <= 0)
			break;
	}
}

// Printf in OLED
void oledPrintf(int x, int y,  int Fontsize, char *format, ...)
{
	const int oled_str_size = 42;
	char oled_str[oled_str_size];
	va_list ap;
	char *p, *sval;
	int ival;
	double dval;
	memset(oled_str, ' ', oled_str_size);

	va_start(ap, format);
	vsnprintf (oled_str,oled_str_size,format, ap);
	va_end(ap);
	oledWriteString(x,y,oled_str,Fontsize);
}

