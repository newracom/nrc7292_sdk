/*
 * MIT License
 *
 * Copyright (c) 2023 Newracom, Inc.
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

#ifdef NRC_LWIP
#include "nrc_sdk.h"
#else
#include <stdlib.h>
#include <string.h>
#endif

#include "tlv.h"

static void encode_int(char *buffer, int value)
{
	buffer[0] = (value & 0xff000000) >> 24;
	buffer[1] = (value & 0x00ff0000) >> 16;
	buffer[2] = (value & 0x0000ff00) >> 8;
	buffer[3] = (value & 0x000000ff);
}

static int decode_int(char *buffer)
{
	int value = 0;

	value |= ((0xff & buffer[0]) << 24);
	value |= ((0xff & buffer[1]) << 16);
	value |= ((0xff & buffer[2]) << 8);
	value |= (0xff & buffer[3]);

	return value;
}

char *tlv_encode(int tag, unsigned int length, char *value)
{
	char *buffer;

#ifdef NRC_LWIP
	buffer = (char *) nrc_mem_malloc(sizeof(int) + sizeof(int) + length + 1);
#else
	buffer = (char *) malloc(sizeof(int) + sizeof(int) + length + 1);
#endif
	memset(buffer, 0, sizeof(int) + sizeof(int) + length + 1);

	encode_int(buffer, tag);
	encode_int(buffer + sizeof(int), length);

	if (value) {
		memcpy(buffer + sizeof(int) + sizeof(int), value, length);
	}
	return buffer;
}

void tlv_decode(char *buffer, unsigned int *tag, unsigned int *length, char *value)
{
	*tag = decode_int(buffer);
	*length = decode_int(buffer + sizeof(int));
	memcpy(value, buffer + sizeof(int) + sizeof(int), *length);
}
