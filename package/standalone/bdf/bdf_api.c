#include "nrc_sdk.h"

#include "bd.h"
#define BDF_LEN   __bdf_bd_dat_len
#define BDF_DATA   __bdf_bd_dat

unsigned char* get_bdf_data(unsigned int* size)
{
	unsigned char *buffer = NULL;
	unsigned int length = 0;

	length = (unsigned int)BDF_LEN;
	*size = length;

	if(length == 0) {
		E(TT_API, "Size of BD file is zero\n");
		return NULL;
	}

	buffer = pvPortMalloc(length);
	if(!buffer)
		return NULL;

	memset(buffer, 0x0, length);
	memcpy(buffer, BDF_DATA, length);

	V(TT_API, "bd buffer %p length %d\n",buffer, length);

	return buffer;
}