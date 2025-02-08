#ifndef __ELOOP_FREERTOS_H__
#define __ELOOP_FREERTOS_H__

#define ELOOP_MESSAGE_SIZE (12)
#define ELOOP_MESSAGE_BUF_SIZE (512)
#define ELOOP_MESSAGE_COUNT (2)
#define ELOOP_MESSAGE_RSP_SIZE (256)
//-------------------------------------------------------------//
// README: Due to the possibility of causing malfunctions when 
//  directly calling wpa_supplicant's ctrl_iface, I switched 
//  to using Message Queue mechanism. 
//-------------------------------------------------------------//
extern QueueHandle_t eloop_message_queue_req;
extern QueueHandle_t eloop_message_queue_rsp;

typedef struct {
	int vif_id;
	uint32_t wait_rsp;
	char *buf;
} eloop_msg_t;

void eloop_run_signal();
#endif //__ELOOP_FREERTOS_H__
