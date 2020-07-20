#ifndef HAL_LMAC_UTIL_H
#define HAL_LMAC_UTIL_H

uint32_t lmac_bytes_to_bufend(struct _SYS_BUF* sys_buf, uint8_t *ptr);
bool    lmac_send_null_frame();
bool	lmac_send_qos_null_frame();
uint8_t* lmac_get_macaddr_magic_code(void);
bool    lmac_check_sf_macaddr(uint8_t *mac, int vif_id);
#if defined(INCLUDE_TWT_SUPPORT)
void    lmac_ap_send_null_frame(void);
#endif /* defined(INCLUDE_TWT_SUPPORT) */
#if defined(STANDARD_11AH)
int util_str_to_bandwidth(const char *str);
int util_str_to_prim_ch_bandwidth(const char *str);
bool util_str_onoff(const char *str);
#endif /* defined(STANDARD_11AH) */
#endif
