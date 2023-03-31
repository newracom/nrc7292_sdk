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

#include "2in66b_b.h"
#include "2in66b_r.h"

#define NRC_EPD_BUSY_PIN	0
#define NRC_EPD_RST_PIN		1
#define NRC_EPD_DC_PIN		2
#define NRC_EPD_CS_PIN		3
#define NRC_EPD_SCL_PIN		4
#define NRC_EPD_SDA_PIN		5
#define NRC_EPD_PWR_PIN		8


/******************************************************************************
 * FunctionName : run_sample_epd_2in66b
 * Description  : sample test for epd_2in66b
 * Parameters   :
 * Returns	    : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_epd_2in66b(void)
{
	struct EPD_IO_PINS IO_Pins;
	UBYTE *BlackImage = NULL;
	UBYTE *RedImage = NULL;
	UWORD ImageSize = 0;
	int ret = NRC_FAIL;
	int i;

	IO_Pins.BUSY = NRC_EPD_BUSY_PIN;
	IO_Pins.RST = NRC_EPD_RST_PIN;
	IO_Pins.DC = NRC_EPD_DC_PIN;
	IO_Pins.CS = NRC_EPD_CS_PIN;
	IO_Pins.SCL = NRC_EPD_SCL_PIN;
	IO_Pins.SDA = NRC_EPD_SDA_PIN;
	IO_Pins.PWR = NRC_EPD_PWR_PIN;

	nrc_usr_print("\n");
	nrc_usr_print("========================================================\n");
	nrc_usr_print("                e-Paper EPD_2IN66B TEST                 \n");
	nrc_usr_print("========================================================\n");

	ImageSize = EPD_2IN66B_ImageSize();
	BlackImage = (UBYTE *)nrc_mem_malloc(ImageSize);
	RedImage = (UBYTE *)nrc_mem_malloc(ImageSize);

	if (!BlackImage || !RedImage) {
		nrc_usr_print("malloc() failed. (BlackImage=%p, RedImage=%p)\n", BlackImage, RedImage);
		goto exit_sample;
	}

	Paint_NewImage(BlackImage, EPD_2IN66B_WIDTH, EPD_2IN66B_HEIGHT, 270, WHITE);
	Paint_Clear(WHITE);

	Paint_NewImage(RedImage, EPD_2IN66B_WIDTH, EPD_2IN66B_HEIGHT, 270, WHITE);
	Paint_Clear(WHITE);

	nrc_usr_print("\n[1] Init\n");

	if (NRC_EPD_IO_Enable(&IO_Pins) != 0) {
		nrc_usr_print("NRC_EPD_IO_Enable() failed\n");
		goto exit_sample;
	}

	EPD_2IN66B_Init();
	EPD_2IN66B_Clear();

	nrc_usr_print("\n[2] Show\n");

	Paint_SelectImage(BlackImage);
	GUI_ReadBmpImage((struct BMP_IMAGE *)bmp_2in66b_b, 0, 0);
	Paint_SelectImage(RedImage);
	GUI_ReadBmpImage((struct BMP_IMAGE *)bmp_2in66b_r, 0, 0);

	EPD_2IN66B_Display(BlackImage, RedImage);
	EPD_2IN66B_Sleep();

	for (i = 1 ; i <= 5 ; i++) {
		_delay_ms(1000);
		nrc_usr_print("   %d sec\n", i);
	}

	nrc_usr_print("\n[3] Draw\n");

	Paint_SelectImage(BlackImage);
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

	Paint_SelectImage(RedImage);
	Paint_Clear(WHITE);
	Paint_DrawCircle(160, 95, 20, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
	Paint_DrawCircle(210, 95, 20, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
	Paint_DrawLine(85, 95, 125, 95, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
	Paint_DrawLine(105, 75, 105, 115, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
	Paint_DrawString_CN(130, 0,"콱봤abc", &Font12CN, BLACK, WHITE);
	Paint_DrawString_EN(10, 20, "hello world", &Font12, WHITE, BLACK);
	Paint_DrawNum(10, 33, 123456789, &Font12, BLACK, WHITE);

	EPD_2IN66B_Wakeup();
	EPD_2IN66B_Display(BlackImage, RedImage);
	EPD_2IN66B_Sleep();

	for (i = 1 ; i <= 5 ; i++) {
		_delay_ms(1000);
		nrc_usr_print("   %d sec\n", i);
	}

	nrc_usr_print("\n[4] Exit\n");

	EPD_2IN66B_Wakeup();
	EPD_2IN66B_Clear();
	NRC_EPD_IO_Disable();

	nrc_usr_print("\n[5] Done\n\n");

	ret = NRC_SUCCESS;

exit_sample:

	if (BlackImage)
		nrc_mem_free(BlackImage);

	if (RedImage)
		nrc_mem_free(RedImage);

	BlackImage = NULL;
	RedImage = NULL;

	return ret;
}


/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns	    : none
 *******************************************************************************/
void user_init(void)
{
	run_sample_epd_2in66b();
}
