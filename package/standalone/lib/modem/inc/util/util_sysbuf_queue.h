#ifndef __UTIL_SYS_BUF_QUEUE_H__
#define __UTIL_SYS_BUF_QUEUE_H__

#include "system_common.h"

struct sysbuf_queue {
	uint32_t count;
	SYS_BUF  *head;
	SYS_BUF  *tail;
} __packed;

void util_sysbuf_queue_init(struct sysbuf_queue *hque);
void util_sysbuf_queue_push(struct sysbuf_queue *hque, SYS_BUF *sysbuf);
SYS_BUF *util_sysbuf_queue_pop(struct sysbuf_queue *hque);
SYS_BUF *util_sysbuf_queue_peek(struct sysbuf_queue *hque);
uint32_t util_sysbuf_queue_count(struct sysbuf_queue *hque);

#endif /* __UTIL_QUEUE_H__ */
