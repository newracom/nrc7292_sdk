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

#include "nvs_types.hpp"

extern "C" uint32_t util_crc_compute_crc32(uint8_t *data, uint32_t length);

namespace nvs
{
uint32_t Item::calculateCrc32() const
{
    uint32_t result = 0xffffffff;
    const uint8_t* p = reinterpret_cast<const uint8_t*>(this);
#ifdef CRC32_COMPUTE_EXTRA_ENABLE
    result = util_crc_compute_crc32((uint8_t *) (p + offsetof(Item, nsIndex)),
									offsetof(Item, crc32) - offsetof(Item, nsIndex));
    result = util_crc_compute_crc32((uint8_t *) (p + offsetof(Item, key)), sizeof(key));
#endif /* CRC32_COMPUTE_EXTRA_ENABLE */
    result = util_crc_compute_crc32((uint8_t *) (p + offsetof(Item, data)), sizeof(data));
    return result;
}

uint32_t Item::calculateCrc32WithoutValue() const
{
    uint32_t result = 0xffffffff;
    const uint8_t* p = reinterpret_cast<const uint8_t*>(this);
#ifdef CRC32_COMPUTE_EXTRA_ENABLE
    result = util_crc_compute_crc32((uint8_t *) (p + offsetof(Item, nsIndex)),
                      offsetof(Item, datatype) - offsetof(Item, nsIndex));
    result = util_crc_compute_crc32((uint8_t *) (p + offsetof(Item, key)), sizeof(key));
#endif /* CRC32_COMPUTE_EXTRA_ENABLE */
    result = util_crc_compute_crc32((uint8_t *) (p + offsetof(Item, chunkIndex)), sizeof(chunkIndex));
    return result;
}

uint32_t Item::calculateCrc32(const uint8_t* data, size_t size)
{
    uint32_t result = 0xffffffff;
    result = util_crc_compute_crc32((uint8_t *) data, size);
    return result;
}

} // namespace nvs
