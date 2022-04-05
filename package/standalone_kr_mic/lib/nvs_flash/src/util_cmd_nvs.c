
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

static void nvs_cli_info()
{
	nvs_stats_t nvs_stats;
	if (nvs_get_stats(NULL, &nvs_stats) != NVS_OK) {
		A("nvs info failed.\n");
	} else {
		A("=== nvs information ===\n");
		A("used entries    : %d\n", nvs_stats.used_entries);
		A("free entries    : %d\n", nvs_stats.free_entries);
		A("total entries   : %d\n", nvs_stats.total_entries);
		A("namespace count : %d\n", nvs_stats.namespace_count);
		A("=======================\n");
	}
}

static void nvs_cli_set(char *key, char *value)
{
 	nvs_handle_t nvs_handle;
 	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		if (nvs_set_str(nvs_handle, key, value) != NVS_OK) {
			A("nvs set %s failed.\n", key);
		} else {
			A("Added key : \"%s\", value: \"%s\" successfully.\n", key, value);
		}
		nvs_close(nvs_handle);
	} else {
		A("nvs open failed.\n");
	}
}

static void nvs_cli_set_u8(char *key, char *value)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		uint8_t number = (uint8_t) strtoul(value, NULL, 0);
		if (nvs_set_u8(nvs_handle, key, number) != NVS_OK) {
			A("nvs set %s failed.\n", key);
		} else {
			A("Added key : \"%s\", value: %u (0x%x) successfully.\n", key, number, number);
		}
		nvs_close(nvs_handle);
	} else {
		A("nvs open failed.\n");
	}
}

static void nvs_cli_set_i8(char *key, char *value)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		int8_t number = (int8_t) strtol(value, NULL, 0);
		if (nvs_set_i8(nvs_handle, key, number) != NVS_OK) {
			A("nvs set %s failed.\n", key);
		} else {
			A("Added key : \"%s\", value: %d (0x%x) successfully.\n", key, number, number);
		}
		nvs_close(nvs_handle);
	} else {
		A("nvs open failed.\n");
	}
}

static void nvs_cli_set_u16(char *key, char *value)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		uint16_t number = (uint16_t) strtoul(value, NULL, 0);
		if (nvs_set_u16(nvs_handle, key, number) != NVS_OK) {
			A("nvs set %s failed.\n", key);
		} else {
			A("Added key : \"%s\", value: %u (0x%x) successfully.\n", key, number, number);
		}
		nvs_close(nvs_handle);
	} else {
		A("nvs open failed.\n");
	}
}

static void nvs_cli_set_i16(char *key, char *value)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		int16_t number = (int16_t) strtol(value, NULL, 0);
		if (nvs_set_i16(nvs_handle, key, number) != NVS_OK) {
			A("nvs set %s failed.\n", key);
		} else {
			A("Added key : \"%s\", value: %d (0x%x) successfully.\n", key, number, number);
		}
		nvs_close(nvs_handle);
	} else {
		A("nvs open failed.\n");
	}
}

static void nvs_cli_set_u32(char *key, char *value)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		uint32_t number = (uint32_t) strtoul(value, NULL, 0);
		if (nvs_set_u32(nvs_handle, key, number) != NVS_OK) {
			A("nvs set %s failed.\n", key);
		} else {
			A("Added key : \"%s\", value: %u (0x%x) successfully.\n", key, number, number);
		}
		nvs_close(nvs_handle);
	} else {
		A("nvs open failed.\n");
	}
}

static void nvs_cli_set_i32(char *key, char *value)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		int32_t number = (int32_t) strtol(value, NULL, 0);
		if (nvs_set_i32(nvs_handle, key, number) != NVS_OK) {
			A("nvs set %s failed.\n", key);
		} else {
			A("Added key : \"%s\", value: %d (0x%x) successfully.\n", key, number, number);
		}
		nvs_close(nvs_handle);
	} else {
		A("nvs open failed.\n");
	}
}

static void nvs_cli_set_u64(char *key, char *value)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		uint64_t number = (uint64_t) strtoull(value, NULL, 0);
		if (nvs_set_u64(nvs_handle, key, number) != NVS_OK) {
			A("nvs set %s failed.\n", key);
		} else {
			A("Added key : \"%s\", value: %llu (0x%llx) successfully.\n", key, number, number);
		}
		nvs_close(nvs_handle);
	} else {
		A("nvs open failed.\n");
	}
}

