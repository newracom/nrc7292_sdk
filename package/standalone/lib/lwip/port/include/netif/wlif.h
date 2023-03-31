#ifndef __WLIF_H__
#define __WLIF_H__

#include "driver_nrc.h"

err_t wlif_init( struct netif *netif );
void lwif_input(struct nrc_wpa_if* intf, void *buffer, int data_len);

#endif /* __WLIF_H__ */
