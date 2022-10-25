#ifndef __HAL_HOST_INTERFACE_H__
#define __HAL_HOST_INTERFACE_H__

#include "util_sysbuf_queue.h"
#include "util_byte_stream.h"

struct host_interface_ops {
	struct sysbuf_queue to_host_que;
	bool support_stream_mode;
	bool support_stream_align;

	/* below fields are filled by host interface implement */
	void *priv;
	void (*init)(void *priv);
	void (*deinit)(void *priv);
	void (*start)(void *priv);
#if !defined(NRC_ROMLIB)
	void (*reset)(int que);
#else
	void (*start_fw)();
#endif /* !defined(NRC_ROMLIB) */
	void (*stop)(void *priv);
	void (*request_to_host)(void *priv);
	void (*show)(void *priv);
	void (*update_credit)(void *priv, uint8_t ac, uint8_t value, bool inc);
	void (*update_slot)(void *priv, uint8_t value, bool inc);
	void (*loopback) (bool enable, bool verify);
	void (*send) (uint16_t length);
	bool (*verify_pattern) (SYS_BUF *head);
	int  (*core_dump) (struct byte_stream *bs, void *arg);
};
#if defined(NRC_ROMLIB)
#define HAL_HOST_INTERFACE_SEND_TO_HOST(a)  hal_host_interface_send_to_host(a)
#define HAL_HOST_INTERFACE_RX_DONE(a)		hal_host_interface_rx_done_fw(a)
void hal_host_interface_send_to_host(SYS_BUF *packet);
void hal_host_interface_rx_done_fw(SYS_BUF *packet);
#else
#define HAL_HOST_INTERFACE_SEND_TO_HOST(a)  hal_host_interface_send_to_host(a)
#define HAL_HOST_INTERFACE_RX_DONE(a)		hal_host_interface_rx_done(a)
void hal_host_interface_send_to_host(SYS_BUF *packet);
void hal_host_interface_rx_done(SYS_BUF *packet);
#endif /* defined(NRC_ROMLIB) */

void hal_host_interface_register_ops(struct host_interface_ops *ops);
void hal_host_interface_deinit();
void hal_host_interface_init();
void hal_host_interface_start();
void hal_host_interface_stop();
void hal_host_interface_reset();
void hal_host_interface_update_credit(uint8_t ac, uint8_t value, bool inc);
void hal_host_interface_update_slot(uint8_t value, bool inc);
void hal_host_interface_tx_enable(bool enable);
void hal_host_interface_loopback(bool enable, bool verify);
void hal_host_interface_send(uint16_t length);


#endif //__HAL_HOST_INTERFACE_H__
