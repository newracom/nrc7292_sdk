#ifndef __CMD_H__
#define __CMD_H__
#include "system.h"

#define READ_WORD_SIZE      4

#define COPYRIGHT(year) "Copyright (c) "#year \
	" Newracom Inc. All rights reserved."

#define NRC_MAX_ARGV 16
#define NRC_MAX_CMDLINE_SIZE 128

#define CMD_RET_SUCCESS 	0
#define	CMD_RET_FAILURE 	1
#define CMD_RET_UNHANDLED 	2
#define CMD_RET_HAS_RESULT  10
#define	CMD_RET_USAGE  		-1


typedef struct nrc_cmd {
	const char *name;
	int (*handler)(struct nrc_cmd *t, int argc, char *argv[]);
	const char *desc;
	const char *usage;
	int flag;
} cmd_tbl_t;


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

#if !defined(RELEASE)
#define CMD(cmd, fn, d, u)				\
	ll_cmd_declare(cmd) = {				\
		.name = #cmd,				\
		.handler = fn, 				\
		.desc = d, 				\
		.usage = u,				\
		.flag = CMD_ATTR_TOP,	\
	}

#define SUBCMD(cmd, subcmd,fn, d, u)			\
	ll_subcmd_declare(cmd, subcmd) = { 		\
		.name = #subcmd,			\
		.handler = fn, 				\
		.desc = d,	 			\
		.usage = u,	 			\
		.flag = CMD_ATTR_SUB,	\
	}

#define SUBCMD_EXPL(cmd, subcmd,fn, d, u)			\
	ll_subcmd_declare(cmd, subcmd) = { 		\
		.name = #subcmd,			\
		.handler = fn, 				\
		.desc = d,	 			\
		.usage = u,	 			\
		.flag = CMD_ATTR_SUB | CMD_ATTR_EXPL,	\
	}
#else
#define CMD(cmd, fn, d, u)
#define SUBCMD(cmd, subcmd,fn, d, u)
#define SUBCMD_EXPL(cmd, subcmd,fn, d, u)
#endif	/* !defined(RELEASE) */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) sizeof(x)/sizeof(x[0])
#endif
#define OR "  or:  "

#define VERIFY_ARG_COUNT(n) \
	if (argc<n+1) return CMD_RET_USAGE;

struct option {
	const char *key;
	int val;
};

int util_cmd_getopt_long(int argc, char *argv[], struct option *options, int *index);
void util_cmd_set_result_buf(char *result);
int util_cmd_run_command(char *cmd);
int util_cmd_hex2num(char c);
int util_cmd_hex2byte(uint8_t *hex, uint8_t *byte);
int util_cmd_parse_hwaddr(char *string, uint8_t *addr);

#endif /* __CMD_H__ */
