#include "system_common.h"
#include "lmac_debug.h"
#include "lmac_common.h"
#include "system_modem_api.h"
#include "lmac_drv.h"
#include "lmac_ps_common.h"
#include "umac_s1g_channel.h"
#include "umac_scan.h"

#define BW_TO_NUMBER(bw)    bw == BW_1M ? 1 : bw == BW_2M ? 2 : 4

struct cca_info_node
{
    uint16_t s1g_freq;
    uint16_t cca;
    uint16_t nons1g_freq_idx;
    uint8_t bw;
    struct cca_info_node * next;
};
struct self_conf_result
{
    int len;
    uint16_t best_nons1g_freq_idx;
    uint16_t best_s1g_freq;
    uint16_t best_cca;
    uint8_t best_bw;
    struct cca_info_node * head;
    struct cca_info_node * tail;
};

struct self_conf_result * self_config_scan_find_channel(int ccid, int pref_bw, int dwell_time);
bool self_config_get_status();