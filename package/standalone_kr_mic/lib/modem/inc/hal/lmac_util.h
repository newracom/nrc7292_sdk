#ifndef HAL_LMAC_UTIL_H
#define HAL_LMAC_UTIL_H

uint32_t lmac_bytes_to_bufend(struct _SYS_BUF* sys_buf, uint8_t *ptr);
void    lmac_init_null_frame();
void    lmac_send_null_frame(void(*callback)());
bool	lmac_send_qos_null_frame(bool pm);
uint8_t* lmac_get_macaddr_magic_code(void);
bool    lmac_check_sf_macaddr(uint8_t *mac, int vif_id);
#if defined(INCLUDE_FRAG_FRAME)
bool fragment_frame(LMAC_TXBUF *first, const uint16_t fragSize, bool skip_free);
#endif /* INCLUDE_FRAG_FRAME */

uint32_t util_tsf_diff(uint32_t start_tsf, uint32_t end_tsf);
uint8_t hex_to_int(char c);

#if defined(INCLUDE_TWT_SUPPORT)
void    lmac_twt_send_pspoll();
void    lmac_twt_send_null_frame(uint8_t pm);
#endif /* defined(INCLUDE_TWT_SUPPORT) */
#if defined(STANDARD_11AH)
int util_str_to_bandwidth(const char *str);
int util_str_to_prim_ch_bandwidth(const char *str);
#endif /* defined(STANDARD_11AH) */
bool util_str_onoff(const char *str);
#endif
