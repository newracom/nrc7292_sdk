
#ifdef SUPPORT_NVS_FLASH

#include "nrc_sdk.h"
#include "nvs_flash.h"

#define NVS_CLI_USAGE \
"\nnvs {show (prints keys and types stored)|\n\
    info (show statistics)|\n\
    set key value (to set string value)|\n\
    set_{i8,u8,i16,u16,i32,u32,i64,u64} key value (to set number value)|\n\
    get key (to get string value)|\n\
    get_{i8,u8,i16,u16,i32,u32,i64,u64} key (to get number value)|\n\
    delete key (to delete key and its value)|\n\
    commit (commit to flash - not used)|\n\
    erase (erase nvs contents)|\n\
    reset (erase flash area - low format)|\n\
    dump (dump nvs partition information)}"

extern void nvs_dump(const char *partName);

static void nvs_cli_info(char *key, char *value)
{
	(void)key;
	(void)value;

	nvs_stats_t nvs_stats;
	if (nvs_get_stats(NULL, &nvs_stats) != NVS_OK) {
		CPA("nvs info failed.\n");
	} else {
		CPA("=== nvs information ===\n");
		CPA("used entries    : %d\n", nvs_stats.used_entries);
		CPA("free entries    : %d\n", nvs_stats.free_entries);
		CPA("total entries   : %d\n", nvs_stats.total_entries);
		CPA("namespace count : %d\n", nvs_stats.namespace_count);
		CPA("=======================\n");
	}
}

static void nvs_cli_set(char *key, char *value)
{
 	nvs_handle_t nvs_handle;
 	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		if (nvs_set_str(nvs_handle, key, value) != NVS_OK) {
			CPA("nvs set %s failed.\n", key);
		} else {
			CPA("Added key : \"%s\", value: \"%s\" successfully.\n", key, value);
		}
		nvs_close(nvs_handle);
	} else {
		CPA("nvs open failed.\n");
	}
}

static void nvs_cli_set_u8(char *key, char *value)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		uint8_t number = (uint8_t) strtoul(value, NULL, 0);
		if (nvs_set_u8(nvs_handle, key, number) != NVS_OK) {
			CPA("nvs set %s failed.\n", key);
		} else {
			CPA("Added key : \"%s\", value: %u (0x%x) successfully.\n", key, number, number);
		}
		nvs_close(nvs_handle);
	} else {
		CPA("nvs open failed.\n");
	}
}

static void nvs_cli_set_i8(char *key, char *value)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		int8_t number = (int8_t) strtol(value, NULL, 0);
		if (nvs_set_i8(nvs_handle, key, number) != NVS_OK) {
			CPA("nvs set %s failed.\n", key);
		} else {
			CPA("Added key : \"%s\", value: %d (0x%x) successfully.\n", key, number, number);
		}
		nvs_close(nvs_handle);
	} else {
		CPA("nvs open failed.\n");
	}
}

static void nvs_cli_set_u16(char *key, char *value)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		uint16_t number = (uint16_t) strtoul(value, NULL, 0);
		if (nvs_set_u16(nvs_handle, key, number) != NVS_OK) {
			CPA("nvs set %s failed.\n", key);
		} else {
			CPA("Added key : \"%s\", value: %u (0x%x) successfully.\n", key, number, number);
		}
		nvs_close(nvs_handle);
	} else {
		CPA("nvs open failed.\n");
	}
}

static void nvs_cli_set_i16(char *key, char *value)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		int16_t number = (int16_t) strtol(value, NULL, 0);
		if (nvs_set_i16(nvs_handle, key, number) != NVS_OK) {
			CPA("nvs set %s failed.\n", key);
		} else {
			CPA("Added key : \"%s\", value: %d (0x%x) successfully.\n", key, number, number);
		}
		nvs_close(nvs_handle);
	} else {
		CPA("nvs open failed.\n");
	}
}

static void nvs_cli_set_u32(char *key, char *value)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		uint32_t number = (uint32_t) strtoul(value, NULL, 0);
		if (nvs_set_u32(nvs_handle, key, number) != NVS_OK) {
			CPA("nvs set %s failed.\n", key);
		} else {
			CPA("Added key : \"%s\", value: %u (0x%x) successfully.\n", key, number, number);
		}
		nvs_close(nvs_handle);
	} else {
		CPA("nvs open failed.\n");
	}
}

