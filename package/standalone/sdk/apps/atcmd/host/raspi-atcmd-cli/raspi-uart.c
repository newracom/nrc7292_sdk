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


#include <termios.h>
#include <sys/mman.h>

#include "raspi-hif.h"

#define raspi_uart_info(fmt, ...)		/* log_info(fmt, ##__VA_ARGS__) */
#define raspi_uart_error(fmt, ...)		log_error(fmt, ##__VA_ARGS__)

/**********************************************************************************************/

static int raspi_gpio_base (unsigned *addr)
{
	const char *str_file = "/proc/device-tree/soc/ranges";
	unsigned char buf[4];
	FILE *fp;

	fp = fopen(str_file, "rb");
	if (!fp)
	{
		raspi_uart_error("%s, open(), %s\n", strerror(errno), str_file);
		return -errno;
	}

	fseek(fp, 4, SEEK_SET);

	if (fread(buf, 1, sizeof buf, fp) == sizeof buf)
		*addr = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3] << 0;
	else
		*addr = 0x20000000;

	*addr += 0x200000; /* gpio offset */

	fclose(fp);

	return 0;
}

static int raspi_gpio_mmap (unsigned gpio_base, void **gpio_map)
{
	const char *str_file = "/dev/mem";
	void *addr;
	int fd;

	fd = open(str_file, O_RDWR|O_SYNC);
	if (fd < 0)
	{
		raspi_uart_error("%s, open(), %s\n", strerror(errno), str_file);
		return -errno;
	}

	addr = mmap(NULL, 4 * 1024, PROT_READ|PROT_WRITE, MAP_SHARED, fd, gpio_base);
	if (addr == MAP_FAILED)
	{
		close(fd);

		raspi_uart_error("%s, mmap()\n", strerror(errno));
		return -errno;
	}

	*gpio_map = addr;

	close(fd);

	return 0;
}

static int raspi_uart_rts_cts_config (bool enable)
{
	const int gfpsel = 1;
   	const int gpiomask = 0x00fc0000; /* GPIO 16 for CTS0 and 17 for RTS0 */
	volatile unsigned *gpio;
	unsigned gpio_base;
	int err;

	err = raspi_gpio_base(&gpio_base);
	if (err)
		return err;

	err = raspi_gpio_mmap(gpio_base, (void **)&gpio);
	if (err)
		return err;

	if (enable)
	   	gpio[gfpsel] |= gpiomask;
   	else
		gpio[gfpsel] &= ~gpiomask;

	raspi_uart_info("UART RTS/CTS: %s\n", enable ? "enable" : "disable");

	return 0;
}

static int raspi_uart_rts_cts_enable (void)
{
	return raspi_uart_rts_cts_config(true);
}

static int raspi_uart_rts_cts_disable (void)
{
	return raspi_uart_rts_cts_config(false);
}

/**********************************************************************************************/

static int g_raspi_uart_comfd = -1;
static bool g_raspi_uart_rts_cts = false;

static unsigned int raspi_uart_baudrate_to_flag (uint32_t baudrate)
{
	switch (baudrate)
	{
/*		case 1200: 	return B1200;
		case 2400: 	return B2400;
		case 4800: 	return B4800;
		case 9600: 	return B9600; */
		case 19200: 	return B19200;
		case 38400: 	return B38400;
		case 57600: 	return B57600;
		case 115200: 	return B115200;
		case 230400:	return B230400;
		case 460800:	return B460800;
		case 500000:	return B500000;
		case 576000:	return B576000;
		case 921600:	return B921600;
		case 1000000:	return B1000000;
		case 1152000:	return B1152000;
		case 1500000:	return B1500000;
		case 2000000:	return B2000000;
/*		case 2500000:	return B2500000;
		case 3000000:	return B3000000;
		case 3500000:	return B3500000;
		case 4000000:	return B4000000; */
		default:		return B0;
	}
}

int raspi_uart_open (char *device, uint32_t baudrate, bool rts_cts)
{
	struct termios oldtio, newtio;
	int flag_baudrate;
	int comfd;
	int err;

	raspi_uart_info("uart_open: device=%s baudrate=%u rts/cts=%s\n",
						device, baudrate, rts_cts ? "on" : "off");

	flag_baudrate = raspi_uart_baudrate_to_flag(baudrate);
	if (flag_baudrate == B0)
	{
		raspi_uart_error("%s, baudrate=%u\n", strerror(EINVAL), baudrate);
		return -EINVAL;
	}

	comfd = open(device, O_RDWR | O_NOCTTY);
/*	comfd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK); */
/*	comfd = open(device, O_RDWR | O_NOCTTY | O_NDELAY); */
	if (comfd < 0)
	{
		raspi_uart_error("%s, open()\n", strerror(errno));
		return -errno;
	}

	if (rts_cts)
		err = raspi_uart_rts_cts_enable();
	else
		err = raspi_uart_rts_cts_disable();

	if (err)
		return err;

	tcgetattr(comfd,&oldtio); /* save current port settings */

	newtio.c_cflag = CS8 | CLOCAL | CREAD | (rts_cts ? CRTSCTS : 0);
	newtio.c_cflag |= flag_baudrate;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	newtio.c_lflag = 0;
	newtio.c_cc[VMIN]=1;
	newtio.c_cc[VTIME]=0;
	tcflush(comfd, TCIOFLUSH);
	tcsetattr(comfd,TCSAFLUSH,&newtio);

	g_raspi_uart_comfd = comfd;
	g_raspi_uart_rts_cts = rts_cts;

	return 0;
}

void raspi_uart_close (void)
{
	raspi_uart_info("uart_close\n");

	if (g_raspi_uart_comfd >= 0)
		close(g_raspi_uart_comfd);

	if (g_raspi_uart_rts_cts)
		raspi_uart_rts_cts_disable();

	g_raspi_uart_comfd = -1;
	g_raspi_uart_rts_cts = false;
}

int raspi_uart_read (char *buf, int len)
{
	int ret = -ENODEV;

	if (g_raspi_uart_comfd >= 0)
	{
		ret = read(g_raspi_uart_comfd, buf, len);
		if (ret < 0)
			ret = -errno;
	}

	return ret;
}

int raspi_uart_write (char *buf, int len)
{
	int ret = -ENODEV;

	if (g_raspi_uart_comfd >= 0)
	{
		ret = write(g_raspi_uart_comfd, buf, len);
		if (ret < 0)
			ret = -errno;
	}

	return ret;
}