static void nvs_cli_set_i64(char *key, char *value)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		int64_t number = (int64_t) strtoll(value, NULL, 0);
		if (nvs_set_i64(nvs_handle, key, number) != NVS_OK) {
			A("nvs set %s failed.\n", key);
		} else {
			A("Added key : \"%s\", value: %lld (0x%llx) successfully.\n", key, number, number);
		}
		nvs_close(nvs_handle);
	} else {
		A("nvs open failed.\n");
	}
}

static void nvs_cli_get(char *key)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READONLY, &nvs_handle) == NVS_OK) {
		char value[256];
		size_t length = sizeof(value);

		if (nvs_get_str(nvs_handle, key, value, &length) != NVS_OK) {
			A("nvs get \"%s\" failed.\n", key);
		} else {
			A("key: \"%s\", value: \"%s\", size: %d\n", key, value, length);
		}
		nvs_close(nvs_handle);
	} else {
		A("nvs open failed.\n");
	}
}

static void nvs_cli_get_u8(char *key)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READONLY, &nvs_handle) == NVS_OK) {
		uint8_t value;

		if (nvs_get_u8(nvs_handle, key, &value) != NVS_OK) {
			A("nvs get \"%s\" failed.\n", key);
		} else {
			A("key: \"%s\", value: %u (0x%x)\n", key, value, value);
		}
		nvs_close(nvs_handle);
	} else {
		A("nvs open failed.\n");
	}
}

static void nvs_cli_get_i8(char *key)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READONLY, &nvs_handle) == NVS_OK) {
		int8_t value;

		if (nvs_get_i8(nvs_handle, key, &value) != NVS_OK) {
			A("nvs get \"%s\" failed.\n", key);
		} else {
			A("key: \"%s\", value: %d (0x%x)\n", key, value, value);
		}
		nvs_close(nvs_handle);
	} else {
		A("nvs open failed.\n");
	}
}

static void nvs_cli_get_u16(char *key)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READONLY, &nvs_handle) == NVS_OK) {
		uint16_t value;

		if (nvs_get_u16(nvs_handle, key, &value) != NVS_OK) {
			A("nvs get \"%s\" failed.\n", key);
		} else {
			A("key: \"%s\", value: %u (0x%x)\n", key, value, value);
		}
		nvs_close(nvs_handle);
	} else {
		A("nvs open failed.\n");
	}
}

static void nvs_cli_get_i16(char *key)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READONLY, &nvs_handle) == NVS_OK) {
		int16_t value;

		if (nvs_get_i16(nvs_handle, key, &value) != NVS_OK) {
			A("nvs get \"%s\" failed.\n", key);
		} else {
			A("key: \"%s\", value: %d (0x%x)\n", key, value, value);
		}
		nvs_close(nvs_handle);
	} else {
		A("nvs open failed.\n");
	}
}

static void nvs_cli_get_u32(char *key)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READONLY, &nvs_handle) == NVS_OK) {
		uint32_t value;

		if (nvs_get_u32(nvs_handle, key, &value) != NVS_OK) {
			A("nvs get \"%s\" failed.\n", key);
		} else {
			A("key: \"%s\", value: %u (0x%x)\n", key, value, value);
		}
		nvs_close(nvs_handle);
	} else {
		A("nvs open failed.\n");
	}
}

static void nvs_cli_get_i32(char *key)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READONLY, &nvs_handle) == NVS_OK) {
		int32_t value;

		if (nvs_get_i32(nvs_handle, key, &value) != NVS_OK) {
			A("nvs get \"%s\" failed.\n", key);
		} else {
			A("key: \"%s\", value: %d (0x%x)\n", key, value, value);
		}
		nvs_close(nvs_handle);
	} else {
	A("nvs open failed.\n");
	}
}

static void nvs_cli_get_u64(char *key)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READONLY, &nvs_handle) == NVS_OK) {
		uint64_t value;

		if (nvs_get_u64(nvs_handle, key, &value) != NVS_OK) {
			A("nvs get \"%s\" failed.\n", key);
		} else {
			A("key: \"%s\", value: %llu (0x%llx)\n", key, value, value);
		}
		nvs_close(nvs_handle);
	} else {
		A("nvs open failed.\n");
	}
}

static void nvs_cli_get_i64(char *key)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READONLY, &nvs_handle) == NVS_OK) {
		int64_t value;

		if (nvs_get_i64(nvs_handle, key, &value) != NVS_OK) {
			A("nvs get \"%s\" failed.\n", key);
		} else {
			A("key: \"%s\", value: %lld (0x%llx)\n", key, value, value);
		}
		nvs_close(nvs_handle);
	} else {
		A("nvs open failed.\n");
	}
}

