#include "nvs_partition_lookup.hpp"

#ifdef CONFIG_NVS_ENCRYPTION
#include "nvs_encrypted_partition.hpp"
#endif // CONFIG_NVS_ENCRYPTION

namespace nvs {

namespace partition_lookup {
static const char* TAG = "PartitionLookUp";

nvs_err_t lookup_nvs_partition(const char* label, NVSPartition **p)
{
	uint32_t address = 0;
	size_t size = MIN_PARTITION_SIZE;

	if (strcmp (label, "USER_CONFIG_1") == 0) {
		address = SF_USER_CONFIG_1;
	} else {
		NVS_LOGD(TAG, "[%s] NVS_ERR_NVS_PART_NOT_FOUND...", __func__);
		return NVS_ERR_NVS_PART_NOT_FOUND;
	}
	NVS_LOGD(TAG, "[%s] label %s, address = 0x%x...", __func__, label, address);
    NVSPartition *partition = new (std::nothrow) NVSPartition(address, size);
    if (partition == nullptr) {
		NVS_LOGD(TAG, "[%s] NVSPartition allocation failed, no mem...", __func__);
        return NVS_ERR_NO_MEM;
    }

    *p = partition;

    return NVS_OK;
}

#ifdef CONFIG_NVS_ENCRYPTION
nvs_err_t lookup_nvs_encrypted_partition(const char* label, nvs_sec_cfg_t* cfg, NVSPartition **p)
{
    const esp_partition_t* esp_partition = esp_partition_find_first(
            NVS_PARTITION_TYPE_DATA, NVS_PARTITION_SUBTYPE_DATA_NVS, label);

    if (esp_partition == nullptr) {
        return NVS_ERR_NOT_FOUND;
    }

    if (esp_partition->encrypted) {
        return NVS_ERR_NVS_WRONG_ENCRYPTION;
    }

    NVSEncryptedPartition *enc_p = new (std::nothrow) NVSEncryptedPartition(esp_partition);
    if (enc_p == nullptr) {
        return NVS_ERR_NO_MEM;
    }

    nvs_err_t result = enc_p->init(cfg);
    if (result != NVS_OK) {
        delete enc_p;
        return result;
    }

    *p = enc_p;

    return NVS_OK;
}

#endif // CONFIG_NVS_ENCRYPTION

} // partition_lookup

} // nvs
