#ifndef __CMD_H__
#define __CMD_H__

#include "system.h"

#define READ_WORD_SIZE	4
#define COPYRIGHT(year) "Copyright (c) "#year \
	" Newracom Inc. All rights reserved."
#define NRC_MAX_ARGV	16
#define NRC_MAX_CMDLINE_SIZE	128
#define	CMD_RET_SUCCESS	0
#define	CMD_RET_FAILURE	1
#define	CMD_RET_UNHANDLED	2
#define	CMD_RET_HAS_RESULT	10
#define	CMD_RET_USAGE	-1
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) sizeof(x)/sizeof(x[0])
#endif
#define OR "  or:  "
#define VERIFY_ARG_COUNT(n) if (argc<n+1) return CMD_RET_USAGE;

typedef struct nrc_cmd {
	const char *name;
	int (*handler)(struct nrc_cmd *t, int argc, char *argv[]);
	const char *desc;
	const char *usage;
	int flag;
} cmd_tbl_t;

struct option {
	const char *key;
	int val;
};

#if defined (INCLUDE_USE_CLI)
#define ll_cmd_declare(cmd)						\
	struct nrc_cmd _nrc_cmd_2_##cmd __aligned(4)			\
	__attribute__((used, section(".nrc_cmd_2_"#cmd)))

#define ll_cmd_start()							\
({									\
	static char start[0] __aligned(4) __attribute__((used,	\
		section(".nrc_cmd_1")));			\
	(struct nrc_cmd *)&start;					\
})

#define ll_cmd_end()							\
({									\
	static char end[0] __aligned(4) __attribute__((used,		\
		section(".nrc_cmd_3")));				\
	(struct nrc_cmd *)&end;						\
})

#define ll_subcmd_declare(cmd, subcmd)					\
	struct nrc_cmd _nrc_cmd_2_##cmd##_2_##subcmd __aligned(4)	\
	__attribute__((used, 						\
		       section(".nrc_cmd_2_"#cmd"_2_"#subcmd)))

#define ll_subcmd_start(cmd)						\
({									\
	static char start[0] __aligned(4) __attribute__((used,	\
		section(".nrc_cmd_2_"#cmd"_1")));			\
	(struct nrc_cmd *)&start;					\
})

#define ll_subcmd_end(cmd)						\
({									\
	static char end[0] __aligned(4) __attribute__((used,		\
		section(".nrc_cmd_2_"#cmd"_3")));			\
	(struct nrc_cmd *)&end;						\
})


#define for_each_subcmd(s, cmd)						\
	for (s = ll_subcmd_start(cmd); s < ll_subcmd_end(cmd); s++)


#define for_each_subcmd_opt(s, opt)						\
	for (struct option *s = opt; s->key != 0; s++)

#define CMD_ATTR_TOP 		(0)
#define CMD_ATTR_SUB 		(1)
#define CMD_ATTR_EXPL 		(2)

#if defined (INCLUDE_USE_CLI_MINIMUM)
#define CMD(cmd, fn, d, u)
#define SUBCMD(cmd, subcmd,fn, d, u)
#define SUBCMD_EXPL(cmd, subcmd,fn, d, u)

/* Minimum CLI Set (mandatory CLI SET) : CMD_MAND SUBCMD_MAND */
#define CMD_MAND(cmd, fn, d, u)	\
	ll_cmd_declare(cmd) = { 	\
		.name = #cmd,			\
		.handler = fn,			\
		.desc = d,				\
		.usage = u, 			\
		.flag = CMD_ATTR_TOP,	\
	}

#define SUBCMD_MAND(cmd, subcmd,fn, d, u)	\
	ll_subcmd_declare(cmd, subcmd) = {		\
		.name = #subcmd,					\
		.handler = fn,						\
		.desc = d,							\
		.usage = u, 						\
		.flag = CMD_ATTR_SUB,				\
	}
#else
#define CMD(cmd, fn, d, u)			\
	ll_cmd_declare(cmd) = {			\
		.name = #cmd,				\
		.handler = fn,				\
		.desc = d,					\
		.usage = u, 				\
		.flag = CMD_ATTR_TOP,		\
	}

#define SUBCMD(cmd, subcmd,fn, d, u)		\
	ll_subcmd_declare(cmd, subcmd) = { 		\
		.name = #subcmd,					\
		.handler = fn, 						\
		.desc = d,	 						\
		.usage = u,	 						\
		.flag = CMD_ATTR_SUB,				\
	}

#define SUBCMD_EXPL(cmd, subcmd,fn, d, u)		\
	ll_subcmd_declare(cmd, subcmd) = {			\
		.name = #subcmd							\
		.handler = fn,							\
		.desc = d,								\
		.usage = u,								\
		.flag = CMD_ATTR_SUB | CMD_ATTR_EXPL,	\
	}

#define CMD_MAND(cmd, fn, d, u)	CMD(cmd, fn, d, u)
#define SUBCMD_MAND(cmd, subcmd,fn, d, u)	SUBCMD(cmd, subcmd,fn, d, u)
#endif /* INCLUDE_USE_CLI_MINIMUM */

#if defined (INCLUDE_USE_CLI_HALOW)
/* CLI for HaLow */
#define CMD_HALOW(cmd, fn, d, u)	\
	ll_cmd_declare(cmd) = { 	\
		.name = #cmd,			\
		.handler = fn,			\
		.desc = d,				\
		.usage = u, 			\
		.flag = CMD_ATTR_TOP,	\
	}

#define SUBCMD_HALOW(cmd, subcmd,fn, d, u)	\
	ll_subcmd_declare(cmd, subcmd) = {		\
		.name = #subcmd,					\
		.handler = fn,						\
		.desc = d,							\
		.usage = u, 						\
		.flag = CMD_ATTR_SUB,				\
	}
#else
#define CMD_HALOW(cmd, fn, d, u)	CMD(cmd, fn, d, u)
#define SUBCMD_HALOW(cmd, subcmd,fn, d, u)	SUBCMD(cmd, subcmd,fn, d, u)
#endif /* INCLUDE_USE_CLI_HALOW */

void util_cmd_set_raw_res_len(int length);
int util_cmd_get_raw_res_len();
int util_cmd_getopt_long(int argc, char *argv[], struct option *options, int *index);
void util_cmd_set_result_buf(char *result);
int util_cmd_run_command(char *cmd);
int util_cmd_hex2num(char c);
int util_cmd_hex2byte(uint8_t *hex, uint8_t *byte);
int util_cmd_parse_hwaddr(char *string, uint8_t *addr);

#else //#if defined (INCLUDE_USE_CLI)
/* Not Support All CLI */
#define CMD(cmd, fn, d, u)
#define CMD_MAND(cmd, fn, d, u)
#define CMD_HALOW(cmd, fn, d, u)
#define SUBCMD(cmd, subcmd,fn, d, u)
#define SUBCMD_MAND(cmd, subcmd,fn, d, u)
#define SUBCMD_HALOW(cmd, subcmd,fn, d, u)
#define SUBCMD_EXPL(cmd, subcmd,fn, d, u)
#define SUBCMD_HALOW(cmd, subcmd,fn, d, u)

#if !defined (CUNIT)
static inline void util_cmd_set_raw_res_len(int length){};
#endif //#if !defined (CUNIT)
static inline int util_cmd_get_raw_res_len(){return 0;};
static inline void util_cmd_set_result_buf(char *result){};
#if defined(INCLUDE_COMPACT_CLI)
#include "hal_uart.h"
int compact_cmd_set_report(int on);
int compact_show_config();
int compact_phy_txgain(uint32_t txgain);
int compact_phy_rxgain(uint32_t rxgain);
int compact_test_agg(int ac, uint32_t maxagg_num);
int compact_sniffer(uint32_t val);
int compact_mm();
int compact_read_reg(uint32_t addr);
int compact_write_reg(uint32_t addr, uint32_t val);
int compact_agg(uint32_t val);
int compact_rc(uint32_t val);
int compact_mcs(uint32_t val);
int compact_show_tx_stats();
int compact_test_snr(uint8_t val);
int compact_set_freq_test(uint32_t val);
static inline int util_cmd_run_command(char *cmd)
{	
	uint32_t arg;
	int ac;
	switch(*cmd)
	{
		case 'A':			
			compact_cmd_set_report(1);
			break;
		case 'B':
			compact_cmd_set_report(0);
			break;
		case 'C':
			compact_show_config();
			break;
		case 'D':
			compact_show_tx_stats();
			break;
		case 'E':
			arg = atoi(cmd+2);
			compact_rc(arg);
			break;
		case 'G':
			arg = atoi(cmd+2);
			compact_agg(arg);
			break;
		case 'M':
			arg = atoi(cmd+2);
			compact_mcs(arg);
			break;
		case 'N':
			ac = atoi(cmd+2);
			arg = atoi(cmd+4);
			compact_test_agg(ac, arg);
			break;
		case 'P':
			arg = atoi(cmd+2);
			compact_test_snr(arg);
			break;
		case 'R':
			arg = atoi(cmd+2);	
			compact_phy_rxgain(arg);
			break;
		case 'S':
			arg = atoi(cmd+2);	
			compact_sniffer(arg);
			break;
		case 'T':
			arg = atoi(cmd+2);	
			compact_phy_txgain(arg);
			break;
		case 'U':
			compact_mm();
			break;
		case 'V':
			arg = atoi(cmd+2);
			compact_read_reg(arg);
			break;
		case 'W':		
			arg = atoi(cmd+2);
			uint32_t arg2 = atoi(cmd+13);
			compact_write_reg(arg, arg2);
			break;
		case 'Z':
			arg = atoi(cmd+2);
			compact_set_freq_test(arg);
			break;
			
		default:
			return 0;
	}
	return 0;
};
#else
static inline int util_cmd_run_command(char *cmd){return 0;};
#endif /* defined(INCLUDE_COMPACT_CLI) */
static inline int util_cmd_hex2num(char c){return 0;};
static inline int util_cmd_hex2byte(uint8_t *hex, uint8_t *byte){return 0;};
static inline int util_cmd_parse_hwaddr(char *string, uint8_t *addr){return 0;};

#endif //#if defined (INCLUDE_USE_CLI)

#endif /* __CMD_H__ */