static void nvs_cli_show()
{
	nvs_handle_t nvs_handle;
	size_t used_entries = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READONLY, &nvs_handle) == NVS_OK) {
		if (nvs_get_used_entry_count(nvs_handle, &used_entries) == NVS_OK) {
		} else {
			A("nvs show failed.\n");
			return;
		}
	} else {
		A("nvs open failed.\n");
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
			A("key '%s', type ", info.key);
			switch(info.type) {
			case NVS_TYPE_U8:
				nvs_get_u8(nvs_handle, info.key, (uint8_t *) value);
				A("'u8', value '%u' \n", *((uint8_t *) value));
				break;
			case NVS_TYPE_I8:
				nvs_get_i8(nvs_handle, info.key, (int8_t *) value);
				A("'i8', value '%d' \n", *((int8_t *) value));
				break;
			case NVS_TYPE_U16:
				nvs_get_u16(nvs_handle, info.key, (uint16_t *) value);
				A("'u16', value '%u' \n", *((uint16_t *) value));
				break;
			case NVS_TYPE_I16:
				nvs_get_i16(nvs_handle, info.key, (int16_t *) value);
				A("'i16', value '%d' \n", *((int16_t *) value));
				break;
			case NVS_TYPE_U32:
				nvs_get_u32(nvs_handle, info.key, (uint32_t *) value);
				A("'u32', value '%u' \n", *((uint32_t *) value));
				break;
			case NVS_TYPE_I32:
				nvs_get_i32(nvs_handle, info.key, (int32_t *) value);
				A("'i32', value '%d' \n", *((int32_t *) value));
				break;
			case NVS_TYPE_U64:
				nvs_get_u64(nvs_handle, info.key, (uint64_t *) value);
				A("'u64', value '%llu' \n", *((uint8_t *) value));
				break;
			case NVS_TYPE_I64:
				nvs_get_i64(nvs_handle, info.key, (int64_t *) value);
				A("'i64', value '%ll' \n", *((int64_t *) value));
				break;
			case NVS_TYPE_STR:
				nvs_get_str(nvs_handle, info.key, (char *) value, &length);
				A("'string', value '%s' \n", (char *) value);
				break;
			case NVS_TYPE_BLOB:
				A("'blob' \n");
				break;
			default:
				A("'unknown' \n");
				break;
			}
		};
	} else {
		A("nvs show: no entry found...\n");
	}

	nvs_close(nvs_handle);
}

static void nvs_cli_delete(char *key)
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		if (nvs_erase_key(nvs_handle, key) != NVS_OK) {
			A("nvs delete failed for key \"%s\".\n", key);
		} else {
			A("key - \"%s\" has been removed successfully.\n", key);
		}
		nvs_close(nvs_handle);
	} else {
		A("nvs open failed.\n");
	}
}

static void nvs_cli_commit()
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		if (nvs_commit(nvs_handle) != NVS_OK) {
			A("nvs commit failed.\n");
		} else {
			A("nvs committed successfully.\n");
		}
		nvs_close(nvs_handle);
	} else {
		A("nvs open failed.\n");
	}
}

static void	nvs_cli_erase()
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		if (nvs_erase_all(nvs_handle) != NVS_OK) {
			A("nvs erase failed.\n");
		} else {
			A("nvs erased all.\n");
		}
		nvs_close(nvs_handle);
	} else {
		A("nvs open failed.\n");
	}
}

static void	nvs_cli_reset()
{
	if (nvs_flash_erase_partition(NVS_DEFAULT_PART_NAME) != NVS_OK) {
		A("nvs reset config partition failed.\n");
	} else {
		A("nvs config partition reset, restart system...\n");
	}
}

static void nvs_cli_dump()
{
	nvs_dump(NVS_DEFAULT_PART_NAME);
}

