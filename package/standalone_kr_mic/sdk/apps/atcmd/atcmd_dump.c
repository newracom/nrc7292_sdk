/*
 * MIT License
 *
 * Copyright (c) 2020 Newracom, Inc.
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


#include "atcmd.h"


static const char hex_asc[] = "0123456789abcdef";

#define hex_asc_lo(x)	hex_asc[((x) & 0x0f)]
#define hex_asc_hi(x)	hex_asc[((x) & 0xf0) >> 4]

#define _D	0x04	/* digit */
#define _SP	0x80	/* hard space (0x20) */

static const unsigned char _ctype[] =
{
	_C,_C,_C,_C,_C,_C,_C,_C,							/* 0-7 */
	_C,_C|_S,_C|_S,_C|_S,_C|_S,_C|_S,_C,_C,				/* 8-15 */
	_C,_C,_C,_C,_C,_C,_C,_C,							/* 16-23 */
	_C,_C,_C,_C,_C,_C,_C,_C,							/* 24-31 */
	_S|_SP,_P,_P,_P,_P,_P,_P,_P,						/* 32-39 */
	_P,_P,_P,_P,_P,_P,_P,_P,							/* 40-47 */
	_D,_D,_D,_D,_D,_D,_D,_D,							/* 48-55 */
	_D,_D,_P,_P,_P,_P,_P,_P,							/* 56-63 */
	_P,_U|_X,_U|_X,_U|_X,_U|_X,_U|_X,_U|_X,_U,			/* 64-71 */
	_U,_U,_U,_U,_U,_U,_U,_U,							/* 72-79 */
	_U,_U,_U,_U,_U,_U,_U,_U,							/* 80-87 */
	_U,_U,_U,_P,_P,_P,_P,_P,							/* 88-95 */
	_P,_L|_X,_L|_X,_L|_X,_L|_X,_L|_X,_L|_X,_L,			/* 96-103 */
	_L,_L,_L,_L,_L,_L,_L,_L,							/* 104-111 */
	_L,_L,_L,_L,_L,_L,_L,_L,							/* 112-119 */
	_L,_L,_L,_P,_P,_P,_P,_C,							/* 120-127 */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,					/* 128-143 */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,					/* 144-159 */
	_S|_SP,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,/* 160-175 */
	_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,	/* 176-191 */
	_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,	/* 192-207 */
	_U,_U,_U,_U,_U,_U,_U,_P,_U,_U,_U,_U,_U,_U,_U,_L,	/* 208-223 */
	_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,	/* 224-239 */
	_L,_L,_L,_L,_L,_L,_L,_P,_L,_L,_L,_L,_L,_L,_L,_L		/* 240-255 */
};

static int _atcmd_dump_printf (bool terminal, const char *fmt, ...)
{
	char buf[ATCMD_LEN_MAX];
	int len;
	va_list ap;

	va_start(ap, fmt);

	len = vsnprintf(buf, sizeof(buf), fmt, ap);
	if (len > 0)
	{
		if (terminal)
			atcmd_transmit(buf, len);
		else
			_atcmd_printf("%s", buf);
	}

	return len;
}

static int _atcmd_dump_hex_to_buffer (const void *buf, size_t len,
						char *linebuf, size_t linebuflen, bool ascii)
{
	const uint8_t *ptr = buf;
	int ngroups;
	uint8_t ch;
	int j, lx = 0;
	int ascii_column;
	int ret;

	if (len > 16)		/* limit to one line at a time */
		len = 16;

	ngroups = len;
	ascii_column = 16 * 2 + 16 + 1;

	if (!linebuflen)
		goto overflow1;

	if (!len)
		goto nil;

	for (j = 0; j < len; j++)
	{
		if (linebuflen < lx + 2)
			goto overflow2;

		ch = ptr[j];
		linebuf[lx++] = hex_asc_hi(ch);

		if (linebuflen < lx + 2)
			goto overflow2;

		linebuf[lx++] = hex_asc_lo(ch);

		if (linebuflen < lx + 2)
			goto overflow2;

		linebuf[lx++] = ' ';
	}

	if (j)
		lx--;

	if (!ascii)
		goto nil;

	while (lx < ascii_column)
	{
		if (linebuflen < lx + 2)
			goto overflow2;

		linebuf[lx++] = ' ';
	}

	for (j = 0; j < len; j++)
	{
		if (linebuflen < lx + 2)
			goto overflow2;

		ch = ptr[j];
		linebuf[lx++] = (isascii(ch) && isprint(ch)) ? ch : '.';
	}

nil:
	linebuf[lx] = '\0';
	return lx;

overflow2:
	linebuf[lx++] = '\0';

overflow1:
	return ascii ? ascii_column + len : 3 * ngroups - 1;
}

static void _atcmd_dump_hex_print (const void *buf, size_t len, bool ascii, bool terminal)
{
	int type = 1;
	const uint8_t *ptr = buf;
	int i, linelen, remaining = len;
	char linebuf[32 * 3 + 2 + 32 + 1];

	for (i = 0; i < len; i += 16)
	{
		if (remaining < 16)
			linelen = remaining;
		else
			linelen = 16;

		remaining -= 16;

		_atcmd_dump_hex_to_buffer(ptr + i, linelen, linebuf, sizeof(linebuf), ascii);

		switch (type)
		{
			case 0: /* DUMP_PREFIX_ADDRESS */
				_atcmd_dump_printf(terminal, "%p: %s\r\n", ptr + i, linebuf);
				break;

			case 1: /* DUMP_PREFIX_OFFSET */
				_atcmd_dump_printf(terminal, "%.8x: %s\r\n", i, linebuf);
				break;

			default:
				_atcmd_dump_printf(terminal, "%s\r\n", linebuf);
		}
	}

	_atcmd_dump_printf(terminal, "\r\n");
}

void atcmd_dump_hex_print (const void *buf, size_t len, bool ascii)
{
	return _atcmd_dump_hex_print(buf, len, ascii, false);
}


void atcmd_dump_hex_print_terminal (const void *buf, size_t len, bool ascii)
{
	return _atcmd_dump_hex_print(buf, len, ascii, true);
}

