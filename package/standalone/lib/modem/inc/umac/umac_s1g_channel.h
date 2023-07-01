#ifndef UMAC_S1G_CHANNEL_H
#define UMAC_S1G_CHANNEL_H

#if defined(INCLUDE_NEW_CHANNEL_CTX)
#include "system.h"
#include "system_common.h"
#include "system_modem_api.h"
#include "umac_scan.h"
#include "lmac_ps_common.h"
#include "nrc-wim-types.h"
#endif

#if !defined(CUNIT)
#define MAX_NON_S1G_CHANNEL_NUM 45 	/* Max Num of S1G Channels */
#define MAX_S1G_OPER_CLASS 5		/* Max Num of Operation Class */
#define MAX_NON_S1G_CHANNEL_NUM_1M 17 	/* Max Num of S1G Channels */
#define MAX_NON_S1G_CHANNEL_NUM_2M 9 	/* Max Num of S1G Channels */
#define MAX_NON_S1G_CHANNEL_NUM_4M 4 	/* Max Num of S1G Channels */
#if defined(INCLUDE_BD_SUPPORT)
#define	MAX_MCS_LEVEL_NUM	11
#endif /* defined(INCLUDE_BD_SUPPORT) */
//#define DEFAULT_INDEX 25
#define DEFAULT_INDEX	0	/* CH NUM => US:30 KR:17(8/2) TW:22 JP:12 EU:7 */
#define BANDWIDTH_1M	1
#define BANDWIDTH_2M	2
#define BANDWIDTH_4M	4

typedef struct {
	uint16_t    s1g_freq;
	uint16_t    nons1g_freq;
	uint8_t     cca_level_type;
	uint8_t     s1g_freq_index;
	uint8_t     global_oper_class;
	uint8_t     chan_spacing;
	uint16_t    start_freq;
	uint16_t    nons1g_freq_index;
	int8_t      offset; 					/* for 1MHz bandwidth alignment */
	int8_t      primary_loc;				/* for 1MHz bandwidth alignment */
} CHANNEL_MAPPING_TABLE;

#if defined(INCLUDE_BD_SUPPORT)
typedef struct {
	uint8_t	index;
	uint8_t	tx_pwr_value[MAX_MCS_LEVEL_NUM];
} CH_TXPWR_INFO;
#endif /* defined(INCLUDE_BD_SUPPORT) */

#if defined(NRC_ROMLIB)
enum {
	US = 0,
	JP,
	K0,
	K1,
	TW,
	EU,
	CN,
	NZ,
	AU,
	K2,
	MAX_COUNTRY_CODE
};
// extern  CHANNEL_MAPPING_TABLE nons1g_cc_ch_mapping_table[MAX_COUNTRY_CODE][MAX_NON_S1G_CHANNEL_NUM];
#endif /* defined(NRC_ROMLIB) */

const CHANNEL_MAPPING_TABLE *get_channel_mapping_tables(int country_index);

uint8_t GetNumOfChannelCC(void);
uint8_t GetNumOfChannel(uint8_t ccid);
uint8_t GetS1GChannelNumber(uint8_t index);
uint32_t GetS1GCenterFreq(uint8_t index);
uint8_t CheckChannelIndex(uint8_t index);
uint8_t GetChannelIndexByS1GFreq(uint16_t s1g_freq);
uint8_t GetChannelSpacingByS1GFreq(uint16_t s1g_freq);
uint16_t GetS1GFreqByChannelIndex(uint8_t index);
uint16_t GetStartS1GFreqByChspacing(uint8_t spacing);
uint16_t GetEndS1GFreqByChspacing(uint8_t spacing);
uint16_t GetNonS1GDefaultFreq(void);
uint16_t GetStartIndexByBW(bw_t bw);
uint8_t GetS1GChIDFromS1GFreq(const uint16_t s1g_freq);

bool SetChannelConf(const CHANNEL_MAPPING_TABLE *item);
uint16_t GetS1GFreq(const uint16_t nons1g_freq, int8_t* offset, int8_t* primary_loc);
#if defined(INCLUDE_NEW_CHANNEL_CTX)
const struct s1g_channel_table* GetS1GDefaulTable(void);
const struct s1g_channel_table* GetS1GFirstTable(void);
const struct s1g_channel_table* get_s1g_channel_item(uint8_t index);
const struct s1g_channel_table* get_s1g_channel_item_by_s1g_freq(const uint16_t s1g_freq);
const struct s1g_channel_table* get_s1g_channel_item_by_nons1g_freq(const uint16_t s1g_freq);
const struct s1g_channel_table* get_s1g_channel_item_by_channel_number(uint8_t index);
#else
const CHANNEL_MAPPING_TABLE* GetS1GDefaulTable(void);
const CHANNEL_MAPPING_TABLE* GetS1GFirstTable(void);
const CHANNEL_MAPPING_TABLE *get_s1g_channel_item(uint8_t index);
const CHANNEL_MAPPING_TABLE* get_s1g_channel_item_by_s1g_freq(const uint16_t s1g_freq);
const CHANNEL_MAPPING_TABLE* get_s1g_channel_item_by_nons1g_freq(const uint16_t nons1g_freq);
const CHANNEL_MAPPING_TABLE *get_s1g_channel_item_by_channel_number(uint8_t index);
#endif /* defined(INCLUDE_NEW_CHANNEL_CTX) */
uint8_t APGetChID(const uint16_t nons1g_freq);
uint8_t APGetS1GChID(const uint16_t nons1g_freq);
uint8_t APGetOperClass(const uint8_t s1g_freq_index);
uint8_t STAGetS1GChNum(uint8_t center_freq_index);
uint16_t STAGetS1GFreq(const uint16_t nons1g_freq);
uint8_t STAGetNonS1GChNum(uint8_t center_freq_index);
uint16_t STAGetNonS1GFreq(const uint16_t s1g_freq);
uint16_t STAGetNonS1GFreqWithBw(const uint16_t s1g_freq, bw_t bw);
uint8_t GetS1GCCAType(uint16_t center_freq);
uint16_t GetNonS1GFreqFromNonS1GChNum(uint16_t nons1g_freq_index);
uint32_t get_non_s1g_center_freq_from_s1g_freq_index(uint8_t index);
bool CheckSupportNonS1GFreq(const uint16_t nons1g_freq);
void ShowChannelTable();
bool CheckSupportS1GFreq(const uint16_t s1g_freq);
bool SetChannelByIdx(uint8_t index);
#if defined(INCLUDE_NEW_CHANNEL_CTX)
bool umac_s1g_freq_check(uint16_t freq);
void umac_set_channel_table(const struct s1g_channel_table *cc_table, uint8_t chans_num);
#endif /*  defined(INCLUDE_NEW_CHANNEL_CTX) */
uint32_t GetS1GFreqByNonS1GFreq(const uint16_t nons1g_freq);
void UpdateCHTableByCC(const char* country_code);
void CheckNUpdateCHTableByVif(int vif_id);

#if defined(INCLUDE_BD_SUPPORT)
void setChannelTXPwrInfoValid(bool valid);
bool getChannelTXPwrInfoValid();
CH_TXPWR_INFO * getChannelTXPwrInfo(uint16_t ch);
void getChannelMask(uint32_t *l, uint32_t *h);
void setChannelMask(uint32_t l, uint32_t h);
#endif /* defined(INCLUDE_BD_SUPPORT) */


#endif /* !defined(CUNIT) */
#endif    // UMAC_S1G_CHANNEL_H
