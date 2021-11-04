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


#include "hif.h"


/**********************************************************************************************/

#ifdef CONFIG_HIF_FIFO_MUTEX

static SemaphoreHandle_t g_hif_fifo_mutex = NULL;

static int _hif_fifo_mutex_create (_hif_fifo_t *fifo)
{
	if (fifo)
	{
		fifo->mutex.handle = xSemaphoreCreateMutexStatic(&fifo->mutex.buffer);
		if (fifo->mutex.handle)
		{
/*			_hif_info("fifo_mutex: fifo=%p handle=%p\n", fifo, fifo->mutex.handle); */
		   return 0;
		}
	}

	return -1;
}

static void _hif_fifo_mutex_delete (_hif_fifo_t *fifo)
{
	SemaphoreHandle_t handle = fifo ? fifo->mutex.handle : NULL;

	if (handle)
		vSemaphoreDelete(handle);
}

static bool __hif_fifo_mutex_take (_hif_fifo_t *fifo, bool isr)
{
	SemaphoreHandle_t handle = fifo ? fifo->mutex.handle : NULL;
	bool take;

	if (!handle)
		return true;

	if (!isr)
	{
		int time_ms = 0; // block

		take = xSemaphoreTake(handle, pdMS_TO_TICKS(time_ms));
	}
	else
	{
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;

		take = xSemaphoreTakeFromISR(handle, &xHigherPriorityTaskWoken);

		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}

/*	if (!take)
		_hif_error("fail (%s, %p)\n", isr ? " isr" : "", handle); */

	return take;
}

static bool __hif_fifo_mutex_give (_hif_fifo_t *fifo, bool isr)
{
	SemaphoreHandle_t handle = fifo ? fifo->mutex.handle : NULL;
	bool give;

	if (!handle)
		return true;

	if (!isr)
		give = xSemaphoreGive(handle);
	else
	{
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;

		give = xSemaphoreGiveFromISR(handle, &xHigherPriorityTaskWoken);

		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}

	if (!give)
		_hif_error("fail (%s, %p)\n", isr ? " isr" : "", handle);

	return give;
}

bool _hif_fifo_mutex_take (_hif_fifo_t *fifo)
{
	return __hif_fifo_mutex_take(fifo, false);
}

bool _hif_fifo_mutex_take_isr (_hif_fifo_t *fifo)
{
	return __hif_fifo_mutex_take(fifo, true);
}

bool _hif_fifo_mutex_give (_hif_fifo_t *fifo)
{
	return __hif_fifo_mutex_give(fifo, false);
}

bool _hif_fifo_mutex_give_isr (_hif_fifo_t *fifo)
{
	return __hif_fifo_mutex_give(fifo, true);
}

#else

bool _hif_fifo_mutex_take (_hif_fifo_t *fifo)
{
	return true;
}

bool _hif_fifo_mutex_take_isr (_hif_fifo_t *fifo)
{
	return true;
}

bool _hif_fifo_mutex_give (_hif_fifo_t *fifo)
{
	return true;
}

bool _hif_fifo_mutex_give_isr (_hif_fifo_t *fifo)
{
	return true;
}

#endif /* #ifdef CONFIG_HIF_FIFO_MUTEX */

/*********************************************************************************************/

_hif_fifo_t *_hif_fifo_create (char *buffer, int size, bool mutex)
{
	_hif_fifo_t *fifo;

	if (size <= 0)
		return NULL;

	fifo = (_hif_fifo_t *)_hif_malloc(sizeof(_hif_fifo_t));
	if (!fifo)
		return NULL;

	fifo->static_buffer = buffer ? true : false;

	if (fifo->static_buffer)
		fifo->buffer = buffer;
	else
	{
		fifo->buffer = (char *)_hif_malloc(size);
		if (!fifo->buffer)
		{
			_hif_free(fifo);
			return NULL;
		}
	}

	fifo->buffer_end = fifo->buffer + size;

#ifdef CONFIG_HIF_FIFO_MUTEX
	if (mutex && _hif_fifo_mutex_create(fifo) != 0)
	{
		_hif_free(fifo);
		return NULL;
	}
#endif

	fifo->size = size;
	fifo->cnt = 0;
	fifo->push_idx = 0;
	fifo->pop_idx = 0;

	return fifo;
}

