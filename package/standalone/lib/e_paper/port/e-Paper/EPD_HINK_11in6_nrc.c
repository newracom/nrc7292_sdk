/*****************************************************************************
* | File      	:  	EPD_HINK_11in6.c
* | Author      :   Newracom / Newratek
* | Function    :   HINK-E116A07
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2022-07-08
* | Info        :		
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
#include "EPD_HINK_11in6_nrc.h"
#include "Debug.h"

#define EPD_HINK_11IN6_SEND_DATA_BURST

static UDOUBLE ImageSize = EPD_HINK_11IN6_IMAGE_SIZE;

/******************************************************************************
function :	Software reset
parameter:
******************************************************************************/
static void EPD_HINK_11IN6_Reset(void)
{
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(1);
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(1);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(1);
}

/******************************************************************************
function :	send command
parameter:
     Reg : Command register
******************************************************************************/
static void EPD_HINK_11IN6_SendCommand(UBYTE Reg)
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

#if defined(EPD_HINK_11IN6_SEND_DATA_BURST)
static void EPD_HINK_11IN6_SendData(UBYTE *pData, UDOUBLE Len)
{
    DEV_Digital_Write(EPD_DC_PIN, 1);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_Write_nByte(pData, Len);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}
#else
static void EPD_HINK_11IN6_SendData(UBYTE *pData, UDOUBLE Len)
{
	UDOUBLE i;

	for (i = 0 ; i < Len ; i++)
	{
	    DEV_Digital_Write(EPD_DC_PIN, 1);
	    DEV_Digital_Write(EPD_CS_PIN, 0);
    	DEV_SPI_WriteByte(pData[i]);
	    DEV_Digital_Write(EPD_CS_PIN, 1);
	}
}
#endif

/******************************************************************************
function :	Wait until the busy_pin goes LOW
parameter:
******************************************************************************/
static void EPD_HINK_11IN6_ReadBusy(void)
{
    Debug("e-Paper busy\r\n");

	while(DEV_Digital_Read(EPD_BUSY_PIN) == 1) //LOW: idle, HIGH: busy
        DEV_Delay_ms(1);
    
	Debug("e-Paper busy release\r\n");
}

/******************************************************************************
function :	Turn On Display
parameter:
******************************************************************************/
static void EPD_HINK_11IN6_TurnOnDisplay(void)
{
	UBYTE Data;

	Debug("TurnOnDisplay\n");
	
	Data = 0xF7;						/* Load LUT from MCU(0x32) */
	EPD_HINK_11IN6_SendCommand(0x22); 	/* Disply Update Control 2 */
	EPD_HINK_11IN6_SendData(&Data, 1);

	EPD_HINK_11IN6_SendCommand(0x20); // Activate Display Update Sequence

	EPD_HINK_11IN6_ReadBusy();
}

/******************************************************************************
function :	Setting the display window
parameter:
******************************************************************************/
static void EPD_HINK_11IN6_SetWindows(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend)
{
	UBYTE Data[4];

    Data[0] = Xstart & 0xFF;
    Data[1] = (Xstart >> 8) & 0x03;
    Data[2] = Xend & 0xFF;
    Data[3] = (Xend >> 8) & 0x03;

	EPD_HINK_11IN6_SendCommand(0x44); // SET_RAM_X_ADDRESS_START_END_POSITION
    EPD_HINK_11IN6_SendData(Data, 4);

    Data[0] = Ystart & 0xFF;
    Data[1] = (Ystart >> 8) & 0x03;
    Data[2] = Yend & 0xFF;
    Data[3] = (Yend >> 8) & 0x03;

    EPD_HINK_11IN6_SendCommand(0x45); // SET_RAM_Y_ADDRESS_START_END_POSITION
    EPD_HINK_11IN6_SendData(Data, 4);
}

/******************************************************************************
function :	Set Cursor
parameter:
******************************************************************************/
static void EPD_HINK_11IN6_SetCursor(UWORD Xstart, UWORD Ystart)
{
	UBYTE Data[2];

    Data[0] = Xstart & 0xFF;
    Data[1] = (Xstart >> 8) & 0x03;
    EPD_HINK_11IN6_SendCommand(0x4E); // SET_RAM_X_ADDRESS_COUNTER
    EPD_HINK_11IN6_SendData(Data, 2);

    Data[0] = Ystart & 0xFF;
    Data[1] = (Ystart >> 8) & 0x03;
    EPD_HINK_11IN6_SendCommand(0x4F); // SET_RAM_Y_ADDRESS_COUNTER
    EPD_HINK_11IN6_SendData(Data, 2);
}

