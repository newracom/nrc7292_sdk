#ifndef __PING_H__
#define __PING_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lwip/arch.h"
#include "lwip/ip_addr.h"
#include "arch/sys_arch.h"

/* PING_DEBUG: Disable debugging for PING.*/
#define PING_DEBUG     LWIP_DBG_ON
#define PING6_DEBUG     LWIP_DBG_OFF

/** ping delay - in milliseconds */
#ifndef PING_DELAY_DEFAULT
#define PING_DELAY_UNIT     1000  /* ms */
#define PING_DELAY_DEFAULT     1 * PING_DELAY_UNIT
#endif

#ifndef PING_DATA_SIZE
#define PING_DATA_SIZE     32
#endif

#define PING_COUNT_DEFAULT     5

typedef struct _ping_parm ping_parm_t;
struct _ping_parm
{
	u32_t packet_size;
	u32_t interval;
	u32_t target_count;
	u32_t time;
	u32_t count;
	u32_t success;
	u32_t total_delay;
	u32_t time_delay;
	u16_t seq_num;
	u16_t id;
	u8_t force_stop;
	sys_thread_t ping_thread;
	ip_addr_t addr;
	ping_parm_t* next;
};

void ping_thread(void *arg);
void ping6_thread(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* __PING_H__ */
