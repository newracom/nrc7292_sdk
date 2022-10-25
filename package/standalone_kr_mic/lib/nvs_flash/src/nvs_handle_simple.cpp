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
#include <cstdlib>
#include "nvs_handle.hpp"
#include "nvs_partition_manager.hpp"

namespace nvs {
static const char *TAG = "handle_simple";

NVSHandleSimple::~NVSHandleSimple() {
    NVSPartitionManager::get_instance()->close_handle(this);
}

nvs_err_t NVSHandleSimple::set_typed_item(ItemType datatype, const char *key, const void* data, size_t dataSize)
{
    if (!valid) return NVS_ERR_NVS_INVALID_HANDLE;
    if (mReadOnly) return NVS_ERR_NVS_READ_ONLY;

    return mStoragePtr->writeItem(mNsIndex, datatype, key, data, dataSize);
}

nvs_err_t NVSHandleSimple::get_typed_item(ItemType datatype, const char *key, void* data, size_t dataSize)
{
    if (!valid) return NVS_ERR_NVS_INVALID_HANDLE;

    return mStoragePtr->readItem(mNsIndex, datatype, key, data, dataSize);
}

nvs_err_t NVSHandleSimple::set_string(const char *key, const char* str)
{
    if (!valid) return NVS_ERR_NVS_INVALID_HANDLE;
    if (mReadOnly) return NVS_ERR_NVS_READ_ONLY;

    return mStoragePtr->writeItem(mNsIndex, nvs::ItemType::SZ, key, str, strlen(str) + 1);
}

nvs_err_t NVSHandleSimple::set_blob(const char *key, const void* blob, size_t len)
{
    if (!valid) return NVS_ERR_NVS_INVALID_HANDLE;
    if (mReadOnly) return NVS_ERR_NVS_READ_ONLY;

    return mStoragePtr->writeItem(mNsIndex, nvs::ItemType::BLOB, key, blob, len);
}

nvs_err_t NVSHandleSimple::get_string(const char *key, char* out_str, size_t len)
{
    if (!valid) return NVS_ERR_NVS_INVALID_HANDLE;

    return mStoragePtr->readItem(mNsIndex, nvs::ItemType::SZ, key, out_str, len);
}

nvs_err_t NVSHandleSimple::get_blob(const char *key, void* out_blob, size_t len)
{
    if (!valid) return NVS_ERR_NVS_INVALID_HANDLE;

    return mStoragePtr->readItem(mNsIndex, nvs::ItemType::BLOB, key, out_blob, len);
}

nvs_err_t NVSHandleSimple::get_item_size(ItemType datatype, const char *key, size_t &size)
{
    if (!valid) return NVS_ERR_NVS_INVALID_HANDLE;

    return mStoragePtr->getItemDataSize(mNsIndex, datatype, key, size);
}

nvs_err_t NVSHandleSimple::erase_item(const char* key)
{
    if (!valid) return NVS_ERR_NVS_INVALID_HANDLE;
    if (mReadOnly) return NVS_ERR_NVS_READ_ONLY;

    return mStoragePtr->eraseItem(mNsIndex, key);
}

nvs_err_t NVSHandleSimple::erase_all()
{
    if (!valid) return NVS_ERR_NVS_INVALID_HANDLE;
    if (mReadOnly) return NVS_ERR_NVS_READ_ONLY;

    return mStoragePtr->eraseNamespace(mNsIndex);
}

nvs_err_t NVSHandleSimple::commit()
{
    if (!valid) return NVS_ERR_NVS_INVALID_HANDLE;

    nvs_err_t err = NVS_OK;
    uint8_t *buffer = nullptr;
    const Partition *partition = mStoragePtr->getPart();
    uint32_t address = mStoragePtr->getPartAddress();
    size_t size = mStoragePtr->getPartSize();
    NVS_LOGD(TAG, "[%s] Address: 0x%x, size: %d\n", __func__, address, size);

    buffer = new (std::nothrow) uint8_t[size];
    if (nrc_sf_read(address, buffer, size) == size) {
        if (nrc_sf_erase(address, size)) {
            if (nrc_sf_write(address, buffer, size) == size) {
				err = NVS_OK;
			} else {
				err = NVS_FAIL;
			}
        } else {
            err = NVS_FAIL;
        }
    } else {
        err = NVS_FAIL;
    }

	delete [] buffer;
    return err;
}

nvs_err_t NVSHandleSimple::get_used_entry_count(size_t& used_entries)
{
    used_entries = 0;

    if (!valid) return NVS_ERR_NVS_INVALID_HANDLE;

    size_t used_entry_count;
    nvs_err_t err = mStoragePtr->calcEntriesInNamespace(mNsIndex, used_entry_count);
    if(err == NVS_OK){
        used_entries = used_entry_count;
    }
    return err;
}

void NVSHandleSimple::debugDump() {
    return mStoragePtr->debugDump();
}

nvs_err_t NVSHandleSimple::fillStats(nvs_stats_t& nvsStats) {
    return mStoragePtr->fillStats(nvsStats);
}

nvs_err_t NVSHandleSimple::calcEntriesInNamespace(size_t& usedEntries) {
    return mStoragePtr->calcEntriesInNamespace(mNsIndex, usedEntries);
}

bool NVSHandleSimple::findEntry(nvs_opaque_iterator_t* it, const char* name) {
    return mStoragePtr->findEntry(it, name);
}

bool NVSHandleSimple::nextEntry(nvs_opaque_iterator_t* it) {
    return mStoragePtr->nextEntry(it);
}

const char *NVSHandleSimple::get_partition_name() const {
    return mStoragePtr->getPartName();
}

}