static void nvs_cli_set_i32(char *key, char *value)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		int32_t number = (int32_t) strtol(value, NULL, 0);
		if (nvs_set_i32(nvs_handle, key, number) != NVS_OK) {
			CPA("nvs set %s failed.\n", key);
		} else {
			CPA("Added key : \"%s\", value: %d (0x%x) successfully.\n", key, number, number);
		}
		nvs_close(nvs_handle);
	} else {
		CPA("nvs open failed.\n");
	}
}

static void nvs_cli_set_u64(char *key, char *value)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		uint64_t number = (uint64_t) strtoull(value, NULL, 0);
		if (nvs_set_u64(nvs_handle, key, number) != NVS_OK) {
			CPA("nvs set %s failed.\n", key);
		} else {
			CPA("Added key : \"%s\", value: %llu (0x%llx) successfully.\n", key, number, number);
		}
		nvs_close(nvs_handle);
	} else {
		CPA("nvs open failed.\n");
	}
}

static void nvs_cli_set_i64(char *key, char *value)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		int64_t number = (int64_t) strtoll(value, NULL, 0);
		if (nvs_set_i64(nvs_handle, key, number) != NVS_OK) {
			CPA("nvs set %s failed.\n", key);
		} else {
			CPA("Added key : \"%s\", value: %lld (0x%llx) successfully.\n", key, number, number);
		}
		nvs_close(nvs_handle);
	} else {
		CPA("nvs open failed.\n");
	}
}

static void nvs_cli_get(char *key, char *value)
{
	(void)value;

	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		char value[256];
		size_t length = sizeof(value);

		if (nvs_get_str(nvs_handle, key, value, &length) != NVS_OK) {
			CPA("nvs get \"%s\" failed.\n", key);
		} else {
			CPA("key: \"%s\", value: \"%s\", size: %d\n", key, value, length);
		}
		nvs_close(nvs_handle);
	} else {
		CPA("nvs open failed.\n");
	}
}

static void nvs_cli_get_u8(char *key, char *value)
{
	(void)value;

	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		uint8_t value;

		if (nvs_get_u8(nvs_handle, key, &value) != NVS_OK) {
			CPA("nvs get \"%s\" failed.\n", key);
		} else {
			CPA("key: \"%s\", value: %u (0x%x)\n", key, value, value);
		}
		nvs_close(nvs_handle);
	} else {
		CPA("nvs open failed.\n");
	}
}

static void nvs_cli_get_i8(char *key, char *value)
{
	(void)value;

	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		int8_t value;

		if (nvs_get_i8(nvs_handle, key, &value) != NVS_OK) {
			CPA("nvs get \"%s\" failed.\n", key);
		} else {
			CPA("key: \"%s\", value: %d (0x%x)\n", key, value, value);
		}
		nvs_close(nvs_handle);
	} else {
		CPA("nvs open failed.\n");
	}
}

static void nvs_cli_get_u16(char *key, char *value)
{
	(void)value;

	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		uint16_t value;

		if (nvs_get_u16(nvs_handle, key, &value) != NVS_OK) {
			CPA("nvs get \"%s\" failed.\n", key);
		} else {
			CPA("key: \"%s\", value: %u (0x%x)\n", key, value, value);
		}
		nvs_close(nvs_handle);
	} else {
		CPA("nvs open failed.\n");
	}
}

static void nvs_cli_get_i16(char *key, char *value)
{
	(void)value;

	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		int16_t value;

		if (nvs_get_i16(nvs_handle, key, &value) != NVS_OK) {
			CPA("nvs get \"%s\" failed.\n", key);
		} else {
			CPA("key: \"%s\", value: %d (0x%x)\n", key, value, value);
		}
		nvs_close(nvs_handle);
	} else {
		CPA("nvs open failed.\n");
	}
}

static void nvs_cli_get_u32(char *key, char *value)
{
	(void)value;

	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		uint32_t value;

		if (nvs_get_u32(nvs_handle, key, &value) != NVS_OK) {
			CPA("nvs get \"%s\" failed.\n", key);
		} else {
			CPA("key: \"%s\", value: %u (0x%x)\n", key, value, value);
		}
		nvs_close(nvs_handle);
	} else {
		CPA("nvs open failed.\n");
	}
}

static void nvs_cli_get_i32(char *key, char *value)
{
	(void)value;

	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		int32_t value;

		if (nvs_get_i32(nvs_handle, key, &value) != NVS_OK) {
			CPA("nvs get \"%s\" failed.\n", key);
		} else {
			CPA("key: \"%s\", value: %d (0x%x)\n", key, value, value);
		}
		nvs_close(nvs_handle);
	} else {
	CPA("nvs open failed.\n");
	}
}

