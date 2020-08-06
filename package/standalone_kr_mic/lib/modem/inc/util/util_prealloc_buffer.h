#ifndef UTIL_PREALLOC_BUFFER_H
#define UTIL_PREALLOC_BUFFER_H

#include "system.h"
#include "util_list.h"

struct prealloc_buffer {
	bool activate;
	uint8_t num_required_tx_buffer;
	uint8_t num_required_rx_buffer;

	struct dl_list tx_buffer_list;
	struct dl_list rx_buffer_list;	
};

enum pb_free_ret {
	RET_PB_FREE_FAIL = 0,
	RET_SB_FREE_SUCC,
	RET_PB_FREE_SUCC
};

#define PB_INIT(a,b,c) util_prealloc_buffer_init(a,b,c)
#define PB_FINALIZE(a)	util_prealloc_buffer_finalize(a)
#define PB_AVAIL(a,b) util_prealloc_buffer_available(a,b)
#define PB_ALLOC(a,b) util_prealloc_buffer_alloc(a,b)
#define PB_FREE(a,b,c) util_prealloc_buffer_free(a,b,c)
#define PB_SHOW(a) util_prealloc_buffer_show(a)

bool util_prealloc_buffer_init(struct prealloc_buffer *pb, uint8_t num_tx_buffer, uint8_t num_rx_buffer);
void util_prealloc_buffer_finalize(struct prealloc_buffer *pb);
uint8_t util_prealloc_buffer_available(struct prealloc_buffer *pb, uint8_t pool_id);
struct _SYS_BUF* util_prealloc_buffer_alloc(struct prealloc_buffer *pb, uint8_t pool_id);
enum pb_free_ret util_prealloc_buffer_free(struct prealloc_buffer *pb, uint8_t pool_id, struct _SYS_BUF *buf);
void util_prealloc_buffer_show(struct prealloc_buffer *pb);

#endif