void _hif_fifo_delete (_hif_fifo_t *fifo)
{
	if (fifo)
	{
#ifdef CONFIG_HIF_FIFO_MUTEX
		_hif_fifo_mutex_delete(fifo);
#endif

		if (!fifo->static_buffer)
			_hif_free(fifo->buffer);

		_hif_free(fifo);
	}
}

void _hif_fifo_reset (_hif_fifo_t *fifo)
{
	if (fifo)
	{
		fifo->cnt = 0;
		fifo->push_idx = 0;
		fifo->pop_idx = 0;
	}
}

char *_hif_fifo_push_addr (_hif_fifo_t *fifo, int offset)
{
	int buf_idx = (fifo->push_idx + offset) % fifo->size;

	return &fifo->buffer[buf_idx];
}

char *_hif_fifo_pop_addr (_hif_fifo_t *fifo, int offset)
{
	int buf_idx = (fifo->pop_idx + offset) % fifo->size;

	return &fifo->buffer[buf_idx];
}

int _hif_fifo_free_size (_hif_fifo_t *fifo)
{
	return fifo->size - fifo->cnt;
}

int _hif_fifo_fill_size (_hif_fifo_t *fifo)
{
	return fifo->cnt;
}

bool _hif_fifo_empty (_hif_fifo_t *fifo)
{
	return !!(_hif_fifo_free_size(fifo) == fifo->size);
}

bool _hif_fifo_full (_hif_fifo_t *fifo)
{
	return !!(_hif_fifo_free_size(fifo) == 0);
}

char _hif_fifo_getc (_hif_fifo_t *fifo)
{
	char data;

	data = fifo->buffer[fifo->pop_idx % fifo->size];
	fifo->pop_idx++;

	fifo->cnt--;

	return data;
}

void _hif_fifo_putc (_hif_fifo_t *fifo, char c)
{
	fifo->buffer[fifo->push_idx % fifo->size] = c;
	fifo->push_idx++;

	fifo->cnt++;
}

int _hif_fifo_read (_hif_fifo_t *fifo, char *buf, int len)
{
	if (fifo && len > 0)
	{
		int fill_size = _hif_fifo_fill_size(fifo);

		if (len > fill_size)
			len = fill_size;

		if (!buf)
			fifo->pop_idx += len;
		else
		{
			char *pop_addr;
			int pop_len;
			int i;

			for (i = 0 ; i < len ; )
			{
				pop_addr = &fifo->buffer[fifo->pop_idx % fifo->size];

				pop_len = fifo->buffer_end - pop_addr;
				if (pop_len > (len - i))
					pop_len = len - i;

				memcpy(buf + i, pop_addr, pop_len);

				i += pop_len;
				fifo->pop_idx += pop_len;
			}
		}

		fifo->cnt -= len;

		return len;
	}

	return 0;
}

int _hif_fifo_write (_hif_fifo_t *fifo, char *buf, int len)
{
	if (fifo && len > 0)
	{
		int free_size = _hif_fifo_free_size(fifo);

		if (len > free_size)
			len = free_size;

		if (!buf)
			fifo->push_idx += len;
		else
		{
			char *push_addr;
			int push_len;
			int i;

			for (i = 0 ; i < len ; )
			{
				push_addr = &fifo->buffer[fifo->push_idx % fifo->size];

				push_len = fifo->buffer_end - push_addr;
				if (push_len > (len - i))
					push_len = len - i;

				memcpy(push_addr, buf + i, push_len);

				i += push_len;
				fifo->push_idx += push_len;
			}
		}

		fifo->cnt += len;

		return len;
	}

	return 0;
}
