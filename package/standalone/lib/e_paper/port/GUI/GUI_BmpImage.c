
#include "Debug.h"
#include "GUI_Paint.h"
#include "GUI_BmpImage.h"

UBYTE GUI_ReadBmpImage(struct BMP_IMAGE *BmpImage, UWORD Xstart, UWORD Ystart)
{
    UWORD Image_Width_Byte;
    UBYTE color, temp;
	UWORD x, y;

	if (!BmpImage)
		return -1;

    Debug("pixel = %d * %d\r\n", BmpImage->Width, BmpImage->Height);

    Image_Width_Byte = (BmpImage->Width % 8 == 0)? (BmpImage->Width / 8): (BmpImage->Width / 8 + 1);

    for(y = 0; y < BmpImage->Height; y++) {
        for(x = 0; x < BmpImage->Width; x++) {
            if(x > Paint.Width || y > Paint.Height) {
                break;
            }
            temp = BmpImage->Data[(x / 8) + (y * Image_Width_Byte)];
            color = (((temp << (x%8)) & 0x80) == 0x80) ?BLACK:WHITE;
            Paint_SetPixel(Xstart + x, Ystart + y, color);
        }
    }

    return 0;
}

UBYTE GUI_ReadBmpImage_2(struct BMP_IMAGE_2 *BmpImage, UWORD Xstart, UWORD Ystart)
{
    UWORD Image_Width_Byte;
    UBYTE color, temp;
	UWORD x, y;

	if (!BmpImage)
		return -1;

    Debug("pixel = %d * %d\r\n", BmpImage->Width, BmpImage->Height);

    Image_Width_Byte = (BmpImage->Width % 8 == 0)? (BmpImage->Width / 8): (BmpImage->Width / 8 + 1);

    for(y = 0; y < BmpImage->Height; y++) {
        for(x = 0; x < BmpImage->Width; x++) {
            if(x > Paint.Width || y > Paint.Height) {
                break;
            }
            temp = BmpImage->Data[(x / 8) + (y * Image_Width_Byte)];
            color = (((temp << (x%8)) & 0x80) == 0x80) ?BLACK:WHITE;
            Paint_SetPixel(Xstart + x, Ystart + y, color);
        }
    }

    return 0;
}

