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


#include "raspi-hif.h"


static struct
{
	int fd;
    struct gpioevent_request req;
} g_raspi_eirq = { .fd = -1 };

/**********************************************************************************************/

int raspi_eirq_open (const char *gpiochip, int gpio, bool rising, bool nonblock)
{
	int ret;

	if (!gpiochip || gpio < 0)
		return -EINVAL;

	if (g_raspi_eirq.fd >= 0)
		return -EBUSY;

/*	log_debug("%s: %s %d %d %d\n", __func__, gpiochip, gpio, rising, nonblock); */

	g_raspi_eirq.fd = open(gpiochip, O_RDWR);
   	if (g_raspi_eirq.fd < 0)
	{
        log_error("open(), %s, %s\n", gpiochip, strerror(errno));
		goto eirq_open_error;
	}

	g_raspi_eirq.req.lineoffset = gpio;
	g_raspi_eirq.req.handleflags = GPIOHANDLE_REQUEST_INPUT;
	g_raspi_eirq.req.eventflags = rising ? GPIOEVENT_REQUEST_RISING_EDGE : GPIOEVENT_REQUEST_FALLING_EDGE;
	strcpy(g_raspi_eirq.req.consumer_label, "eirq");

	ret = ioctl(g_raspi_eirq.fd, GPIO_GET_LINEEVENT_IOCTL, &g_raspi_eirq.req);
   	if (ret	< 0)
	{
        log_error("ioctl(), GPIO_GET_LINEEVENT, %s\n", strerror(errno));
		goto eirq_open_error;
	}

	if (nonblock)
	{
		ret = fcntl(g_raspi_eirq.req.fd, F_GETFL);
		if (ret < 0)
		{
			log_error("fcntl(), F_GETFL, %s\n", strerror(errno));
			goto eirq_open_error;
		}

		ret = fcntl(g_raspi_eirq.req.fd, F_SETFL, ret | O_NONBLOCK);
		if (ret < 0)
		{
			log_error("fcntl(), F_SETFL, %s\n", strerror(errno));
			goto eirq_open_error;
		}
	}

	return 0;

eirq_open_error:

	raspi_eirq_close();

	return -errno;
}

void raspi_eirq_close (void)
{
	if (g_raspi_eirq.fd >= 0)
	{
		close(g_raspi_eirq.fd);
		g_raspi_eirq.fd = -1;
	}
}

/*
 * The timeout argument specifies the number of milliseconds.
 *  - A negative value in timeout means an infinite timeout.
 *  - A timeout of zero causes poll() to return immediately.
 */
int raspi_eirq_poll (int timeout)
{
	if (g_raspi_eirq.fd >= 0)
	{
		struct gpioevent_data evdata;
		struct pollfd fdset;
		int ret;

		fdset.fd = g_raspi_eirq.req.fd;
		fdset.events = POLLIN;
		fdset.revents = 0;

		ret = poll(&fdset, 1, timeout);

		if (ret < 0)
		{
			log_error("poll(), %s\n", strerror(errno));

			return -errno;
		}
		else if (ret == 0)
		{
/*			log_debug("%s: timeout\n", __func__); */

			return -ETIME;
		}
		else if (fdset.revents & POLLIN)
		{
/*			log_debug("%s: POLLIN\n", __func__); */

			for (ret = 0 ; ret < sizeof(evdata) ; )
			{
				ret += read(g_raspi_eirq.req.fd, (char *)&evdata + ret, sizeof(evdata) - ret);

				if (ret < 0 && errno != EAGAIN)
				{
					log_error("read(), %s\n", strerror(errno));

					return -errno;
				}
				else if (ret == sizeof(evdata))
				{
/*					log_debug("%s: id=%d timestamp=%lld\n", __func__, evdata.id, evdata.timestamp); */

					if (evdata.id != (g_raspi_eirq.req.eventflags & 0x3))
						log_info("%s: event id mismatch, %d->%d\n", __func__, g_raspi_eirq.req.eventflags & 0x3, evdata.id);

					return 1;
				}

				log_info("%s: read(), ret=%d/%d\n", __func__, ret, sizeof(evdata));
			}
		}
	}

	return 0;
}

