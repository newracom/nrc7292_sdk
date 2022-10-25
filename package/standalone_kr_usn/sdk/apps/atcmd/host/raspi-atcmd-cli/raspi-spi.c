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


#include "raspi-hif.h"


/* #define CONFIG_SPIDEV_MODE32 */

#define raspi_spi_info(fmt, ...)		/* log_info(fmt, ##__VA_ARGS__) */
#define raspi_spi_error(fmt, ...)		log_error(fmt, ##__VA_ARGS__)


static int g_raspi_spi_fd = -1;

/**********************************************************************************************/

static int raspi_spi_ioctl (unsigned long request, void *val)
{
	if (g_raspi_spi_fd < 0)
	{
		errno = ENODEV;
		return -1;
	}

	return ioctl(g_raspi_spi_fd, request, val);
}

static int raspi_spi_set_mode (uint32_t mode)
{
#ifdef CONFIG_SPIDEV_MODE32
	return raspi_spi_ioctl(SPI_IOC_WR_MODE32, &mode)
#else
	return raspi_spi_ioctl(SPI_IOC_WR_MODE, &mode);
#endif
}

static int raspi_spi_get_mode (uint32_t *mode)
{
#ifdef CONFIG_SPIDEV_MODE32
	return raspi_spi_ioctl(SPI_IOC_RD_MODE32, mode)
#else
	return raspi_spi_ioctl(SPI_IOC_RD_MODE, mode);
#endif
}

/* static int raspi_spi_set_lsb_first (uint8_t lsb_first)
{
	return raspi_spi_ioctl(SPI_IOC_WR_LSB_FIRST, &lsb_first);
} */

/* static int raspi_spi_get_lsb_first (uint8_t *lsb_first)
{
	return raspi_spi_ioctl(SPI_IOC_RD_LSB_FIRST, lsb_first);
} */

static int raspi_spi_set_bits_per_word (uint8_t bits_per_word)
{
	return raspi_spi_ioctl(SPI_IOC_WR_BITS_PER_WORD, &bits_per_word);
}

static int raspi_spi_get_bits_per_word (uint8_t *bits_per_word)
{
	return raspi_spi_ioctl(SPI_IOC_RD_BITS_PER_WORD, bits_per_word);
}

static int raspi_spi_set_max_speed_hz (uint32_t max_speed_hz)
{
	return raspi_spi_ioctl(SPI_IOC_WR_MAX_SPEED_HZ, &max_speed_hz);
}

static int raspi_spi_get_max_speed_hz (uint32_t *max_speed_hz)
{
	return raspi_spi_ioctl(SPI_IOC_RD_MAX_SPEED_HZ, max_speed_hz);
}

/**********************************************************************************************/

int raspi_spi_open (const char *device)
{
	int fd;

	if (!device)
	{
		raspi_spi_error("%s\n", strerror(EINVAL));
		return -EINVAL;
	}

	if (g_raspi_spi_fd >= 0)
	{
		raspi_spi_error("%s\n", strerror(EBUSY));
		return -EBUSY;
	}

	fd = open(device, O_RDWR);
	if (fd < 0)
	{
		raspi_spi_error("%s, open()\n", strerror(errno));
		return -errno;
	}

	g_raspi_spi_fd = fd;

	return 0;
}

int raspi_spi_close (void)
{
	if (g_raspi_spi_fd < 0)
	{
		raspi_spi_error("%s\n", strerror(ENODEV));
		return -ENODEV;
	}

	if (close(g_raspi_spi_fd) != 0)
	{
		raspi_spi_error("%s, close()\n", strerror(errno));
		return -errno;
	}

	g_raspi_spi_fd = -1;

	return 0;
}

