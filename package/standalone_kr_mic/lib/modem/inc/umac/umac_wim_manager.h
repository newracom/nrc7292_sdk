#ifndef __UMAC_WIM_MANAGER_H__
#define __UMAC_WIM_MANAGER_H__

struct wim_manager_external_interface {
	void (*tx_enable)(bool enable);
	void (*send_to)(SYS_BUF *packet);
} __packed;

typedef SYS_BUF* (*wim_handler)(int vif_id, struct nrc_tlv**);
const wim_handler *umac_wim_manager_init();
void umac_wim_manager_run(int vif_id, uint16_t cmd, struct nrc_tlv **map);
void umac_wim_manager_run2(void *param);
bool umac_wim_manager_run_by_work_queue(void *param);
void umac_wim_manager_register_external_interface(struct wim_manager_external_interface *ext_if);
void umac_wim_manager_restart();
void umac_wim_manager_check_ps_abnormal();
void umac_wim_manager_inform_CSA();
void umac_wim_manager_inform_chan_switch();

#endif //__UMAC_WIM_MANAGER_H__