int nvs_cli_handler(cmd_tbl_t *t, int argc, char *argv[])
{
	if (argc < 2 || strcmp(argv[0], "nvs") != 0) {
		A("Usage: %s\n", NVS_CLI_USAGE);
		return CMD_RET_SUCCESS;
	}
	if (strcmp(argv[1], "info") == 0) {
		nvs_cli_info();
	} else if (strcmp(argv[1], "set") == 0) {
		if (argc < 4) {
			A("Usage: nvs set <key> <value>\n");
		} else {
			char buf[256] = {0,};
			char *tmp = buf;
			for(int i = 3; i < argc; i++){
				sprintf(tmp + strlen(tmp), "%s ", argv[i]);
			}
			*(tmp + strlen(tmp)-1) = '\0';

			nvs_cli_set(argv[2], buf);
		}
	} else if (strcmp(argv[1], "set_u8") == 0) {
		if (argc < 4) {
			A("Usage: nvs set_u8 <key> <value>\n");
		} else {
			nvs_cli_set_u8(argv[2], argv[3]);
		}
	} else if (strcmp(argv[1], "set_i8") == 0) {
		if (argc < 4) {
			A("Usage: nvs set_i8 <key> <value>\n");
		} else {
			nvs_cli_set_i8(argv[2], argv[3]);
		}
	} else if (strcmp(argv[1], "set_u16") == 0) {
		if (argc < 4) {
			A("Usage: nvs set_u16 <key> <value>\n");
		} else {
			nvs_cli_set_u16(argv[2], argv[3]);
		}
	} else if (strcmp(argv[1], "set_i16") == 0) {
		if (argc < 4) {
			A("Usage: nvs set_i16 <key> <value>\n");
		} else {
			nvs_cli_set_i16(argv[2], argv[3]);
		}
	} else if (strcmp(argv[1], "set_u32") == 0) {
		if (argc < 4) {
			A("Usage: nvs set_u32 <key> <value>\n");
		} else {
			nvs_cli_set_u32(argv[2], argv[3]);
		}
	} else if (strcmp(argv[1], "set_i32") == 0) {
		if (argc < 4) {
			A("Usage: nvs set_i32 <key> <value>\n");
		} else {
			nvs_cli_set_i32(argv[2], argv[3]);
		}
	} else if (strcmp(argv[1], "set_u64") == 0) {
		if (argc < 4) {
			A("Usage: nvs set_u64 <key> <value>\n");
		} else {
			nvs_cli_set_u64(argv[2], argv[3]);
		}
	} else if (strcmp(argv[1], "set_i64") == 0) {
		if (argc < 4) {
			A("Usage: nvs set_i64 <key> <value>\n");
		} else {
			nvs_cli_set_i64(argv[2], argv[3]);
		}
	} else if (strcmp(argv[1], "get") == 0) {
		if (argc != 3) {
			A("Usage: nvs get <key>\n");
		} else {
			nvs_cli_get(argv[2]);
		}
	} else if (strcmp(argv[1], "get_u8") == 0) {
		if (argc != 3) {
			A("Usage: nvs get_u8 <key>\n");
		} else {
			nvs_cli_get_u8(argv[2]);
		}
	} else if (strcmp(argv[1], "get_i8") == 0) {
		if (argc != 3) {
			A("Usage: nvs get_i8 <key>\n");
		} else {
			nvs_cli_get_i8(argv[2]);
		}
	} else if (strcmp(argv[1], "get_u16") == 0) {
		if (argc != 3) {
			A("Usage: nvs get_u16 <key>\n");
		} else {
			nvs_cli_get_u16(argv[2]);
		}
	} else if (strcmp(argv[1], "get_i16") == 0) {
		if (argc != 3) {
			A("Usage: nvs get_i16 <key>\n");
		} else {
			nvs_cli_get_i16(argv[2]);
		}
	} else if (strcmp(argv[1], "get_u32") == 0) {
		if (argc != 3) {
			A("Usage: nvs get_u32 <key>\n");
		} else {
			nvs_cli_get_u32(argv[2]);
		}
	} else if (strcmp(argv[1], "get_i32") == 0) {
		if (argc != 3) {
			A("Usage: nvs get_i32 <key>\n");
		} else {
			nvs_cli_get_i32(argv[2]);
		}
	} else if (strcmp(argv[1], "get_u64") == 0) {
		if (argc != 3) {
			A("Usage: nvs get_u64 <key>\n");
		} else {
			nvs_cli_get_u64(argv[2]);
		}
	} else if (strcmp(argv[1], "get_i64") == 0) {
		if (argc != 3) {
			A("Usage: nvs get_i64 <key>\n");
		} else {
			nvs_cli_get_i64(argv[2]);
		}
	} else if (strcmp(argv[1], "show") == 0) {
		nvs_cli_show();
	} else if (strcmp(argv[1], "delete") == 0) {
		if (argc != 3) {
			A("Usage: nvs delete <key>\n");
		} else {
			nvs_cli_delete(argv[2]);
		}
	} else if (strcmp(argv[1], "commit") == 0) {
		nvs_cli_commit();
	} else if (strcmp(argv[1], "erase") == 0) {
		nvs_cli_erase();
	} else if (strcmp(argv[1], "reset") == 0) {
		nvs_cli_reset();
	} else if (strcmp(argv[1], "dump") == 0) {
		nvs_cli_dump();
	} else {
		A("nvs invalid command\n");
		A("%s\n", NVS_CLI_USAGE);
	}
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