static int raspi_spi_setup_print (void)
{
	uint32_t mode;
	uint8_t bits_per_word;
	uint32_t max_speed_hz;

	if (raspi_spi_get_mode(&mode) != 0)
	{
		raspi_spi_error("%s, get_mode()\n", strerror(errno));
		return -errno;
	}

	if (raspi_spi_get_bits_per_word(&bits_per_word) != 0)
	{
		raspi_spi_error("%s, get_bits_per_word()\n", strerror(errno));
		return -errno;
	}

	if (raspi_spi_get_max_speed_hz(&max_speed_hz) != 0)
	{
		raspi_spi_error("%s, get_max_speed_hz()\n", strerror(errno));
		return -errno;
	}

	printf("[ SPI Configurations ]\n");
	printf("  - MODE : 0x%X\n", mode);
	printf("	- CPHA	  : %d\n", !!(mode & SPI_CPHA));
	printf("	- CPOL	  : %d\n", !!(mode & SPI_CPOL));
	printf("	- CS_HIGH   : %d\n", !!(mode & SPI_CS_HIGH));
	printf("	- LSB_FIRST : %d\n", !!(mode & SPI_LSB_FIRST));
	printf("	- 3WIRE	 : %d\n", !!(mode & SPI_3WIRE));
	printf("	- LOOP	  : %d\n", !!(mode & SPI_LOOP));
	printf("	- NO_CS	 : %d\n", !!(mode & SPI_NO_CS));
	printf("	- READY	 : %d\n", !!(mode & SPI_READY));
#ifdef CONFIG_SPIDEV_MODE32
	printf("	- TX_DUAL   : %d\n", !!(mode & SPI_TX_DUAL));
	printf("	- TX_QUAD   : %d\n", !!(mode & SPI_TX_QUAD));
	printf("	- RX_QUAD   : %d\n", !!(mode & SPI_RX_DUAL));
	printf("	- RX_QUAD   : %d\n", !!(mode & SPI_RX_QUAD));
#endif
	printf("  - BITS_PER_WORD : %u\n", bits_per_word);
	printf("  - MAX_SPEED_HZ  : %u\n", max_speed_hz);
	printf("\n");

	return 0;
}

int raspi_spi_setup (int mode, int bits_per_word, int max_speed_hz, bool print)
{
	if (raspi_spi_set_mode(mode & 0x3) != 0)
	{
		raspi_spi_error("%s, set_mode()\n", strerror(errno));
		return -errno;
	}

	if (raspi_spi_set_bits_per_word(bits_per_word) != 0)
	{
		raspi_spi_error("%s, set_bits_per_word()\n", strerror(errno));
		return -errno;
	}

	if (raspi_spi_set_max_speed_hz(max_speed_hz) != 0)
	{
		raspi_spi_error("%s, set_max_speed_hz()\n", strerror(errno));
		return -errno;
	}

	if (print)
		return raspi_spi_setup_print();

	return 0;
}

int raspi_spi_transfer (struct spi_ioc_transfer *xfers, int n_xfers)
{
	int ret = raspi_spi_ioctl(SPI_IOC_MESSAGE(n_xfers), xfers);

   	if (ret	< 0)
	{
		raspi_spi_error("%s\n", strerror(errno));
		return -errno;
	}

	return ret;
}

int raspi_spi_single_transfer (char *tx_buf, char *rx_buf, int len)
{
	struct spi_ioc_transfer xfer;
	int ret;

	if ((!tx_buf && !rx_buf) || !len)
	{
		raspi_spi_error("%s\n", strerror(EINVAL));
		return -EINVAL;
	}

	memset(&xfer, 0, sizeof(struct spi_ioc_transfer));

#if __BITS_PER_LONG == 64
	xfer.tx_buf = (__u64)tx_buf;
	xfer.rx_buf = (__u64)rx_buf;
#else
	xfer.tx_buf = (__u32)tx_buf;
	xfer.rx_buf = (__u32)rx_buf;
#endif
	xfer.len = len;

/*	xfer.delay_usecs = 1; */
/*	xfer.cs_change = 1; */

	ret = raspi_spi_transfer(&xfer, 1);
	if (ret < 0)
		return ret;
	else if (ret != len)
		raspi_spi_error("not completed. (%d/%d)\n", ret, len);

	return 0;
}

