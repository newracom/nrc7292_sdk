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

#ifndef __RASPI_H__
#define __RASPI_H__
/**********************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <endian.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include <getopt.h>
#include <termios.h>
#include <linux/spi/spidev.h>

/**********************************************************************************************/

#define raspi_printf(fmt, ...)		printf("[RPI] " fmt, ##__VA_ARGS__)

#define raspi_trace()				raspi_printf("%s::%d\n", __func__, __LINE__)
#define raspi_debug(fmt, ...)		raspi_printf(fmt, ##__VA_ARGS__)
#define raspi_info(fmt, ...)		raspi_printf(fmt, ##__VA_ARGS__)
#define raspi_error(fmt, ...)		raspi_printf("%s::%d: " fmt, __func__, __LINE__, ##__VA_ARGS__)

/**********************************************************************************************/

typedef enum
{
	false = 0,
	true = 1
} bool;

enum RASPI_HIF_TYPE
{
	RASPI_HIF_NONE = -1,

	RASPI_HIF_SPI = 0,
	RASPI_HIF_UART,
	RASPI_HIF_UART_HFC,

	RASPI_HIF_NUM
};

typedef struct
{
	enum RASPI_HIF_TYPE type;

	union
	{
		struct
		{
			int (*read)(char *buf, int len);
			int (*write)(char *buf, int len);
		};

		int (*transfer) (struct spi_ioc_transfer *xfers, int n_xfers);
	};
} raspi_hif_t;

/**********************************************************************************************/

/*
 * 	raspi-hif.c
 */
extern int raspi_hif_open (int type, char *device, int speed);
extern void raspi_hif_close (void);
extern int raspi_hif_read (char *buf, int len);
extern int raspi_hif_write (char *buf, int len);


/*
 * 	raspi-uart.c
 */

extern int raspi_uart_open (char *device, uint32_t baudrate, bool hfc);
extern void raspi_uart_close (void);
extern int raspi_uart_read (char *buf, int len);
extern int raspi_uart_write (char *buf, int len);


/*
 * 	raspi-spi.c
 */

#include <linux/spi/spidev.h>

extern int raspi_spi_open (const char *device);
extern int raspi_spi_close (void);
extern int raspi_spi_setup (int mode, int bits_per_word, int max_speed_hz, bool print);
extern int raspi_spi_transfer (struct spi_ioc_transfer *xfers, int n_xfers);
extern int raspi_spi_single_transfer (char *tx_buf, char *rx_buf, int len);

/**********************************************************************************************/
#endif /* #ifndef __RASPI_H__ */
