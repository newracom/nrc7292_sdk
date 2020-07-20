#ifndef UMAC_S1G_CHANNEL_H
#define UMAC_S1G_CHANNEL_H

#if !defined(CUNIT)
#define MAX_NON_S1G_CHANNEL_NUM 45 	/* Max Num of S1G Channels */
#define MAX_S1G_OPER_CLASS 5		/* Max Num of Operation Class */
#define MAX_NON_S1G_CHANNEL_NUM_1M 17 	/* Max Num of S1G Channels */
#define MAX_NON_S1G_CHANNEL_NUM_2M 9 	/* Max Num of S1G Channels */
#define MAX_NON_S1G_CHANNEL_NUM_4M 4 	/* Max Num of S1G Channels */
//#define DEFAULT_INDEX 25
#define DEFAULT_INDEX 3			/* CH NUM => US:30 KR:17 TW:22 JP:12 EU:7 */
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

uint8_t GetS1GChannelNumber(uint8_t index);
uint32_t GetS1GCenterFreq(uint8_t index);
uint8_t CheckChannelIndex(uint8_t index);
uint8_t GetChannelIndexByS1GFreq(uint16_t s1g_freq);
uint16_t GetS1GFreqByChannelIndex(uint8_t index);
uint16_t GetNonS1GDefaultFreq(void);

bool SetChannelConf(const CHANNEL_MAPPING_TABLE *item);
uint16_t GetS1GFreq(const uint16_t nons1g_freq, int8_t* offset, int8_t* primary_loc);
const CHANNEL_MAPPING_TABLE* GetS1GDefaulTable(void);
const CHANNEL_MAPPING_TABLE *get_s1g_channel_item(uint8_t index);
const CHANNEL_MAPPING_TABLE* get_s1g_channel_item_by_s1g_freq(const uint16_t s1g_freq);
const CHANNEL_MAPPING_TABLE * get_s1g_channel_item_by_nons1g_freq(const uint16_t nons1g_freq);
const CHANNEL_MAPPING_TABLE *get_s1g_channel_item_by_channel_number(uint8_t index);
uint8_t APGetChID(const uint16_t nons1g_freq);
uint8_t APGetS1GChID(const uint16_t nons1g_freq);
uint8_t APGetOperClass(const uint8_t s1g_freq_index);
uint8_t STAGetNonS1GChNum(uint8_t center_freq_index);
uint16_t STAGetNoneS1GFreq(const uint16_t s1g_freq);
uint8_t GetS1GCCAType(uint16_t center_freq);
uint16_t GetNonS1GFreqFromNonS1GChNum(uint16_t nons1g_freq_index);
uint32_t get_non_s1g_center_freq_from_s1g_freq_index(uint8_t index);
bool CheckSupportNonS1GFreq(const uint16_t nons1g_freq);
void umac_set_country_code(uint8_t *country_code);
void UpdateChannelTable(uint16_t size, uint8_t *table);
void ShowChannelTable(uint16_t size);

#endif /* !defined(CUNIT) */
#endif    // UMAC_S1G_CHANNEL_H
