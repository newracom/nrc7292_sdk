#ifndef __GUI_BMPIMAGE_H
#define __GUI_BMPIMAGE_H

#include "DEV_Config.h"

struct BMP_IMAGE {
	UWORD Width;
	UWORD Height;
	UBYTE Data[0];
};

UBYTE GUI_ReadBmpImage(struct BMP_IMAGE *BmpImage, UWORD Xstart, UWORD Ystart);

struct BMP_IMAGE_2 {
	UWORD Width;
	UWORD Height;
	UBYTE* Data;
};

UBYTE GUI_ReadBmpImage_2(struct BMP_IMAGE_2 *BmpImage, UWORD Xstart, UWORD Ystart);

#endif
