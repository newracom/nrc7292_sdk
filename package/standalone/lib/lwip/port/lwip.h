#ifndef LWIP_H
#define LWIP_H
#if defined(INCLUDE_STANDALONE)
void lwif_input(uint8_t vif_id, void *buffer, int data_len, bool is_ap);
void reset_ip_address(int vif);
#else
static inline void lwif_input(uint8_t vif_id, void *buffer, int data_len, bool is_ap){}
static inline void reset_ip_address(int vif){}
#endif /* defined(INCLUDE_STANDALONE) */
#endif /* LWIP_H */
