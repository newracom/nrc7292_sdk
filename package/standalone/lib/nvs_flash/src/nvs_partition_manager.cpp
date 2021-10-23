// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "nvs_partition_manager.hpp"
#include "nvs_partition_lookup.hpp"

#ifdef CONFIG_NVS_ENCRYPTION
#include "nvs_encrypted_partition.hpp"
#endif // CONFIG_NVS_ENCRYPTION

namespace nvs {

static const char* TAG = "PartitionManager";

NVSPartitionManager* NVSPartitionManager::instance = nullptr;

NVSPartitionManager* NVSPartitionManager::get_instance()
{
    if (!instance) {
        instance = new (std::nothrow) NVSPartitionManager();
    }

    return instance;
}

nvs_err_t NVSPartitionManager::init_partition(const char *partition_label)
{
    if (strlen(partition_label) > NVS_PART_NAME_MAX_SIZE) {
        return NVS_ERR_INVALID_ARG;
    }

    uint32_t size;
    Storage* mStorage;

	NVS_LOGD(TAG, "[%s] calling lookup_storage_from_name with %s", __func__, partition_label);
    mStorage = lookup_storage_from_name(partition_label);
    if (mStorage) {
		NVS_LOGD(TAG, "[%s] Storage found...", __func__);
        return NVS_OK;
    }

    assert(SPI_FLASH_SEC_SIZE != 0);
	NVS_LOGD(TAG, "[%s] calling lookup_nvs_partition with %s", __func__, partition_label);
    NVSPartition *p = nullptr;
    nvs_err_t result = partition_lookup::lookup_nvs_partition(partition_label, &p);

    if (result != NVS_OK) {
        goto error;
    }

    size = p->get_size();
	NVS_LOGD(TAG, "[%s] calling init_custom...", __func__);
    result = init_custom(p, 0, size / SPI_FLASH_SEC_SIZE);
    if (result != NVS_OK) {
        goto error;
    }

    nvs_partition_list.push_back(p);

    return NVS_OK;

error:
    delete p;
    return result;
}

nvs_err_t NVSPartitionManager::init_custom(Partition *partition, uint32_t baseSector, uint32_t sectorCount)
{
    Storage* new_storage = nullptr;
    Storage* storage = lookup_storage_from_name(partition->get_partition_name());
    if (storage == nullptr) {
		NVS_LOGD(TAG, "[%s] Allocating new Storage...", __func__);
		new_storage = new (std::nothrow) Storage(partition);

        if (new_storage == nullptr) {
			NVS_LOGD(TAG, "[%s] Storage allocation failed, no memory...", __func__);
            return NVS_ERR_NO_MEM;
        }

        storage = new_storage;
    } else {
        // if storage was initialized already, we don't need partition and hence delete it
		NVS_LOGD(TAG, "[%s] storage already initialized, delete it...", __func__);
        for (auto it = nvs_partition_list.begin(); it != nvs_partition_list.end(); ++it) {
            if (partition == it) {
                nvs_partition_list.erase(it);
                delete partition;
                break;
            }
        }
    }
	NVS_LOGD(TAG, "[%s] Initialize storage by calling storage->init(%d, %d)...", __func__, baseSector, sectorCount);
    nvs_err_t err = storage->init(baseSector, sectorCount);
    if (new_storage != nullptr) {
        if (err == NVS_OK) {
			NVS_LOGD(TAG, "[%s] Storage to list ...", __func__);
            nvs_storage_list.push_back(new_storage);
        } else {
            delete new_storage;
        }
    }
    return err;
}

#ifdef CONFIG_NVS_ENCRYPTION
nvs_err_t NVSPartitionManager::secure_init_partition(const char *part_name, nvs_sec_cfg_t* cfg)
{
    if (strlen(part_name) > NVS_PART_NAME_MAX_SIZE) {
        return NVS_ERR_INVALID_ARG;
    }

    Storage* mStorage;

    mStorage = lookup_storage_from_name(part_name);
    if (mStorage != nullptr) {
        return NVS_OK;
    }

    NVSPartition *p;
    nvs_err_t result;
    if (cfg != nullptr) {
        result = partition_lookup::lookup_nvs_encrypted_partition(part_name, cfg, &p);
    } else {
        result = partition_lookup::lookup_nvs_partition(part_name, &p);
    }

    if (result != NVS_OK) {
        return result;
    }

    uint32_t size = p->get_size();

    result = init_custom(p, 0, size / SPI_FLASH_SEC_SIZE);
    if (result != NVS_OK) {
        delete p;
        return result;
    }

    nvs_partition_list.push_back(p);

    return NVS_OK;
}
#endif // CONFIG_NVS_ENCRYPTION

nvs_err_t NVSPartitionManager::deinit_partition(const char *partition_label)
{
    Storage* storage = lookup_storage_from_name(partition_label);
    if (!storage) {
        return NVS_ERR_NVS_NOT_INITIALIZED;
    }

    /* Clean up handles related to the storage being deinitialized */
    for (auto it = nvs_handles.begin(); it != nvs_handles.end(); ++it) {
        if (it->mStoragePtr == storage) {
            it->valid = false;
            nvs_handles.erase(it);
        }
    }

    /* Finally delete the storage and its partition */
    nvs_storage_list.erase(storage);
    delete storage;

    for (auto it = nvs_partition_list.begin(); it != nvs_partition_list.end(); ++it) {
        if (strcmp(it->get_partition_name(), partition_label) == 0) {
            NVSPartition *p = it;
            nvs_partition_list.erase(it);
            delete p;
            break;
        }
    }

    return NVS_OK;
}

nvs_err_t NVSPartitionManager::open_handle(const char *part_name,
        const char *ns_name,
        nvs_open_mode_t open_mode,
        NVSHandleSimple** handle)
{
    uint8_t nsIndex;
    Storage* sHandle;

    if (nvs_storage_list.empty()) {
		NVS_LOGD(TAG, "[%s] nvs_storage_list.empty, NVS_ERR_NVS_NOT_INITIALIZED...", __func__);
        return NVS_ERR_NVS_NOT_INITIALIZED;
    }

    sHandle = lookup_storage_from_name(part_name);
    if (sHandle == nullptr) {
		NVS_LOGD(TAG, "[%s] lookup_storage_from_name(%s) failed, NVS_ERR_NVS_PART_NOT_FOUND...", __func__, part_name);
        return NVS_ERR_NVS_PART_NOT_FOUND;
    }

    nvs_err_t err = sHandle->createOrOpenNamespace(ns_name, open_mode == NVS_READWRITE, nsIndex);
    if (err != NVS_OK) {
		NVS_LOGD(TAG, "[%s] createOrOpenNamespace failed...", __func__);
        return err;
    }

    *handle = new (std::nothrow) NVSHandleSimple(open_mode==NVS_READONLY, nsIndex, sHandle);

    if (handle == nullptr) {
		NVS_LOGD(TAG, "[%s] NVSHandleSimple failed, no mem...", __func__);
        return NVS_ERR_NO_MEM;
    }
	NVS_LOGD(TAG, "[%s] nvs_handles.push_back(*handle)", __func__);
    nvs_handles.push_back(*handle);

    return NVS_OK;
}

nvs_err_t NVSPartitionManager::close_handle(NVSHandleSimple* handle) {
    for (auto it = nvs_handles.begin(); it != nvs_handles.end(); ++it) {
        if (it == intrusive_list<NVSHandleSimple>::iterator(handle)) {
            nvs_handles.erase(it);
            return NVS_OK;
        }
    }

    return NVS_ERR_NVS_INVALID_HANDLE;
}

size_t NVSPartitionManager::open_handles_size()
{
    return nvs_handles.size();
}

Storage* NVSPartitionManager::lookup_storage_from_name(const char* name)
{
	NVS_LOGD(TAG, "[%s] name : %s", __func__, name);
    auto it = find_if(begin(nvs_storage_list), end(nvs_storage_list), [=](Storage& e) -> bool {
         NVS_LOGD(TAG, "[%s] e.getPartName() = %s, name = %s", __func__, e.getPartName(), name);
        return (strcmp(e.getPartName(), name) == 0);
    });

    if (it == end(nvs_storage_list)) {
		NVS_LOGD(TAG, "[%s] Storage name not found in nvs_storage_list...", __func__);
        return nullptr;
    }
    return it;
}

} // nvs
