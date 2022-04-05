SRCS += \
	nvs_partition.cpp \
	nvs_partition_lookup.cpp \
	nvs_partition_manager.cpp \
	nvs_api.cpp \
	nvs_cxx_api.cpp \
	nvs_handle_locked.cpp \
	nvs_handle_simple.cpp \
	nvs_item_hash_list.cpp \
	nvs_page.cpp \
	nvs_pagemanager.cpp \
	nvs_storage.cpp \
	nvs_types.cpp \
	cpp_config.cpp
#	nvs_encrypted_partition.cpp

CSRCS += util_cmd_nvs.c

# DEFINE += -DNVS_DEBUG
