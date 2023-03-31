/*****************************************************************************
* | File      	:  	EPD_HINK_11in6.h
* | Author      :   Newracom / Newratek
* | Function    :   HINK-E116A07, HOLITECH
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
#ifndef __EPD_HINK_11IN6_H_
#define __EPD_HINK_11IN6_H_

#include "DEV_Config.h"

// Display resolution
#define EPD_HINK_11IN6_WIDTH       	960
#define EPD_HINK_11IN6_HEIGHT      	640

#define EPD_HINK_11IN6_WIDTH_SIZE	(EPD_HINK_11IN6_WIDTH / 8)
#define EPD_HINK_11IN6_IMAGE_SIZE	(EPD_HINK_11IN6_WIDTH_SIZE * EPD_HINK_11IN6_HEIGHT)

struct EPD_HINK_11IN6_BMP
{
	UWORD Width;
	UWORD Height;
	UBYTE Image[EPD_HINK_11IN6_IMAGE_SIZE];
};

void EPD_HINK_11IN6_Init(void);
void EPD_HINK_11IN6_Display(UBYTE *ImageBlack, UBYTE *ImageRed, bool TurnOnDisplay);
void EPD_HINK_11IN6_Clear(void);
void EPD_HINK_11IN6_Sleep(UDOUBLE time);
void EPD_HINK_11IN6_Wakeup(void);

#define EPD_HINK_11IN6_BW_Display(ImageBlack, TurnOnDisplay)	EPD_HINK_11IN6_Display(ImageBlack, NULL, TurnOnDisplay)
#define EPD_HINK_11IN6_RED_Display(ImageRed, TurnOnDisplay)		EPD_HINK_11IN6_Display(NULL, ImageRed, TurnOnDisplay)
#endif
