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
#define ADD_TRACKER_TAG(buf, tag)					\
	do												\
	{												\
		if (buf) {									\
			SYS_BUF *cur = buf;                	    \
			while (cur)                        	    \
			{                                  	    \
				SYS_HDR(cur).m_tracker_tag = tag; 	\
				cur = SYS_BUF_LINK(cur);          	\
			}                                     	\
		} else {									\
			V(TT_MM, "sys: Try to add a tag to a null buffer\n");	\
		}															\
	} while (0);

enum txbuf_tracker_status {
	TXBUF_FREED 		= 0,
	TXBUF_ALLOCATED		= 1,
	TXBUF_LINKED		= 2,
	TXBUF_UPLINK		= 3,
	TXBUF_ENQUE_NDP		= 4,
	TXBUF_ENQUE			= 5,
	TXBUF_REQUE			= 6,

	TXBUF_PREALLOC 		= 10,
	TXBUF_FWREQ			= 11,
	TXBUF_WIM_BUILDER 	= 12,
	TXBUF_WIM_PREALLOC	= 13,
	TXBUF_CSPI_SEND		= 14,
	TXBUF_BCN			= 15
};

enum rxbuf_tracker_status {
	RXBUF_FREED 		= 0,
	RXBUF_ALLOCATED		= 1,
	RXBUF_LINKED		= 2,
	RXBUF_S1GHOOK		= 3,
	RXBUF_HOST			= 4,
	RXBUF_SENT			= 5,
	RXBUF_CB			= 6,

	RXBUF_PREALLOC 		= 10
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
#if !defined(RELEASE)
struct _SYS_BUF* sys_buf_alloc_for_lb(uint8_t pool_id , uint16_t hif_len);
#endif
void sys_buf_len_calc_using_hif_len(struct _SYS_BUF *packet);
int system_memory_pool_get_index(SYS_BUF* sbuf);

#endif //SYSTEM_MEMORY_MANAGER_H
