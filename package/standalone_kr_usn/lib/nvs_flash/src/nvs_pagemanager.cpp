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
#include "nvs_pagemanager.hpp"

namespace nvs
{
static const char* TAG = "PageManager";

nvs_err_t PageManager::load(Partition *partition, uint32_t baseSector, uint32_t sectorCount)
{
    if (partition == nullptr) {
        return NVS_ERR_INVALID_ARG;
    }

    mBaseSector = baseSector;
    mPageCount = sectorCount;
    mPageList.clear();
    mFreePageList.clear();
    mPages.reset(new (nothrow) Page[sectorCount]);

	NVS_LOGD(TAG, "[%s] mBaseSector = %d, mPageCount = %d", __func__, mBaseSector, mPageCount);

    if (!mPages) {
		NVS_LOGD(TAG, "[%s] mPages null, no mem...", __func__);
		return NVS_ERR_NO_MEM;
	}

    for (uint32_t i = 0; i < sectorCount; ++i) {
		NVS_LOGD(TAG, "[%s] mPages[%d] loading...", __func__, i);
        auto err = mPages[i].load(partition, baseSector + i);
        if (err != NVS_OK) {
			NVS_LOGD(TAG, "[%s] mPages[%d].load failed...", __func__, i);
            return err;
        }
        uint32_t seqNumber;
		NVS_LOGD(TAG, "[%s] mPages[%d].getSeqNumber...", __func__, i);
        if (mPages[i].getSeqNumber(seqNumber) != NVS_OK) {
			NVS_LOGD(TAG, "[%s] mPages[%d].getSeqNumber failed, putting mPages[%d] to mFreePageList...", __func__, i, i);
			mFreePageList.push_back(&mPages[i]);
        } else {
			NVS_LOGD(TAG, "[%s] mPages[%d].getSeqNumber (%d) was ok, ...", __func__, i, seqNumber);
            auto pos = std::find_if(std::begin(mPageList), std::end(mPageList), [=](const Page& page) -> bool {
                uint32_t otherSeqNumber;
                return page.getSeqNumber(otherSeqNumber) == NVS_OK && otherSeqNumber > seqNumber;
            });
			NVS_LOGD(TAG, "[%s] mPages[%d].loop next, ...", __func__, i);
            if (pos == mPageList.end()) {
				NVS_LOGD(TAG, "[%s] pos : %d == mPageList.end(), push_back", __func__, pos);
                mPageList.push_back(&mPages[i]);
            } else {
				NVS_LOGD(TAG, "[%s] pos : %d != mPageList.end(), insert", __func__, pos);
                mPageList.insert(pos, &mPages[i]);
            }
        }
    }

    if (mPageList.empty()) {
        mSeqNumber = 0;
		NVS_LOGD(TAG, "[%s] mPageList empty, mSeqNumber = %d, activatePage...", __func__, mSeqNumber);
		return activatePage();
    } else {
        uint32_t lastSeqNo;
        NVS_ERROR_CHECK( mPageList.back().getSeqNumber(lastSeqNo) );
        mSeqNumber = lastSeqNo + 1;
		NVS_LOGD(TAG, "[%s] mPageList not empty, mSeqNumber = %d", __func__, mSeqNumber);
    }

    // if power went out after a new item for the given key was written,
    // but before the old one was erased, we end up with a duplicate item
    Page& lastPage = back();
    size_t lastItemIndex = SIZE_MAX;
    Item item;
    size_t itemIndex = 0;
    while (lastPage.findItem(Page::NS_ANY, ItemType::ANY, nullptr, itemIndex, item) == NVS_OK) {
        itemIndex += item.span;
        lastItemIndex = itemIndex;
    }

    if (lastItemIndex != SIZE_MAX) {
        auto last = PageManager::TPageListIterator(&lastPage);
        TPageListIterator it;
		NVS_LOGD(TAG, "[%s] lastItemIndex(%d) != SIZE_MAX(%d)", __func__, lastItemIndex, SIZE_MAX);
        for (it = begin(); it != last; ++it) {

            if ((it->state() != Page::PageState::FREEING) &&
                    (it->eraseItem(item.nsIndex, item.datatype, item.key, item.chunkIndex) == NVS_OK)) {
                break;
            }
        }
        if ((it == last) && (item.datatype == ItemType::BLOB_IDX)) {
            /* Rare case in which the blob was stored using old format, but power went just after writing
             * blob index during modification. Loop again and delete the old version blob*/
            for (it = begin(); it != last; ++it) {

                if ((it->state() != Page::PageState::FREEING) &&
                        (it->eraseItem(item.nsIndex, ItemType::BLOB, item.key, item.chunkIndex) == NVS_OK)) {
                    break;
                }
            }
        }
    }

    // check if power went out while page was being freed
	NVS_LOGD(TAG, "[%s] Checking if power went out while page was being freed...", __func__);
    for (auto it = begin(); it!= end(); ++it) {
		NVS_LOGD(TAG, "[%s] page state : %d", __func__, it->state());
        if (it->state() == Page::PageState::FREEING) {
            Page* newPage = &mPageList.back();
            if (newPage->state() == Page::PageState::ACTIVE) {
                auto err = newPage->erase();
                if (err != NVS_OK) {
					NVS_LOGD(TAG, "[%s] newPage->erase failed...", __func__);
                    return err;
                }
                mPageList.erase(newPage);
				NVS_LOGD(TAG, "[%s] mFreePageList.push_back(newPage)", __func__);
                mFreePageList.push_back(newPage);
            }
            auto err = activatePage();
            if (err != NVS_OK) {
				NVS_LOGD(TAG, "[%s] activatePage failed...", __func__);
                return err;
            }
            newPage = &mPageList.back();

            err = it->copyItems(*newPage);
            if (err != NVS_OK && err != NVS_ERR_NVS_NOT_FOUND) {
				NVS_LOGD(TAG, "[%s] copyItems failed...", __func__);
				return err;
            }

            err = it->erase();
            if (err != NVS_OK) {
				NVS_LOGD(TAG, "[%s] erase failed...", __func__);
				return err;
            }

            Page* p = static_cast<Page*>(it);
            mPageList.erase(it);
			NVS_LOGD(TAG, "[%s] mFreePageList.push_back(p)", __func__);
            mFreePageList.push_back(p);
            break;
        }
    }

    // partition should have at least one free page
    if (mFreePageList.empty()) {
		NVS_LOGD(TAG, "[%s] returns NVS_ERR_NVS_NO_FREE_PAGES ...", __func__);
		return NVS_ERR_NVS_NO_FREE_PAGES;
    }

	NVS_LOGD(TAG, "[%s] returns NVS_OK...", __func__);
	return NVS_OK;
}

nvs_err_t PageManager::requestNewPage()
{
    if (mFreePageList.empty()) {
        return NVS_ERR_NVS_INVALID_STATE;
    }

    // do we have at least two free pages? in that case no erasing is required
    if (mFreePageList.size() >= 2) {
        return activatePage();
    }

    // find the page with the higest number of erased items
    TPageListIterator maxUnusedItemsPageIt;
    size_t maxUnusedItems = 0;
    for (auto it = begin(); it != end(); ++it) {

        auto unused =  Page::ENTRY_COUNT - it->getUsedEntryCount();
        if (unused > maxUnusedItems) {
            maxUnusedItemsPageIt = it;
            maxUnusedItems = unused;
        }
    }

    if (maxUnusedItems == 0) {
        return NVS_ERR_NVS_NOT_ENOUGH_SPACE;
    }

    nvs_err_t err = activatePage();
    if (err != NVS_OK) {
        return err;
    }

    Page* newPage = &mPageList.back();

    Page* erasedPage = maxUnusedItemsPageIt;

#ifndef NDEBUG
    size_t usedEntries = erasedPage->getUsedEntryCount();
#endif
    err = erasedPage->markFreeing();
    if (err != NVS_OK) {
        return err;
    }
    err = erasedPage->copyItems(*newPage);
    if (err != NVS_OK && err != NVS_ERR_NVS_NOT_FOUND) {
        return err;
    }

    err = erasedPage->erase();
    if (err != NVS_OK) {
        return err;
    }

#ifndef NDEBUG
    assert(usedEntries == newPage->getUsedEntryCount());
#endif

    mPageList.erase(maxUnusedItemsPageIt);
    mFreePageList.push_back(erasedPage);

    return NVS_OK;
}

nvs_err_t PageManager::activatePage()
{
    if (mFreePageList.empty()) {
        return NVS_ERR_NVS_NOT_ENOUGH_SPACE;
    }
    Page* p = &mFreePageList.front();
    if (p->state() == Page::PageState::CORRUPT) {
        auto err = p->erase();
        if (err != NVS_OK) {
            return err;
        }
    }
    mFreePageList.pop_front();
    mPageList.push_back(p);
	NVS_LOGD(TAG, "[%s] setSeqNumber : %d...", __func__, mSeqNumber);
    p->setSeqNumber(mSeqNumber);
    ++mSeqNumber;
    return NVS_OK;
}

nvs_err_t PageManager::fillStats(nvs_stats_t& nvsStats)
{
    nvsStats.used_entries      = 0;
    nvsStats.free_entries      = 0;
    nvsStats.total_entries     = 0;
    nvs_err_t err = NVS_OK;

    // list of used pages
    for (auto p = mPageList.begin(); p != mPageList.end(); ++p) {
        err = p->calcEntries(nvsStats);
        if (err != NVS_OK) {
            return err;
        }
    }

    // free pages
    nvsStats.total_entries += mFreePageList.size() * Page::ENTRY_COUNT;
    nvsStats.free_entries  += mFreePageList.size() * Page::ENTRY_COUNT;

    return err;
}

} // namespace nvs
