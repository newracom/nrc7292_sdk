// Copyright 2019 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//         http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstdlib>
#include "nvs_partition.hpp"

namespace nvs {
static const char* TAG = "Partition";

NVSPartition::NVSPartition(uint32_t address, size_t size)
{
	mAddress = address;
	mSize = size;
}

const char *NVSPartition::get_partition_name()
{
	if (mAddress == SF_USER_CONFIG_1) {
		return "USER_CONFIG_1";
	} else {
		return "UNKNOWN Partition";
	}
}

nvs_err_t NVSPartition::read(size_t src_offset, void* dst, size_t size)
{
	uint32_t result = 0;

	NVS_LOGD(TAG, "[%s] nrc_sf_read reading addr = 0x%x, size = %d...", __func__, mAddress + src_offset, size);
	result = nrc_sf_read(mAddress + src_offset, (uint8_t *) dst, size);
	NVS_LOGD(TAG, "[%s] nrc_sf_read returns = %d...", __func__, result);
	if (size == result) {
		return NVS_OK;
	} else {
		return NVS_FAIL;
	}
}

nvs_err_t NVSPartition::write(size_t dst_offset, const void* src, size_t size)
{
	uint32_t result = 0;

	NVS_LOGD(TAG, "[%s] nrc_sf_write writing addr = 0x%x, size = %d...", __func__, mAddress + dst_offset, size);
	result = nrc_sf_write(mAddress + dst_offset, (uint8_t *) src, size);
	NVS_LOGD(TAG, "[%s] nrc_sf_read returns = %d...", __func__, result);

	if (size == result) {
		return NVS_OK;
	} else {
		return NVS_FAIL;
	}
}

nvs_err_t NVSPartition::erase_range(size_t dst_offset, size_t size)
{
	NVS_LOGD(TAG, "[%s] nrc_sf_erase erasing size = %d...", __func__, size);
	if (nrc_sf_erase(mAddress + dst_offset, size)) {
		NVS_LOGD(TAG, "[%s] nrc_sf_erase ok...", __func__);
		return NVS_OK;
	} else {
		NVS_LOGD(TAG, "[%s] nrc_sf_erase failed...", __func__);
		return NVS_FAIL;
	}
}

uint32_t NVSPartition::get_address()
{
    return mAddress;
}

uint32_t NVSPartition::get_size()
{
    return mSize;
}

} // nvs
