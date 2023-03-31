/*****************************************************************************
* | File      	:  	EPD_2in66b.c
* | Author      :   Waveshare team
* | Function    :   2.66inch e-paper b
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2020-11-13
* | Info        :
*----------------
* |	This version:   V1.1
* | Date        :   2021-11-10
* | Info        :	Burst Data Write, Newracom
* -----------------------------------------------------------------------------
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#include "EPD_2in66b_nrc.h"
#include "Debug.h"

#define EPD_2IN66B_SEND_DATA_BURST

static UWORD Width = (EPD_2IN66B_WIDTH % 8 == 0)? (EPD_2IN66B_WIDTH / 8 ): (EPD_2IN66B_WIDTH / 8 + 1);
static UWORD Height = EPD_2IN66B_HEIGHT;

/******************************************************************************
function :	Software reset
parameter:
******************************************************************************/
static void EPD_2IN66B_Reset(void)
{
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(200);
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(2);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(200);
}

/******************************************************************************
function :	send command
parameter:
     Reg : Command register
******************************************************************************/
static void EPD_2IN66B_SendCommand(UBYTE Reg)
{
    DEV_Digital_Write(EPD_DC_PIN, 0);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(Reg);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	send data
parameter:
    Data : Write data
******************************************************************************/

#if defined(EPD_2IN66B_SEND_DATA_BURST)
static void EPD_2IN66B_SendData(UBYTE *pData, UWORD Len)
{
    DEV_Digital_Write(EPD_DC_PIN, 1);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_Write_nByte(pData, Len);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}
#else
static void EPD_2IN66B_SendData(UBYTE Data)
{
    DEV_Digital_Write(EPD_DC_PIN, 1);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(Data);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}
#endif

/******************************************************************************
function :	Wait until the busy_pin goes LOW
parameter:
******************************************************************************/
static void EPD_2IN66B_ReadBusy(void)
{
    Debug("e-Paper busy\r\n");
    DEV_Delay_ms(50);
    while(DEV_Digital_Read(EPD_BUSY_PIN) == 1) {      //LOW: idle, HIGH: busy
        DEV_Delay_ms(10);
    }
    DEV_Delay_ms(50);
    Debug("e-Paper busy release\r\n");
}

/******************************************************************************
function :	Turn On Display
parameter:
******************************************************************************/
static void EPD_2IN66B_TurnOnDisplay(void)
{
    EPD_2IN66B_SendCommand(0x20);
    EPD_2IN66B_ReadBusy();
}

/******************************************************************************
function :	Setting the display window
parameter:
******************************************************************************/
#if defined(EPD_2IN66B_SEND_DATA_BURST)
static void EPD_2IN66B_SetWindows(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend)
{
	UBYTE Data[4];

    Data[0] = (Xstart>>3) & 0x1F;
    Data[1] = (Xend>>3) & 0x1F;
    EPD_2IN66B_SendCommand(0x44); // SET_RAM_X_ADDRESS_START_END_POSITION
    EPD_2IN66B_SendData(Data, 2);

    Data[0] = Ystart & 0xFF;
    Data[1] = (Ystart >> 8) & 0x01;
    Data[2] = Yend & 0xFF;
    Data[3] = (Yend >> 8) & 0x01;
    EPD_2IN66B_SendCommand(0x45); // SET_RAM_Y_ADDRESS_START_END_POSITION
    EPD_2IN66B_SendData(Data, 4);
}
#else
static void EPD_2IN66B_SetWindows(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend)
{
    EPD_2IN66B_SendCommand(0x44); // SET_RAM_X_ADDRESS_START_END_POSITION
    EPD_2IN66B_SendData((Xstart>>3) & 0x1F);
    EPD_2IN66B_SendData((Xend>>3) & 0x1F);

    EPD_2IN66B_SendCommand(0x45); // SET_RAM_Y_ADDRESS_START_END_POSITION
    EPD_2IN66B_SendData(Ystart & 0xFF);
    EPD_2IN66B_SendData((Ystart >> 8) & 0x01);
    EPD_2IN66B_SendData(Yend & 0xFF);
    EPD_2IN66B_SendData((Yend >> 8) & 0x01);
}
#endif

/******************************************************************************
function :	Set Cursor
parameter:
******************************************************************************/
#if defined(EPD_2IN66B_SEND_DATA_BURST)
static void EPD_2IN66B_SetCursor(UWORD Xstart, UWORD Ystart)
{
	UBYTE Data[2];

    Data[0] = Xstart & 0x1F;
    EPD_2IN66B_SendCommand(0x4E); // SET_RAM_X_ADDRESS_COUNTER
    EPD_2IN66B_SendData(Data, 1);

    Data[0] = Ystart & 0xFF;
    Data[1] = (Ystart >> 8) & 0x01;
    EPD_2IN66B_SendCommand(0x4F); // SET_RAM_Y_ADDRESS_COUNTER
    EPD_2IN66B_SendData(Data, 2);
}
#else
static void EPD_2IN66B_SetCursor(UWORD Xstart, UWORD Ystart)
{
    EPD_2IN66B_SendCommand(0x4E); // SET_RAM_X_ADDRESS_COUNTER
    EPD_2IN66B_SendData(Xstart & 0x1F);

    EPD_2IN66B_SendCommand(0x4F); // SET_RAM_Y_ADDRESS_COUNTER
    EPD_2IN66B_SendData(Ystart & 0xFF);
    EPD_2IN66B_SendData((Ystart >> 8) & 0x01);
}
#endif

/******************************************************************************
function :	Initialize the e-Paper register
parameter:
******************************************************************************/
#if defined(EPD_2IN66B_SEND_DATA_BURST)
void EPD_2IN66B_Init(void)
{
	UBYTE Data[2];

    EPD_2IN66B_Reset();
    EPD_2IN66B_ReadBusy();
    EPD_2IN66B_SendCommand(0x12);//soft  reset
    EPD_2IN66B_ReadBusy();

	Data[0] = 0x03;
	EPD_2IN66B_SendCommand(0x11); //data entry mode
	EPD_2IN66B_SendData(Data, 1);

	EPD_2IN66B_SetWindows(0, 0, EPD_2IN66B_WIDTH-1, EPD_2IN66B_HEIGHT-1);

	Data[0] = 0x00;
	Data[1] = 0x80;
	EPD_2IN66B_SendCommand(0x21); //  Display update control
	EPD_2IN66B_SendData(Data, 2);

	EPD_2IN66B_SetCursor(0, 0);
	EPD_2IN66B_ReadBusy();
}
#else
void EPD_2IN66B_Init(void)
{
    EPD_2IN66B_Reset();
    EPD_2IN66B_ReadBusy();
    EPD_2IN66B_SendCommand(0x12);//soft  reset
    EPD_2IN66B_ReadBusy();

	EPD_2IN66B_SendCommand(0x11); //data entry mode
	EPD_2IN66B_SendData(0x03);

	EPD_2IN66B_SetWindows(0, 0, EPD_2IN66B_WIDTH-1, EPD_2IN66B_HEIGHT-1);

	EPD_2IN66B_SendCommand(0x21); //  Display update control
	EPD_2IN66B_SendData(0x00);
	EPD_2IN66B_SendData(0x80);

	EPD_2IN66B_SetCursor(0, 0);
	EPD_2IN66B_ReadBusy();
}
#endif

/******************************************************************************
function :	Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
#if defined(EPD_2IN66B_SEND_DATA_BURST)
void EPD_2IN66B_Display(UBYTE *ImageBlack, UBYTE *ImageRed)
{
	UWORD i;

	if (!ImageBlack || !ImageRed)
		return;

	for (i = 0 ; i < (Height * Width) ; i++)
		ImageRed[i] = ~ImageRed[i];

    EPD_2IN66B_SendCommand(0x24);
	EPD_2IN66B_SendData(ImageBlack, Height * Width);

    EPD_2IN66B_SendCommand(0x26);
	EPD_2IN66B_SendData(ImageRed, Height * Width);

    EPD_2IN66B_TurnOnDisplay();
}
#else
void EPD_2IN66B_Display(UBYTE *ImageBlack, UBYTE *ImageRed)
{
	UWORD i, j;

	if (!ImageBlack || !ImageRed)
		return;

    EPD_2IN66B_SendCommand(0x24);
    for (j = 0; j < Height; j++) {
        for (i = 0; i < Width; i++) {
            EPD_2IN66B_SendData(ImageBlack[i + j * Width]);
        }
    }

    EPD_2IN66B_SendCommand(0x26);
    for (j = 0; j < Height; j++) {
        for (i = 0; i < Width; i++) {
            EPD_2IN66B_SendData(~ImageRed[i + j * Width]);
        }
    }

    EPD_2IN66B_TurnOnDisplay();
}
#endif

/******************************************************************************
function :	Clear screen
parameter:
******************************************************************************/
#if defined(EPD_2IN66B_SEND_DATA_BURST)
void EPD_2IN66B_Clear(void)
{
	UBYTE *pData = (UBYTE *)malloc(Width);
	UWORD i, j;

	if (pData)
	{
		memset(pData, 0xff, Width);
		for (i = 0 ; i < Height ; i++) {
	    	EPD_2IN66B_SendCommand(0x24);
			EPD_2IN66B_SendData(pData, Width);
		}

		memset(pData, 0x00, Width);
		for (i = 0 ; i < Height ; i++) {
	    	EPD_2IN66B_SendCommand(0x26);
			EPD_2IN66B_SendData(pData, Width);
		}

		free(pData);
	}
	else
	{
		UBYTE Data;

		Data = 0xff;
		EPD_2IN66B_SendCommand(0x24);
		for (j = 0; j < Height; j++) {
			for (i = 0; i < Width; i++) {
				EPD_2IN66B_SendData(&Data, 1);
			}
		}

		Data = 0x00;
		EPD_2IN66B_SendCommand(0x26);
		for (j = 0; j < Height; j++) {
			for (i = 0; i < Width; i++) {
				EPD_2IN66B_SendData(&Data, 1);
			}
		}
	}

    EPD_2IN66B_TurnOnDisplay();
}
#else
void EPD_2IN66B_Clear(void)
{
	UWORD i, j;

    EPD_2IN66B_SendCommand(0x24);
    for (j = 0; j < Height; j++) {
        for (i = 0; i < Width; i++) {
            EPD_2IN66B_SendData(0xff);
        }
    }
	EPD_2IN66B_SendCommand(0x26);
    for (j = 0; j < Height; j++) {
        for (i = 0; i < Width; i++) {
            EPD_2IN66B_SendData(0x00);
        }
    }
    EPD_2IN66B_TurnOnDisplay();
}
#endif

/******************************************************************************
function :	Enter sleep mode
parameter:
******************************************************************************/
#if defined(EPD_2IN66B_SEND_DATA_BURST)
void EPD_2IN66B_Sleep(void)
{
	UBYTE Data = 0x01;
    EPD_2IN66B_SendCommand(0x10);
    EPD_2IN66B_SendData(&Data, 1);
}
#else
void EPD_2IN66B_Sleep(void)
{
    EPD_2IN66B_SendCommand(0x10);
    EPD_2IN66B_SendData(0x01);
}
#endif

/******************************************************************************
function :	Wake up from sleep mode
parameter:
******************************************************************************/
void EPD_2IN66B_Wakeup(void)
{
	EPD_2IN66B_Reset();
}

/******************************************************************************
function :	Get image size
parameter:
******************************************************************************/
UWORD EPD_2IN66B_ImageSize(void)
{
	return Width * Height;
}