static void nvs_cli_get_u64(char *key, char *value)
{
	(void)value;

	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		uint64_t value;

		if (nvs_get_u64(nvs_handle, key, &value) != NVS_OK) {
			CPA("nvs get \"%s\" failed.\n", key);
		} else {
			CPA("key: \"%s\", value: %llu (0x%llx)\n", key, value, value);
		}
		nvs_close(nvs_handle);
	} else {
		CPA("nvs open failed.\n");
	}
}

static void nvs_cli_get_i64(char *key, char *value)
{
	(void)value;

	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		int64_t value;

		if (nvs_get_i64(nvs_handle, key, &value) != NVS_OK) {
			CPA("nvs get \"%s\" failed.\n", key);
		} else {
			CPA("key: \"%s\", value: %lld (0x%llx)\n", key, value, value);
		}
		nvs_close(nvs_handle);
	} else {
		CPA("nvs open failed.\n");
	}
}

static void nvs_cli_show(char *key, char *value)
{
	(void)key;
	(void)value;

	nvs_handle_t nvs_handle;
	size_t used_entries = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		if (nvs_get_used_entry_count(nvs_handle, &used_entries) == NVS_OK) {
		} else {
			CPA("nvs show failed.\n");
			return;
		}
	} else {
		CPA("nvs open failed.\n");
		return;
	}

	if (used_entries != 0) {
		nvs_iterator_t it = nvs_entry_find(NVS_DEFAULT_PART_NAME, NVS_DEFAULT_NAMESPACE, NVS_TYPE_ANY);
		while (it != NULL) {
			nvs_entry_info_t info;
			char value[256];
			size_t length = sizeof(value);
			nvs_entry_info(it, &info);
			it = nvs_entry_next(it);
			CPA("key '%s', type ", info.key);
			switch(info.type) {
			case NVS_TYPE_U8:
				nvs_get_u8(nvs_handle, info.key, (uint8_t *) value);
				CPA("'u8', value '%u' \n", *((uint8_t *) value));
				break;
			case NVS_TYPE_I8:
				nvs_get_i8(nvs_handle, info.key, (int8_t *) value);
				CPA("'i8', value '%d' \n", *((int8_t *) value));
				break;
			case NVS_TYPE_U16:
				nvs_get_u16(nvs_handle, info.key, (uint16_t *) value);
				CPA("'u16', value '%u' \n", *((uint16_t *) value));
				break;
			case NVS_TYPE_I16:
				nvs_get_i16(nvs_handle, info.key, (int16_t *) value);
				CPA("'i16', value '%d' \n", *((int16_t *) value));
				break;
			case NVS_TYPE_U32:
				nvs_get_u32(nvs_handle, info.key, (uint32_t *) value);
				CPA("'u32', value '%u' \n", *((uint32_t *) value));
				break;
			case NVS_TYPE_I32:
				nvs_get_i32(nvs_handle, info.key, (int32_t *) value);
				CPA("'i32', value '%d' \n", *((int32_t *) value));
				break;
			case NVS_TYPE_U64:
				nvs_get_u64(nvs_handle, info.key, (uint64_t *) value);
				CPA("'u64', value '%llu' \n", *((uint8_t *) value));
				break;
			case NVS_TYPE_I64:
				nvs_get_i64(nvs_handle, info.key, (int64_t *) value);
				CPA("'i64', value '%ll' \n", *((int64_t *) value));
				break;
			case NVS_TYPE_STR:
				nvs_get_str(nvs_handle, info.key, (char *) value, &length);
				CPA("'string', value '%s' \n", (char *) value);
				break;
			case NVS_TYPE_BLOB:
				CPA("'blob' \n");
				break;
			default:
				CPA("'unknown' \n");
				break;
			}
		};
	} else {
		CPA("nvs show: no entry found...\n");
	}

	nvs_close(nvs_handle);
}

static void nvs_cli_delete(char *key, char *value)
{
	(void)value;

	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		if (nvs_erase_key(nvs_handle, key) != NVS_OK) {
			CPA("nvs delete failed for key \"%s\".\n", key);
		} else {
			CPA("key - \"%s\" has been removed successfully.\n", key);
		}
		nvs_close(nvs_handle);
	} else {
		CPA("nvs open failed.\n");
	}
}

static void nvs_cli_commit(char *key, char *value)
{
	(void)key;
	(void)value;

	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		if (nvs_commit(nvs_handle) != NVS_OK) {
			CPA("nvs commit failed.\n");
		} else {
			CPA("nvs committed successfully.\n");
		}
		nvs_close(nvs_handle);
	} else {
		CPA("nvs open failed.\n");
	}
}

