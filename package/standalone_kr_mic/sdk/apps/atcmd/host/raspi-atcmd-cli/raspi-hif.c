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
#include "nrc-hspi.h"

/**********************************************************************************************/

static pthread_mutex_t g_raspi_hif_mutex;

static void raspi_hif_mutex_init (void)
{
	int ret = pthread_mutex_init(&g_raspi_hif_mutex, NULL);

	if (ret != 0)
		log_error("pthread_mutex_init, %s\n", strerror(ret));
}

static void raspi_hif_mutex_lock (void)
{
	int ret = pthread_mutex_lock(&g_raspi_hif_mutex);

	if (ret != 0)
		log_error("pthread_mutex_lock, %s\n", strerror(ret));
}

static void raspi_hif_mutex_unlock (void)
{
	int ret = pthread_mutex_unlock(&g_raspi_hif_mutex);

	if (ret < 0)
		log_error("pthread_mutex_unlock, %s\n", strerror(ret));
}

/**********************************************************************************************/

static raspi_hif_t g_raspi_hif =
{
	.type = RASPI_HIF_NONE,
	.flags = 0,
	.read = NULL,
	.write = NULL,
};

static void raspi_hif_init (raspi_hif_t *hif)
{
	hif->type = RASPI_HIF_NONE;
	hif->flags = 0;
	hif->read = NULL;
	hif->write = NULL;
}

int raspi_hif_open (int type, char *device, uint32_t speed, uint32_t flags)
{
	raspi_hif_t *hif = &g_raspi_hif;
	int ret;

	if (type == RASPI_HIF_NONE || !device || !speed)
		return -EINVAL;

	if (hif->type != RASPI_HIF_NONE)
		return -EBUSY;

	switch (type)
	{
		case RASPI_HIF_SPI:
			log_info("\r\n");
			log_info("[ SPI ]\n");
			log_info(" - device: %s\n", device);
			log_info(" - clock: %d Hz\n", speed);
			log_info("\r\n");

			ret = raspi_spi_open(device);
			if (ret == 0)
			{
				int mode = 0;
				int bits_per_word = 8;
				int max_speed_hz = speed;

				ret = raspi_spi_setup(mode, bits_per_word, max_speed_hz, false);
				if (ret == 0)
				{
					hspi_ops_t ops;

					raspi_hif_mutex_init();

					ops.spi_transfer = raspi_spi_single_transfer;

					ret = nrc_hspi_open(&ops);
					if (ret == 0)
					{
						hif->type = RASPI_HIF_SPI;
						hif->flags = flags;
						hif->read = nrc_hspi_read;
						hif->write = nrc_hspi_write;
					}
				}
			}
			break;

		case RASPI_HIF_UART:
			log_info("\r\n");
			log_info("[ UART%s ]\n", flags & RASPI_HIF_UART_HFC ? "_HFC" : "");
			log_info(" - device: %s\n", device);
			log_info(" - baudrate : %d\n", speed);
			log_info("\r\n");

			ret = raspi_uart_open(device, speed, !!(flags & RASPI_HIF_UART_HFC));
			if (ret == 0)
			{
				hif->type = type;
				hif->flags = flags;
				hif->read = raspi_uart_read;
				hif->write = raspi_uart_write;
			}
			break;

		default:
			return -ENODEV;
	}

	return ret;
}

void raspi_hif_close (void)
{
	raspi_hif_t *hif = &g_raspi_hif;

	switch (hif->type)
	{
		case RASPI_HIF_SPI:
			nrc_hspi_close();
			raspi_spi_close();
			break;

		case RASPI_HIF_UART:
			raspi_uart_close();
			break;

		default:
			break;
	}

	raspi_hif_init(hif);
}

int raspi_hif_read (char *buf, int len)
{
	raspi_hif_t *hif = &g_raspi_hif;
	int ret;

	switch (hif->type)
	{
		case RASPI_HIF_SPI:
		case RASPI_HIF_UART:
			if (!hif->read)
				return -EPERM;
			break;

		default:
			return -ENODEV;
	}

	if (hif->type == RASPI_HIF_SPI)
		raspi_hif_mutex_lock();

	ret = hif->read(buf, len);

	if (hif->type == RASPI_HIF_SPI)
		raspi_hif_mutex_unlock();

	return ret;
}

int raspi_hif_write (char *buf, int len)
{
	raspi_hif_t *hif = &g_raspi_hif;
	int ret;

	switch (hif->type)
	{
		case RASPI_HIF_SPI:
		case RASPI_HIF_UART:
			if (!hif->write)
				return -EPERM;
			break;

		default:
			return -ENODEV;
	}

	if (hif->type == RASPI_HIF_SPI)
		raspi_hif_mutex_lock();

	ret = hif->write(buf, len);

	if (hif->type == RASPI_HIF_SPI)
		raspi_hif_mutex_unlock();

	return ret;
}
