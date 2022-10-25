// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "nvs_page.hpp"
#include <cstdio>
#include <cstring>

extern "C" uint32_t util_crc_compute_crc32(uint8_t *data, uint32_t length);
extern "C" void hal_uart_printf(const char *f, ...);

namespace nvs
{
static const char* TAG = "Page";

Page::Page() : mPartition(nullptr) { }

uint32_t Page::Header::calculateCrc32()
{
    return util_crc_compute_crc32(
                    reinterpret_cast<uint8_t*>(this) + offsetof(Header, mSeqNumber),
                    offsetof(Header, mCrc32) - offsetof(Header, mSeqNumber));
}

nvs_err_t Page::load(Partition *partition, uint32_t sectorNumber)
{
    if (partition == nullptr) {
        return NVS_ERR_INVALID_ARG;
    }

    mPartition = partition;
    mBaseAddress = sectorNumber * SEC_SIZE;
    mUsedEntryCount = 0;
    mErasedEntryCount = 0;

    Header header;
    auto rc = mPartition->read(mBaseAddress, &header, sizeof(header));
    if (rc != NVS_OK) {
        mState = PageState::INVALID;
        return rc;
    }
    if (header.mState == PageState::UNINITIALIZED) {
        mState = header.mState;
        // check if the whole page is really empty
        // reading the whole page takes ~40 times less than erasing it
        const int BLOCK_SIZE = 128;
        uint32_t* block = new (std::nothrow) uint32_t[BLOCK_SIZE];
		NVS_LOGD(TAG, "[%s] PageState::UNINITIALIZED...", __func__);

        if (!block) {
			NVS_LOGD(TAG, "[%s] block allocation failed, no mem...", __func__);
			return NVS_ERR_NO_MEM;
		}
        for (uint32_t i = 0; i < SPI_FLASH_SEC_SIZE; i += 4 * BLOCK_SIZE) {
			NVS_LOGD(TAG, "[%s] partition read addr 0x%x, block size * 4 = %d...", __func__, mBaseAddress + i, 4 * BLOCK_SIZE);
            rc = mPartition->read(mBaseAddress + i, block, 4 * BLOCK_SIZE);
            if (rc != NVS_OK) {
                mState = PageState::INVALID;
                delete[] block;
				NVS_LOGD(TAG, "[%s] partition read failed?...", __func__);
                return rc;
            }
            if (std::any_of(block, block + BLOCK_SIZE, [](uint32_t val) -> bool { return val != 0xffffffff; })) {
                // page isn't as empty after all, mark it as corrupted
                mState = PageState::CORRUPT;
				NVS_LOGD(TAG, "[%s] partition corrupt...", __func__);
                break;
            }
			NVS_LOGD(TAG, "[%s] i = %d, SPI_FLASH_SEC_SIZE", __func__, i, SPI_FLASH_SEC_SIZE);
        }
		NVS_LOGD(TAG, "[%s] delete block...", __func__);
        delete[] block;
		NVS_LOGD(TAG, "[%s] unintialized block check done...", __func__);

    } else if (header.mCrc32 != header.calculateCrc32()) {
		NVS_LOGD(TAG, "[%s] header crc error (%d != %d...", __func__, header.mCrc32, header.calculateCrc32());
        header.mState = PageState::CORRUPT;
    } else {
        mState = header.mState;
        mSeqNumber = header.mSeqNumber;
		NVS_LOGD(TAG, "[%s] state = %d, mSeqNumber = %d...", __func__, mState, mSeqNumber);
        if(header.mVersion < NVS_VERSION) {
            return NVS_ERR_NVS_NEW_VERSION_FOUND;
        } else {
            mVersion = header.mVersion;
        }
    }

    switch (mState) {
    case PageState::UNINITIALIZED:
		NVS_LOGD(TAG, "[%s] PageState::UNINITIALIZED", __func__);
        break;

    case PageState::FULL:
    case PageState::ACTIVE:
    case PageState::FREEING:
		NVS_LOGD(TAG, "[%s] Call mLoadEntryTable", __func__);
        mLoadEntryTable();
        break;

    default:
        mState = PageState::CORRUPT;
		NVS_LOGD(TAG, "[%s] PageState::CORRUPT", __func__);
        break;
    }
	NVS_LOGD(TAG, "[%s] returning OK...", __func__);

    return NVS_OK;
}

nvs_err_t Page::writeEntry(const Item& item)
{
    nvs_err_t err;

    err = mPartition->write(getEntryAddress(mNextFreeEntry), &item, sizeof(item));

    if (err != NVS_OK) {
        mState = PageState::INVALID;
        return err;
    }

    err = alterEntryState(mNextFreeEntry, EntryState::WRITTEN);
    if (err != NVS_OK) {
        return err;
    }

    if (mFirstUsedEntry == INVALID_ENTRY) {
        mFirstUsedEntry = mNextFreeEntry;
    }

    ++mUsedEntryCount;
    ++mNextFreeEntry;

    return NVS_OK;
}

nvs_err_t Page::writeEntryData(const uint8_t* data, size_t size)
{
    assert(size % ENTRY_SIZE == 0);
    assert(mNextFreeEntry != INVALID_ENTRY);
    assert(mFirstUsedEntry != INVALID_ENTRY);
    const uint16_t count = size / ENTRY_SIZE;

    const uint8_t* buf = data;

#if !defined LINUX_TARGET
    // TODO: check whether still necessary with esp_partition* API
    /* On the ESP32, data can come from DROM, which is not accessible by spi_flash_write
     * function. To work around this, we copy the data to heap if it came from DROM.
     * Hopefully this won't happen very often in practice. For data from DRAM, we should
     * still be able to write it to flash directly.
     * TODO: figure out how to make this platform-specific check nicer (probably by introducing
     * a platform-specific flash layer).
     */
    if ((uint32_t) data < 0x3ff00000) {
        buf = (uint8_t*) malloc(size);
        if (!buf) {
            return NVS_ERR_NO_MEM;
        }
        memcpy((void*)buf, data, size);
    }
#endif // ! LINUX_TARGET

    auto rc = mPartition->write(getEntryAddress(mNextFreeEntry), buf, size);

#if !defined LINUX_TARGET
    if (buf != data) {
        free((void*)buf);
    }
#endif // ! LINUX_TARGET
    if (rc != NVS_OK) {
        mState = PageState::INVALID;
        return rc;
    }
    auto err = alterEntryRangeState(mNextFreeEntry, mNextFreeEntry + count, EntryState::WRITTEN);
    if (err != NVS_OK) {
        return err;
    }
    mUsedEntryCount += count;
    mNextFreeEntry += count;
    return NVS_OK;
}

nvs_err_t Page::writeItem(uint8_t nsIndex, ItemType datatype, const char* key, const void* data, size_t dataSize, uint8_t chunkIdx)
{
    Item item;
    nvs_err_t err;

    if (mState == PageState::INVALID) {
        return NVS_ERR_NVS_INVALID_STATE;
    }

    if (mState == PageState::UNINITIALIZED) {
        err = initialize();
        if (err != NVS_OK) {
            return err;
        }
    }

    if (mState == PageState::FULL) {
        return NVS_ERR_NVS_PAGE_FULL;
    }

    const size_t keySize = strlen(key);
    if (keySize > Item::MAX_KEY_LENGTH) {
        return NVS_ERR_NVS_KEY_TOO_LONG;
    }

    if (dataSize > Page::CHUNK_MAX_SIZE) {
        return NVS_ERR_NVS_VALUE_TOO_LONG;
    }

    size_t totalSize = ENTRY_SIZE;
    size_t entriesCount = 1;
    if (isVariableLengthType(datatype)) {
        size_t roundedSize = (dataSize + ENTRY_SIZE - 1) & ~(ENTRY_SIZE - 1);
        totalSize += roundedSize;
        entriesCount += roundedSize / ENTRY_SIZE;
    }

    // primitive types should fit into one entry
    assert(totalSize == ENTRY_SIZE ||
       isVariableLengthType(datatype));

    if (mNextFreeEntry == INVALID_ENTRY || mNextFreeEntry + entriesCount > ENTRY_COUNT) {
        // page will not fit this amount of data
        return NVS_ERR_NVS_PAGE_FULL;
    }

    // write first item
    size_t span = (totalSize + ENTRY_SIZE - 1) / ENTRY_SIZE;
    item = Item(nsIndex, datatype, span, key, chunkIdx);
    err = mHashList.insert(item, mNextFreeEntry);

    if (err != NVS_OK) {
        return err;
    }

    if (!isVariableLengthType(datatype)) {
        memcpy(item.data, data, dataSize);
        item.crc32 = item.calculateCrc32();
        err = writeEntry(item);
        if (err != NVS_OK) {
            return err;
        }
    } else {
        const uint8_t* src = reinterpret_cast<const uint8_t*>(data);
        item.varLength.dataCrc32 = Item::calculateCrc32(src, dataSize);
        item.varLength.dataSize = dataSize;
        item.varLength.reserved = 0xffff;
        item.crc32 = item.calculateCrc32();
        err = writeEntry(item);
        if (err != NVS_OK) {
            return err;
        }

        size_t left = dataSize / ENTRY_SIZE * ENTRY_SIZE;
        if (left > 0) {
            err = writeEntryData(static_cast<const uint8_t*>(data), left);
            if (err != NVS_OK) {
                return err;
            }
        }

        size_t tail = dataSize - left;
        if (tail > 0) {
            std::fill_n(item.rawData, ENTRY_SIZE, 0xff);
            memcpy(item.rawData, static_cast<const uint8_t*>(data) + left, tail);
            err = writeEntry(item);
            if (err != NVS_OK) {
                return err;
            }
        }

    }
    return NVS_OK;
}

nvs_err_t Page::readItem(uint8_t nsIndex, ItemType datatype, const char* key, void* data, size_t dataSize, uint8_t chunkIdx, VerOffset chunkStart)
{
    size_t index = 0;
    Item item;

    if (mState == PageState::INVALID) {
        return NVS_ERR_NVS_INVALID_STATE;
    }

    nvs_err_t rc = findItem(nsIndex, datatype, key, index, item, chunkIdx, chunkStart);
    if (rc != NVS_OK) {
        return rc;
    }

    if (!isVariableLengthType(datatype)) {
        if (dataSize != getAlignmentForType(datatype)) {
            return NVS_ERR_NVS_TYPE_MISMATCH;
        }

        memcpy(data, item.data, dataSize);
        return NVS_OK;
    }

    if (dataSize < static_cast<size_t>(item.varLength.dataSize)) {
        return NVS_ERR_NVS_INVALID_LENGTH;
    }

    uint8_t* dst = reinterpret_cast<uint8_t*>(data);
    size_t left = item.varLength.dataSize;
    for (size_t i = index + 1; i < index + item.span; ++i) {
        Item ditem;
        rc = readEntry(i, ditem);
        if (rc != NVS_OK) {
            return rc;
        }
        size_t willCopy = ENTRY_SIZE;
        willCopy = (left < willCopy)?left:willCopy;
        memcpy(dst, ditem.rawData, willCopy);
        left -= willCopy;
        dst += willCopy;
    }
    if (Item::calculateCrc32(reinterpret_cast<uint8_t*>(data), item.varLength.dataSize) != item.varLength.dataCrc32) {
        rc = eraseEntryAndSpan(index);
        if (rc != NVS_OK) {
            return rc;
        }
        return NVS_ERR_NVS_NOT_FOUND;
    }
    return NVS_OK;
}

nvs_err_t Page::cmpItem(uint8_t nsIndex, ItemType datatype, const char* key, const void* data, size_t dataSize, uint8_t chunkIdx, VerOffset chunkStart)
{
    size_t index = 0;
    Item item;

    if (mState == PageState::INVALID) {
        return NVS_ERR_NVS_INVALID_STATE;
    }

    nvs_err_t rc = findItem(nsIndex, datatype, key, index, item, chunkIdx, chunkStart);
    if (rc != NVS_OK) {
        return rc;
    }

    if (!isVariableLengthType(datatype)) {
        if (dataSize != getAlignmentForType(datatype)) {
            return NVS_ERR_NVS_TYPE_MISMATCH;
        }

        if (memcmp(data, item.data, dataSize)) {
            return NVS_ERR_NVS_CONTENT_DIFFERS;
        }
        return NVS_OK;
    }

    if (dataSize < static_cast<size_t>(item.varLength.dataSize)) {
        return NVS_ERR_NVS_INVALID_LENGTH;
    }

    const uint8_t* dst = reinterpret_cast<const uint8_t*>(data);
    size_t left = item.varLength.dataSize;
    for (size_t i = index + 1; i < index + item.span; ++i) {
        Item ditem;
        rc = readEntry(i, ditem);
        if (rc != NVS_OK) {
            return rc;
        }
        size_t willCopy = ENTRY_SIZE;
        willCopy = (left < willCopy)?left:willCopy;
        if (memcmp(dst, ditem.rawData, willCopy)) {
            return NVS_ERR_NVS_CONTENT_DIFFERS;
        }
        left -= willCopy;
        dst += willCopy;
    }
    if (Item::calculateCrc32(reinterpret_cast<const uint8_t*>(data), item.varLength.dataSize) != item.varLength.dataCrc32) {
        return NVS_ERR_NVS_NOT_FOUND;
    }

    return NVS_OK;
}

nvs_err_t Page::eraseItem(uint8_t nsIndex, ItemType datatype, const char* key, uint8_t chunkIdx, VerOffset chunkStart)
{
    size_t index = 0;
    Item item;
    nvs_err_t rc = findItem(nsIndex, datatype, key, index, item, chunkIdx, chunkStart);
    if (rc != NVS_OK) {
        return rc;
    }
    return eraseEntryAndSpan(index);
}

nvs_err_t Page::findItem(uint8_t nsIndex, ItemType datatype, const char* key, uint8_t chunkIdx, VerOffset chunkStart)
{
    size_t index = 0;
    Item item;
    return findItem(nsIndex, datatype, key, index, item, chunkIdx, chunkStart);
}

nvs_err_t Page::eraseEntryAndSpan(size_t index)
{
    uint32_t seq_num;
    getSeqNumber(seq_num);
    auto state = mEntryTable.get(index);

    size_t span = 1;
    if (state == EntryState::WRITTEN) {
        Item item;
        auto rc = readEntry(index, item);
        if (rc != NVS_OK) {
            return rc;
        }
        if (item.calculateCrc32() != item.crc32) {
            mHashList.erase(index);
            rc = alterEntryState(index, EntryState::ERASED);
            --mUsedEntryCount;
            ++mErasedEntryCount;
            if (rc != NVS_OK) {
                return rc;
            }
        } else {
            mHashList.erase(index);
            span = item.span;
            for (ptrdiff_t i = index + span - 1; i >= static_cast<ptrdiff_t>(index); --i) {
                if (mEntryTable.get(i) == EntryState::WRITTEN) {
                    --mUsedEntryCount;
                }
                ++mErasedEntryCount;
            }
            if (span == 1) {
                rc = alterEntryState(index, EntryState::ERASED);
            } else {
                rc = alterEntryRangeState(index, index + span, EntryState::ERASED);
            }
            if (rc != NVS_OK) {
                return rc;
            }
        }
    } else {
        auto rc = alterEntryState(index, EntryState::ERASED);
        if (rc != NVS_OK) {
            return rc;
        }
    }

    if (index == mFirstUsedEntry) {
        updateFirstUsedEntry(index, span);
    }

    if (index + span > mNextFreeEntry) {
        mNextFreeEntry = index + span;
    }

    return NVS_OK;
}

void Page::updateFirstUsedEntry(size_t index, size_t span)
{
    assert(index == mFirstUsedEntry);
    mFirstUsedEntry = INVALID_ENTRY;
    size_t end = mNextFreeEntry;
    if (end > ENTRY_COUNT) {
        end = ENTRY_COUNT;
    }
    for (size_t i = index + span; i < end; ++i) {
        if (mEntryTable.get(i) == EntryState::WRITTEN) {
            mFirstUsedEntry = i;
            break;
        }
    }
}

nvs_err_t Page::copyItems(Page& other)
{
    if (mFirstUsedEntry == INVALID_ENTRY) {
        return NVS_ERR_NVS_NOT_FOUND;
    }

    if (other.mState == PageState::UNINITIALIZED) {
        auto err = other.initialize();
        if (err != NVS_OK) {
            return err;
        }
    }

    Item entry;
    size_t readEntryIndex = mFirstUsedEntry;

    while (readEntryIndex < ENTRY_COUNT) {

        if (mEntryTable.get(readEntryIndex) != EntryState::WRITTEN) {
            assert(readEntryIndex != mFirstUsedEntry);
            readEntryIndex++;
            continue;
        }
        auto err = readEntry(readEntryIndex, entry);
        if (err != NVS_OK) {
            return err;
        }

        err = other.mHashList.insert(entry, other.mNextFreeEntry);
        if (err != NVS_OK) {
            return err;
        }

        err = other.writeEntry(entry);
        if (err != NVS_OK) {
            return err;
        }
        size_t span = entry.span;
        size_t end = readEntryIndex + span;

        assert(end <= ENTRY_COUNT);

        for (size_t i = readEntryIndex + 1; i < end; ++i) {
            readEntry(i, entry);
            err = other.writeEntry(entry);
            if (err != NVS_OK) {
                return err;
            }
        }
        readEntryIndex = end;

    }
    return NVS_OK;
}

nvs_err_t Page::mLoadEntryTable()
{
    // for states where we actually care about data in the page, read entry state table
    if (mState == PageState::ACTIVE ||
            mState == PageState::FULL ||
            mState == PageState::FREEING) {
        auto rc = mPartition->read(mBaseAddress + ENTRY_TABLE_OFFSET, mEntryTable.data(),
                                 mEntryTable.byteSize());
        if (rc != NVS_OK) {
            mState = PageState::INVALID;
            return rc;
        }
    }

    mErasedEntryCount = 0;
    mUsedEntryCount = 0;
    for (size_t i = 0; i < ENTRY_COUNT; ++i) {
        auto s = mEntryTable.get(i);
        if (s == EntryState::WRITTEN) {
            if (mFirstUsedEntry == INVALID_ENTRY) {
                mFirstUsedEntry = i;
            }
            ++mUsedEntryCount;
        } else if (s == EntryState::ERASED) {
            ++mErasedEntryCount;
        }
    }

    // for PageState::ACTIVE, we may have more data written to this page
    // as such, we need to figure out where the first unused entry is
    if (mState == PageState::ACTIVE) {
        for (size_t i = 0; i < ENTRY_COUNT; ++i) {
            if (mEntryTable.get(i) == EntryState::EMPTY) {
                mNextFreeEntry = i;
                break;
            }
        }

        // however, if power failed after some data was written into the entry.
        // but before the entry state table was altered, the entry locacted via
        // entry state table may actually be half-written.
        // this is easy to check by reading EntryHeader (i.e. first word)
        while (mNextFreeEntry < ENTRY_COUNT) {
            uint32_t entryAddress = getEntryAddress(mNextFreeEntry);
            uint32_t header;
            auto rc = mPartition->read(entryAddress, &header, sizeof(header));
            if (rc != NVS_OK) {
                mState = PageState::INVALID;
                return rc;
            }
            if (header != 0xffffffff) {
                auto oldState = mEntryTable.get(mNextFreeEntry);
                auto err = alterEntryState(mNextFreeEntry, EntryState::ERASED);
                if (err != NVS_OK) {
                    mState = PageState::INVALID;
                    return err;
                }
                ++mNextFreeEntry;
                if (oldState == EntryState::WRITTEN) {
                    --mUsedEntryCount;
                }
                ++mErasedEntryCount;
            }
            else {
                break;
            }
        }

        // check that all variable-length items are written or erased fully
        Item item;
        size_t lastItemIndex = INVALID_ENTRY;
        size_t end = mNextFreeEntry;
        if (end > ENTRY_COUNT) {
            end = ENTRY_COUNT;
        }
        size_t span;
        for (size_t i = 0; i < end; i += span) {
            span = 1;
            if (mEntryTable.get(i) == EntryState::ERASED) {
                lastItemIndex = INVALID_ENTRY;
                continue;
            }

            if (mEntryTable.get(i) == EntryState::ILLEGAL) {
                lastItemIndex = INVALID_ENTRY;
                auto err = eraseEntryAndSpan(i);
                if (err != NVS_OK) {
                    mState = PageState::INVALID;
                    return err;
                }
                continue;
            }

            lastItemIndex = i;

            auto err = readEntry(i, item);
            if (err != NVS_OK) {
                mState = PageState::INVALID;
                return err;
            }

            if (item.crc32 != item.calculateCrc32()) {
                err = eraseEntryAndSpan(i);
                if (err != NVS_OK) {
                    mState = PageState::INVALID;
                    return err;
                }
                continue;
            }

            err = mHashList.insert(item, i);
            if (err != NVS_OK) {
                mState = PageState::INVALID;
                return err;
            }

            // search for potential duplicate item
            size_t duplicateIndex = mHashList.find(i, item);

            if (isVariableLengthType(item.datatype)) {
                span = item.span;
                bool needErase = false;
                for (size_t j = i; j < i + span; ++j) {
                    if (mEntryTable.get(j) != EntryState::WRITTEN) {
                        needErase = true;
                        lastItemIndex = INVALID_ENTRY;
                        break;
                    }
                }
                if (needErase) {
                    eraseEntryAndSpan(i);
                    continue;
                }
            }

            /* Note that logic for duplicate detections works fine even
             * when old-format blob is present along with new-format blob-index
             * for same key on active page. Since datatype is not used in hash calculation,
             * old-format blob will be removed.*/
            if (duplicateIndex < i) {
				NVS_LOGD(TAG, "[%s] OLD FORMAT erase: %d < %d ?...", __func__, duplicateIndex, i);
                eraseEntryAndSpan(duplicateIndex);
            }
        }

        // check that last item is not duplicate
        if (lastItemIndex != INVALID_ENTRY) {
            size_t findItemIndex = 0;
            Item dupItem;
            if (findItem(item.nsIndex, item.datatype, item.key, findItemIndex, dupItem) == NVS_OK) {
                if (findItemIndex < lastItemIndex) {
                    auto err = eraseEntryAndSpan(findItemIndex);
                    if (err != NVS_OK) {
                        mState = PageState::INVALID;
                        return err;
                    }
                }
            }
        }
    } else if (mState == PageState::FULL || mState == PageState::FREEING) {
        // We have already filled mHashList for page in active state.
        // Do the same for the case when page is in full or freeing state.
        Item item;
        for (size_t i = mFirstUsedEntry; i < ENTRY_COUNT; ++i) {
            if (mEntryTable.get(i) != EntryState::WRITTEN) {
                continue;
            }

            auto err = readEntry(i, item);
            if (err != NVS_OK) {
                mState = PageState::INVALID;
                return err;
            }

            if (item.crc32 != item.calculateCrc32()) {
                err = eraseEntryAndSpan(i);
                if (err != NVS_OK) {
                    mState = PageState::INVALID;
                    return err;
                }
                continue;
            }

            assert(item.span > 0);

            err = mHashList.insert(item, i);
            if (err != NVS_OK) {
                mState = PageState::INVALID;
                return err;
            }

            size_t span = item.span;

            if (isVariableLengthType(item.datatype)) {
                for (size_t j = i + 1; j < i + span; ++j) {
                    if (mEntryTable.get(j) != EntryState::WRITTEN) {
                        eraseEntryAndSpan(i);
                        break;
                    }
                }
            }

            i += span - 1;
        }

    }

    return NVS_OK;
}


nvs_err_t Page::initialize()
{
    assert(mState == PageState::UNINITIALIZED);
    mState = PageState::ACTIVE;
    Header header;
    header.mState = mState;
    header.mSeqNumber = mSeqNumber;
    header.mVersion = mVersion;
    header.mCrc32 = header.calculateCrc32();

    auto rc = mPartition->write(mBaseAddress, &header, sizeof(header));
    if (rc != NVS_OK) {
        mState = PageState::INVALID;
        return rc;
    }

    mNextFreeEntry = 0;
    std::fill_n(mEntryTable.data(), mEntryTable.byteSize() / sizeof(uint32_t), 0xffffffff);
    return NVS_OK;
}

nvs_err_t Page::alterEntryState(size_t index, EntryState state)
{
    assert(index < ENTRY_COUNT);
    mEntryTable.set(index, state);
    size_t wordToWrite = mEntryTable.getWordIndex(index);
    uint32_t word = mEntryTable.data()[wordToWrite];
    auto rc = mPartition->write(mBaseAddress + ENTRY_TABLE_OFFSET + static_cast<uint32_t>(wordToWrite) * 4,
            &word, sizeof(word));
    if (rc != NVS_OK) {
        mState = PageState::INVALID;
        return rc;
    }
    return NVS_OK;
}

nvs_err_t Page::alterEntryRangeState(size_t begin, size_t end, EntryState state)
{
    assert(end <= ENTRY_COUNT);
    assert(end > begin);
    size_t wordIndex = mEntryTable.getWordIndex(end - 1);
    for (ptrdiff_t i = end - 1; i >= static_cast<ptrdiff_t>(begin); --i) {
        mEntryTable.set(i, state);
        size_t nextWordIndex;
        if (i == static_cast<ptrdiff_t>(begin)) {
            nextWordIndex = (size_t) -1;
        } else {
            nextWordIndex = mEntryTable.getWordIndex(i - 1);
        }
        if (nextWordIndex != wordIndex) {
            uint32_t word = mEntryTable.data()[wordIndex];
            auto rc = mPartition->write(mBaseAddress + ENTRY_TABLE_OFFSET + static_cast<uint32_t>(wordIndex) * 4,
                    &word, 4);
            if (rc != NVS_OK) {
                return rc;
            }
        }
        wordIndex = nextWordIndex;
    }
    return NVS_OK;
}

nvs_err_t Page::alterPageState(PageState state)
{
    uint32_t state_val = static_cast<uint32_t>(state);
    auto rc = mPartition->write(mBaseAddress, &state_val, sizeof(state));
    if (rc != NVS_OK) {
        mState = PageState::INVALID;
        return rc;
    }
    mState = (PageState) state;
    return NVS_OK;
}

nvs_err_t Page::readEntry(size_t index, Item& dst) const
{
    auto rc = mPartition->read(getEntryAddress(index), &dst, sizeof(dst));
    if (rc != NVS_OK) {
        return rc;
    }
    return NVS_OK;
}

nvs_err_t Page::findItem(uint8_t nsIndex, ItemType datatype, const char* key, size_t &itemIndex, Item& item, uint8_t chunkIdx, VerOffset chunkStart)
{
    if (mState == PageState::CORRUPT || mState == PageState::INVALID || mState == PageState::UNINITIALIZED) {
        return NVS_ERR_NVS_NOT_FOUND;
    }

    size_t findBeginIndex = itemIndex;
    if (findBeginIndex >= ENTRY_COUNT) {
        return NVS_ERR_NVS_NOT_FOUND;
    }

    size_t start = mFirstUsedEntry;
    if (findBeginIndex > mFirstUsedEntry && findBeginIndex < ENTRY_COUNT) {
        start = findBeginIndex;
    }

    size_t end = mNextFreeEntry;
    if (end > ENTRY_COUNT) {
        end = ENTRY_COUNT;
    }

    if (nsIndex != NS_ANY && datatype != ItemType::ANY && key != NULL) {
        size_t cachedIndex = mHashList.find(start, Item(nsIndex, datatype, 0, key, chunkIdx));
        if (cachedIndex < ENTRY_COUNT) {
            start = cachedIndex;
        } else {
            return NVS_ERR_NVS_NOT_FOUND;
        }
    }

    size_t next;
    for (size_t i = start; i < end; i = next) {
        next = i + 1;
        if (mEntryTable.get(i) != EntryState::WRITTEN) {
            continue;
        }

        auto rc = readEntry(i, item);
        if (rc != NVS_OK) {
            mState = PageState::INVALID;
            return rc;
        }

        auto crc32 = item.calculateCrc32();
        if (item.crc32 != crc32) {
            rc = eraseEntryAndSpan(i);
            if (rc != NVS_OK) {
                mState = PageState::INVALID;
                return rc;
            }
            continue;
        }

        if (isVariableLengthType(item.datatype)) {
            next = i + item.span;
        }

        if (nsIndex != NS_ANY && item.nsIndex != nsIndex) {
            continue;
        }

        if (key != nullptr && strncmp(key, item.key, Item::MAX_KEY_LENGTH) != 0) {
            continue;
        }
        /* For blob data, chunkIndex should match*/
        if (chunkIdx != CHUNK_ANY
                && datatype == ItemType::BLOB_DATA
                && item.chunkIndex != chunkIdx) {
            continue;
        }
        /* Blob-index will match the <ns,key> with blob data.
         * Skip data chunks when searching for blob index*/
        if (datatype == ItemType::BLOB_IDX
                && item.chunkIndex != CHUNK_ANY) {
            continue;
        }
        /* Match the version for blob-index*/
        if (datatype == ItemType::BLOB_IDX
                && chunkStart != VerOffset::VER_ANY
                && item.blobIndex.chunkStart != chunkStart) {
            continue;
        }


        if (datatype != ItemType::ANY && item.datatype != datatype) {
            if (key == nullptr && nsIndex == NS_ANY && chunkIdx == CHUNK_ANY) {
                continue; // continue for bruteforce search on blob indices.
            }
            itemIndex = i;
            return NVS_ERR_NVS_TYPE_MISMATCH;
        }

        itemIndex = i;

        return NVS_OK;
    }

    return NVS_ERR_NVS_NOT_FOUND;
}

nvs_err_t Page::getSeqNumber(uint32_t& seqNumber) const
{
    if (mState != PageState::UNINITIALIZED && mState != PageState::INVALID && mState != PageState::CORRUPT) {
        seqNumber = mSeqNumber;
        return NVS_OK;
    }
    return NVS_ERR_NVS_NOT_INITIALIZED;
}


nvs_err_t Page::setSeqNumber(uint32_t seqNumber)
{
    if (mState != PageState::UNINITIALIZED) {
        return NVS_ERR_NVS_INVALID_STATE;
    }
    mSeqNumber = seqNumber;
    return NVS_OK;
}

nvs_err_t Page::setVersion(uint8_t ver)
{
    if (mState != PageState::UNINITIALIZED) {
        return NVS_ERR_NVS_INVALID_STATE;
    }
    mVersion = ver;
    return NVS_OK;
}

nvs_err_t Page::erase()
{
    auto rc = mPartition->erase_range(mBaseAddress, SPI_FLASH_SEC_SIZE);
    if (rc != NVS_OK) {
        mState = PageState::INVALID;
        return rc;
    }
    mUsedEntryCount = 0;
    mErasedEntryCount = 0;
    mFirstUsedEntry = INVALID_ENTRY;
    mNextFreeEntry = INVALID_ENTRY;
    mState = PageState::UNINITIALIZED;
    mHashList.clear();
    return NVS_OK;
}

nvs_err_t Page::markFreeing()
{
    if (mState != PageState::FULL && mState != PageState::ACTIVE) {
        return NVS_ERR_NVS_INVALID_STATE;
    }
    return alterPageState(PageState::FREEING);
}

nvs_err_t Page::markFull()
{
    if (mState != PageState::ACTIVE) {
        return NVS_ERR_NVS_INVALID_STATE;
    }
    return alterPageState(PageState::FULL);
}

size_t Page::getVarDataTailroom() const
{
    if (mState == PageState::UNINITIALIZED) {
        return CHUNK_MAX_SIZE;
    } else if (mState == PageState::FULL) {
        return 0;
    }
    /* Skip one entry for blob data item precessing the data */
    return ((mNextFreeEntry < (ENTRY_COUNT-1)) ? ((ENTRY_COUNT - mNextFreeEntry - 1) * ENTRY_SIZE): 0);
}

const char* Page::pageStateToName(PageState ps)
{
    switch (ps) {
        case PageState::CORRUPT:
            return "CORRUPT";

        case PageState::ACTIVE:
            return "ACTIVE";

        case PageState::FREEING:
            return "FREEING";

        case PageState::FULL:
            return "FULL";

        case PageState::INVALID:
            return "INVALID";

        case PageState::UNINITIALIZED:
            return "UNINITIALIZED";

        default:
            assert(0 && "invalid state value");
            return "";
    }
}

void Page::debugDump() const
{
    hal_uart_printf("state=%x (%s) addr=%x seq=%ld\nfirstUsed=%d nextFree=%d used=%d erased=%d\n",
		   (unsigned int) mState, pageStateToName(mState), (unsigned int) mBaseAddress, mSeqNumber, static_cast<int>(mFirstUsedEntry), static_cast<int>(mNextFreeEntry), mUsedEntryCount, mErasedEntryCount);
    size_t skip = 0;
    for (size_t i = 0; i < ENTRY_COUNT; ++i) {
        hal_uart_printf("%3d: ", static_cast<int>(i));
        EntryState state = mEntryTable.get(i);
        if (state == EntryState::EMPTY) {
            hal_uart_printf("E\n");
        } else if (state == EntryState::ERASED) {
            hal_uart_printf("X\n");
        } else if (state == EntryState::WRITTEN) {
            Item item;
            readEntry(i, item);
            if (skip == 0) {
                hal_uart_printf("W ns=%2u type=%2u span=%3u key=\"%s\" chunkIdx=%d len=%d\n", item.nsIndex, static_cast<unsigned>(item.datatype), item.span, item.key, item.chunkIndex, (item.span != 1)?((int)item.varLength.dataSize):-1);
                if (item.span > 0 && item.span <= ENTRY_COUNT - i) {
                    skip = item.span - 1;
                } else {
                    skip = 0;
                }
            } else {
                hal_uart_printf("D\n");
                skip--;
            }
        }
    }
}

nvs_err_t Page::calcEntries(nvs_stats_t &nvsStats)
{
    assert(mState != PageState::FREEING);

    nvsStats.total_entries += ENTRY_COUNT;

    switch (mState) {
        case PageState::UNINITIALIZED:
        case PageState::CORRUPT:
            nvsStats.free_entries += ENTRY_COUNT;
            break;

        case PageState::FULL:
        case PageState::ACTIVE:
            nvsStats.used_entries += mUsedEntryCount;
            nvsStats.free_entries += ENTRY_COUNT - mUsedEntryCount; // it's equivalent free + erase entries.
            break;

        case PageState::INVALID:
            return NVS_ERR_NVS_INVALID_STATE;
            break;

        default:
            assert(false && "Unhandled state");
            break;
    }
    return NVS_OK;
}

} // namespace nvs