static void	nvs_cli_erase(char *key, char *value)
{
	(void)key;
	(void)value;

	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		if (nvs_erase_all(nvs_handle) != NVS_OK) {
			CPA("nvs erase failed.\n");
		} else {
			CPA("nvs erased all.\n");
		}
		nvs_close(nvs_handle);
	} else {
		CPA("nvs open failed.\n");
	}
}

static void	nvs_cli_reset(char *key, char *value)
{
	(void)key;
	(void)value;

	if (nvs_flash_erase_partition(NVS_DEFAULT_PART_NAME) != NVS_OK) {
		CPA("nvs reset config partition failed.\n");
	} else {
		CPA("nvs config partition reset, restart system...\n");
	}
}

static void nvs_cli_dump(char *key, char *value)
{
	(void)key;
	(void)value;
	nvs_dump(NVS_DEFAULT_PART_NAME);
}

int nvs_cli_handler(cmd_tbl_t *t, int argc, char *argv[]) {
    if (argc < 2 || argc > 4 || strcmp(argv[0], "nvs") != 0 ) {
        CPA("Usage: %s\n", NVS_CLI_USAGE);
        return CMD_RET_SUCCESS;
    }

    static const struct {
        const char *cmd;
        void (*func)(char *, char *);
        const char *usage;
        int argc_num;
    } commands[] = {
        {"info", nvs_cli_info,  "nvs info", 2},
        {"set", nvs_cli_set, "nvs set <key> <value>", 4},
        {"set_u8", nvs_cli_set_u8, "nvs set_u8 <key> <value>", 4},
        {"set_i8", nvs_cli_set_i8, "nvs set_i8 <key> <value>", 4},
        {"set_u16", nvs_cli_set_u16, "nvs set_u16 <key> <value>", 4},
        {"set_i16", nvs_cli_set_i16, "nvs set_i16 <key> <value>", 4},
        {"set_u32", nvs_cli_set_u32, "nvs set_u32 <key> <value>", 4},
        {"set_i32", nvs_cli_set_i32, "nvs set_i32 <key> <value>", 4},
        {"set_u64", nvs_cli_set_u64, "nvs set_u64 <key> <value>", 4},
        {"set_i64", nvs_cli_set_i64, "nvs set_i64 <key> <value>", 4},
        {"get", nvs_cli_get, "nvs get <key>", 3},
        {"get_u8", nvs_cli_get_u8, "nvs get_u8 <key>", 3},
        {"get_i8", nvs_cli_get_i8, "nvs get_i8 <key>", 3},
        {"get_u16", nvs_cli_get_u16, "nvs get_u16 <key>", 3},
        {"get_i16", nvs_cli_get_i16, "nvs get_i16 <key>", 3},
        {"get_u32", nvs_cli_get_u32, "nvs get_u32 <key>", 3},
        {"get_i32", nvs_cli_get_i32, "nvs get_i32 <key>", 3},
        {"get_u64", nvs_cli_get_u64, "nvs get_u64 <key>", 3},
        {"get_i64", nvs_cli_get_i64, "nvs get_i64 <key>", 3},
        {"show", nvs_cli_show, "nvs show", 2},
        {"delete", nvs_cli_delete, "nvs delete <key>", 3},
        {"commit", nvs_cli_commit, "nvs commit", 2},
        {"erase", nvs_cli_erase, "nvs erase", 2},
        {"reset", nvs_cli_reset, "nvs reset", 2},
        {"dump", nvs_cli_dump, "nvs dump", 2}
    };

    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        if (strcmp(argv[1], commands[i].cmd) == 0) {
            if (argc != commands[i].argc_num) {
                if (commands[i].usage) {
                    CPA("Usage : %s\n", commands[i].usage);
                }
            } else if (commands[i].func) {
                commands[i].func(argc > 2 ? argv[2] : NULL, argc > 3 ? argv[3] : NULL);
            }
            return CMD_RET_SUCCESS;
        }
    }
    CPA("nvs invalid command\n");
    CPA("Usage: %s\n", NVS_CLI_USAGE);
    return CMD_RET_SUCCESS;
}


/* Declaration moved to util_cmd.c */
/*
CMD_MAND(nvs,
	nvs_cli_handler,
	"nvs key value read/write",
	NVS_CLI_USAGE);
*/
#endif /* SUPPORT_NVS_FLASH */
