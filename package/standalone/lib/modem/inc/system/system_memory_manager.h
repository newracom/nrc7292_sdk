#ifndef SYSTEM_MEMORY_MANAGER_H
#define SYSTEM_MEMORY_MANAGER_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "system_type.h"

enum system_memory_type {
	SYSTEM_MEMORY_POOL_RX,
	SYSTEM_MEMORY_POOL_TX,
	SYSTEM_MEMORY_POOL_MAX
};

#if defined(INCLUDE_BUF_TRACKER)
#define ADD_TRACKER_TAG(buf, tag)             \
	do                                        \
	{                                         \
		ASSERT(buf);                          \
		SYS_BUF *cur = buf;                   \
		while (cur)                           \
		{                                     \
			SYS_HDR(cur).m_tracker_tag = tag; \
			cur = SYS_BUF_LINK(cur);          \
		}                                     \
	} while (0);

enum txbuf_tracker_status {
	TXBUF_FREED = 0,
	TXBUF_ALLOCATED,
	TXBUF_LINKED,
	TXBUF_UPLINK,
	TXBUF_ENQUE_NDP,
	TXBUF_ENQUE,
	TXBUF_REQUE,

	TXBUF_PREALLOC = 10,
	TXBUF_FWREQ,
	TXBUF_WIM_BUILDER,
	TXBUF_WIM_PREALLOC,
	TXBUF_CSPI_SEND,
	TXBUF_BCN

};

enum rxbuf_tracker_status {
	RXBUF_FREED = 0,
	RXBUF_ALLOCATED,
	RXBUF_LINKED,
	RXBUF_S1GHOOK,
	RXBUF_HOST,

	RXBUF_PREALLOC = 10
};
#else
#define ADD_TRACKER_TAG(buf, tag)             \
	do                                        \
	{                                         \
	} while (0);
#endif /* defined(INCLUDE_BUF_TRACKER) */

/** System Memory information */
typedef struct system_memory_info {
	uint8_t pool_id;		// pool id
	uint16_t size;		// buffer element size
	uint16_t num;		// number of total buffers
	uint16_t avail;		// number of available buffers
	SYS_BUF* sys_buf_pool;	// sys_buf start address
}system_memory_info_t;

void system_memory_pool_init(void);
struct _SYS_BUF* system_memory_pool_alloc(uint8_t);
void system_memory_pool_free(struct _SYS_BUF* buf);
void system_memory_pool_print        (uint8_t);
int system_memory_pool_number_of_link(struct _SYS_BUF* buf);
void system_memory_pool_print_all(void);
void system_memory_pool_print_detail(uint8_t pool_id);
void system_memory_pool_register_free_hook(bool(*free_hook)(SYS_BUF *packet));
uint16_t system_memory_pool_avail_number(uint8_t pool_id);
void *system_memory_base_address(void);
uint8_t* system_memory_pool_base_addr(uint8_t pool_id);
uint16_t system_memory_pool_buf_size(uint8_t pool_id);

struct _SYS_BUF* sys_buf_alloc(uint8_t pool_id , uint16_t hif_len);
void sys_buf_len_calc_using_hif_len(struct _SYS_BUF *packet);
int system_memory_pool_get_index(SYS_BUF* sbuf);

#endif //SYSTEM_MEMORY_MANAGER_H
