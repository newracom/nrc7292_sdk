#ifndef DRIVER_NRC_TX
#define DRIVER_NRC_TX

int nrc_transmit_from_8023(uint8_t vif_id,uint8_t *frame, const uint16_t len);
int nrc_transmit_from_8023_mb(uint8_t vif_id,uint8_t **frames, const uint16_t len[], int n_frames);

#endif // DRIVER_NRC_TX
