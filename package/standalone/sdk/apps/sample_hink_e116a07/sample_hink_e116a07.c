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
#include "e_paper.h"

#include "patient_care_board_b.h"
#include "patient_care_board_r.h"


#define NRC_EPD_BUSY_PIN	0
#define NRC_EPD_RST_PIN		1
#define NRC_EPD_DC_PIN		2
#define NRC_EPD_CS_PIN		3
#define NRC_EPD_SCL_PIN		4
#define NRC_EPD_SDA_PIN		5
#define NRC_EPD_PWR_PIN		8


/******************************************************************************
 * FunctionName : run_sample_hink_e116a07
 * Description  : sample test for HINK-E116A07
 * Parameters   :
 * Returns	    : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/

nrc_err_t run_sample_hink_e116a07(void)
{
	static UBYTE ImageBuffer[EPD_HINK_11IN6_IMAGE_SIZE];
	const UDOUBLE ImageSize = EPD_HINK_11IN6_IMAGE_SIZE;
	struct EPD_IO_PINS IO_Pins;

	nrc_usr_print("HeapSize: %d %d\n", xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
	nrc_usr_print("ImageSize: %u\n", ImageSize);

	IO_Pins.BUSY = NRC_EPD_BUSY_PIN;
	IO_Pins.RST = NRC_EPD_RST_PIN;
	IO_Pins.DC = NRC_EPD_DC_PIN;
	IO_Pins.CS = NRC_EPD_CS_PIN;
	IO_Pins.SCL = NRC_EPD_SCL_PIN;
	IO_Pins.SDA = NRC_EPD_SDA_PIN;
	IO_Pins.PWR = NRC_EPD_PWR_PIN;

	if (NRC_EPD_IO_Enable(&IO_Pins) != 0) {
		nrc_usr_print("NRC_EPD_IO_Enable() failed\n");
		return NRC_FAIL;
	}

	nrc_usr_print("Init\n");
	EPD_HINK_11IN6_Init();

	nrc_usr_print("Clear\n");
	Paint_NewImage(ImageBuffer, EPD_HINK_11IN6_WIDTH, EPD_HINK_11IN6_HEIGHT, 90, WHITE);
	Paint_Clear(WHITE);
	EPD_HINK_11IN6_Clear();
	EPD_HINK_11IN6_Sleep(1000);

#if 1
	nrc_usr_print("Display: Black & Red\n");
	Paint_Clear(BLACK);
	EPD_HINK_11IN6_BW_Display(ImageBuffer, true);
	EPD_HINK_11IN6_Sleep(1000);
	EPD_HINK_11IN6_RED_Display(ImageBuffer, true);
	EPD_HINK_11IN6_Sleep(1000);
#endif
#if 1
	nrc_usr_print("Draw\n");
	Paint_Clear(WHITE);
	Paint_DrawPoint(10, 80, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT);
	Paint_DrawPoint(10, 90, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);
	Paint_DrawPoint(10, 100, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT);
	Paint_DrawPoint(10, 110, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT);
	Paint_DrawLine(20, 70, 70, 120, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
	Paint_DrawLine(70, 70, 20, 120, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
	Paint_DrawRectangle(20, 70, 70, 120, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
	Paint_DrawRectangle(80, 70, 130, 120, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
	Paint_DrawString_EN(10, 0, "waveshare", &Font16, BLACK, WHITE);
	Paint_DrawString_CN(130, 20, "菓汽든綾", &Font24CN, WHITE, BLACK);
	Paint_DrawNum(10, 50, 987654321, &Font16, WHITE, BLACK);
	EPD_HINK_11IN6_BW_Display(ImageBuffer, false);

	Paint_Clear(WHITE);
	Paint_DrawCircle(160, 95, 20, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
	Paint_DrawCircle(210, 95, 20, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
	Paint_DrawLine(85, 95, 125, 95, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
	Paint_DrawLine(105, 75, 105, 115, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
	Paint_DrawString_CN(130, 0,"콱봤abc", &Font12CN, BLACK, WHITE);
	Paint_DrawString_EN(10, 20, "hello world", &Font12, WHITE, BLACK);
	Paint_DrawNum(10, 33, 123456789, &Font12, BLACK, WHITE);
	EPD_HINK_11IN6_RED_Display(ImageBuffer, true);
	EPD_HINK_11IN6_Sleep(1000);
#endif

	nrc_usr_print("Paint\n");
	Paint_Clear(WHITE);
#if 1 // Display a black image
	GUI_ReadBmpImage((struct BMP_IMAGE *)patient_care_board_b, 0, 0);
	EPD_HINK_11IN6_BW_Display(ImageBuffer, false);
#else // Display a red image
	GUI_ReadBmpImage((struct BMP_IMAGE *)patient_care_board_r, 0, 0);
	EPD_HINK_11IN6_RED_Display(ImageBuffer, true);
#endif
	EPD_HINK_11IN6_Sleep(0);

	while (0)
	{
		nrc_usr_print(".");
		_delay_ms(1000);
	}

	nrc_usr_print("Exit\n");
	EPD_HINK_11IN6_Wakeup();
	EPD_HINK_11IN6_Clear();
	EPD_HINK_11IN6_Sleep(0);

	NRC_EPD_IO_Disable();

	return NRC_SUCCESS;
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns	    : none
 *******************************************************************************/
void user_init(void)
{
	run_sample_hink_e116a07();
}