/******************************************************************************
function :	Initialize the e-Paper register
parameter:
******************************************************************************/
void EPD_HINK_11IN6_Init(void)
{
	UBYTE Data[5];

    EPD_HINK_11IN6_Reset();
    EPD_HINK_11IN6_ReadBusy();

    EPD_HINK_11IN6_SendCommand(0x12);	/* Soft Reset */
    EPD_HINK_11IN6_ReadBusy();

	Data[0] = 0xAE;
	Data[1] = 0xC7;
	Data[2] = 0xC3;
	Data[3] = 0xC0;
	Data[4] = 0x00;
	EPD_HINK_11IN6_SendCommand(0x0C); 	/* Soft Start setting */
	EPD_HINK_11IN6_SendData(Data, 5);
    EPD_HINK_11IN6_ReadBusy();

	Data[0] = 0x7F;						/* set MUX as 639 */
	Data[1] = 0x02;
	Data[2] = 0x00;
	EPD_HINK_11IN6_SendCommand(0x01);	/* Driver Output control */
	EPD_HINK_11IN6_SendData(Data, 3);
    EPD_HINK_11IN6_ReadBusy();

	Data[0] = 0x03;
	EPD_HINK_11IN6_SendCommand(0x11); 	/* Data Entry mode setting */
	EPD_HINK_11IN6_SendData(Data, 1);
    EPD_HINK_11IN6_ReadBusy();

	EPD_HINK_11IN6_SetWindows(0, 0, EPD_HINK_11IN6_WIDTH - 1, EPD_HINK_11IN6_HEIGHT - 1);
	EPD_HINK_11IN6_SetCursor(0, 0);
	
	Data[0] = 0x01; 					/* LUT1, for white */
	EPD_HINK_11IN6_SendCommand(0x3C); 	/* Border Waveform control */
	EPD_HINK_11IN6_SendData(Data, 1);
    EPD_HINK_11IN6_ReadBusy();	
}

/******************************************************************************
function :	Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void EPD_HINK_11IN6_Display(UBYTE *ImageBlack, UBYTE *ImageRed, bool TurnOnDisplay)
{
	if (!ImageBlack && !ImageRed)
		return;

	if (ImageBlack)
	{
		Debug("Display: BW\n");

		EPD_HINK_11IN6_SetCursor(0, 0);
		EPD_HINK_11IN6_SendCommand(0x24);
		EPD_HINK_11IN6_SendData(ImageBlack, ImageSize);
	}
	
	if (ImageRed)
	{
		UDOUBLE i;

		Debug("Display: RED\n");

		for (i = 0 ; i < ImageSize ; i++)
			ImageRed[i] = ~ImageRed[i]; 

		EPD_HINK_11IN6_SetCursor(0, 0);
		EPD_HINK_11IN6_SendCommand(0x26);
		EPD_HINK_11IN6_SendData(ImageRed, ImageSize);
	}

	if (TurnOnDisplay)
	    EPD_HINK_11IN6_TurnOnDisplay();
}

/******************************************************************************
function :	Clear screen
parameter:
******************************************************************************/
void EPD_HINK_11IN6_Clear(void)
{
	UBYTE Data[EPD_HINK_11IN6_WIDTH_SIZE];
	UDOUBLE i;

	for (i = 0 ; i < sizeof(Data) ; i++)
		Data[i] = 0xFF;

	EPD_HINK_11IN6_SendCommand(0x24);
	for (i = 0; i < EPD_HINK_11IN6_HEIGHT ; i++)	
		EPD_HINK_11IN6_SendData(Data, sizeof(Data));

	for (i = 0 ; i < sizeof(Data) ; i++)
		Data[i] = 0x00;
	
	EPD_HINK_11IN6_SendCommand(0x26);
	for (i = 0; i < EPD_HINK_11IN6_HEIGHT ; i++)	
		EPD_HINK_11IN6_SendData(Data, sizeof(Data));

    EPD_HINK_11IN6_TurnOnDisplay();
}

/******************************************************************************
function :	Enter sleep mode
parameter:
******************************************************************************/
void EPD_HINK_11IN6_Sleep(UDOUBLE time)
{
	UBYTE Data = 0x01;

    EPD_HINK_11IN6_SendCommand(0x10);
    EPD_HINK_11IN6_SendData(&Data, 1);

	if (time > 0)
	{
		DEV_Delay_ms(time);
		EPD_HINK_11IN6_Wakeup();
	}
}

/******************************************************************************
function :	Wake up from sleep mode
parameter:
******************************************************************************/
void EPD_HINK_11IN6_Wakeup(void)
{
	EPD_HINK_11IN6_Reset();
}

