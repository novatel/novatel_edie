// ===============================================================================
// |                                                                             |
// |  COPYRIGHT NovAtel Inc, 2022. All rights reserved.                          |
// |                                                                             |
// |  Permission is hereby granted, free of charge, to any person obtaining a    |
// |  copy of this software and associated documentation files (the "Software"), |
// |  to deal in the Software without restriction, including without limitation  |
// |  the rights to use, copy, modify, merge, publish, distribute, sublicense,   |
// |  and/or sell copies of the Software, and to permit persons to whom the      |
// |  Software is furnished to do so, subject to the following conditions:       |
// |                                                                             |
// |  The above copyright notice and this permission notice shall be included    |
// |  in all copies or substantial portions of the Software.                     |
// |                                                                             |
// |  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR |
// |  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   |
// |  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    |
// |  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER |
// |  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    |
// |  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        |
// |  DEALINGS IN THE SOFTWARE.                                                  |
// |                                                                             |
// ===============================================================================
// ! \file framer_manager.cpp
// ===============================================================================

#include "novatel_edie/decoders/common/framer_manager.hpp"

#include <algorithm>

using namespace novatel::edie;

namespace {
static int forceInit = []() {
    std::cerr << "[DEBUG] Ensuring FramerManager initialized before registration.\n";
    FramerManager::GetFramerFactories(); // Force static initialization
    return 0;
}();
} // namespace

FramerManager::FramerManager(const std::vector<std::string>& selectedFramers)
    : pclMyLogger(GetBaseLoggerManager()->RegisterLogger("FramerManager")), pclMyFixedRingBuffer(std::make_shared<UCharFixedRingBuffer>())
{
    std::cerr << "[C++] FramerManager constructor entered\n";

    auto& factoryMap = GetFramerFactories();

    // Check if framerFactories is valid before insertion
    std::cerr << "[DEBUG] framerFactories address: " << &factoryMap << "\n";

    for (const auto& name : selectedFramers)
    {
        std::cerr << "[C++] selectedFramer name: " << name << "\n ";
        auto it = GetFramerFactories().find(name);
        if (it != GetFramerFactories().end())
        {
            std::cerr << "[C++] pclMyFixedRingBuffer&: " << &pclMyFixedRingBuffer << "\n ";
            auto& constructors = it->second;

            auto metadataInstance = constructors.second();
            std::cerr << "[C++] metaDataInstance&: " << &metadataInstance << "\n ";

            auto framerInstance = constructors.first(pclMyFixedRingBuffer);
            std::cerr << "[C++] framerInstance&: " << &framerInstance << "\n ";

            framerRegistry.emplace_back(name, std::move(framerInstance), std::move(metadataInstance));
        }
        else { std::cerr << "Warning: Framer '" << name << "' not registered.\n"; }
    }
}

void FramerManager::RegisterFramer(const std::string& framerName_,
                                   std::function<std::unique_ptr<FramerBase>(std::shared_ptr<UCharFixedRingBuffer>)> framerFactory_,
                                   std::function<std::unique_ptr<MetaDataBase>()> metadataConstructor_)
{
    auto& factoryMap = GetFramerFactories();

    // Check if framerFactories is valid before insertion
    std::cerr << "[DEBUG] framerFactories address: " << &factoryMap << "\n";

    try
    {
        std::cerr << "[DEBUG] Attempting to insert into framerFactories: " << framerName_ << "\n";
        factoryMap[framerName_] = {std::move(framerFactory_), std::move(metadataConstructor_)};
        std::cerr << "[DEBUG] Successfully inserted framer: " << framerName_ << "\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "[ERROR] Exception inserting into framerFactories: " << e.what() << "\n";
    }
}

std::unordered_map<std::string, std::pair<std::function<std::unique_ptr<FramerBase>(std::shared_ptr<UCharFixedRingBuffer>)>,
                                          std::function<std::unique_ptr<MetaDataBase>()>>>&
FramerManager::GetFramerFactories()
{
    static std::unordered_map<std::string, std::pair<std::function<std::unique_ptr<FramerBase>(std::shared_ptr<UCharFixedRingBuffer>)>,
                                                     std::function<std::unique_ptr<MetaDataBase>()>>>
        factories;
    return factories;
}

MetaDataBase* FramerManager::GetMetaData(const std::string framerName_)
{
    for (FramerEntry& element : framerRegistry)
    {
        if (element.framerName == framerName_) { return element.metadataInstance.get(); }
    }
    return nullptr;
}

void FramerManager::ResetAllFramerStates()
{
    for (auto it = framerRegistry.begin(); it != framerRegistry.end(); ++it)
    {
        it->framerInstance->InitAttributes();
        it->framerInstance->ResetState();
    }
}

FramerEntry* FramerManager::GetFramerElement(const std::string framerName_)
{
    for (FramerEntry& element : framerRegistry)
    {
        if (element.framerName == framerName_) { return &element; }
    }
    return nullptr;
}

STATUS FramerManager::GetFrame(unsigned char* pucFrameBuffer_, uint32_t uiFrameBufferSize_, MetaDataBase*& stMetaData_)
{
    STATUS eStatus = STATUS::UNKNOWN;
    auto bestIt = framerRegistry.end();
    int bestOffset = std::numeric_limits<int>::max();
    ResetAllFramerStates();

    while (true)
    {
        if (pclMyFixedRingBuffer->size() == 0) { return STATUS::BUFFER_EMPTY; }

        // Step 1: Scan for first sync (offset == 0) or lowest valid offset
        for (auto it = framerRegistry.begin(); it != framerRegistry.end(); ++it)
        {
            int offset = it->framerInstance->FindSyncOffset(uiFrameBufferSize_, eStatus);
            if (eStatus != STATUS::SUCCESS && eStatus != STATUS::INCOMPLETE) { continue; }

            if (offset == 0)
            {
                bestIt = it;
                bestOffset = offset;
                break;
            }

            if (offset < bestOffset)
            {
                bestOffset = offset;
                bestIt = it;
            }
        }
        if ((bestIt != framerRegistry.end() && eStatus == STATUS::INCOMPLETE) || eStatus == STATUS::BUFFER_EMPTY) { return STATUS::BUFFER_EMPTY; }

        // Sync found, but not at offset 0. Discard those bytes first
        if (bestIt != framerRegistry.end() && bestOffset != 0)
        {
            HandleUnknownBytes(pucFrameBuffer_, bestOffset);
            stMyMetaData.uiLength = bestOffset;
            stMyMetaData.eFormat = HEADER_FORMAT::UNKNOWN;
            stMetaData_ = &stMyMetaData; // There is no valid MetaData object to use from a Framer so use the MetaDataBase from FramerManager
            return STATUS::UNKNOWN;
        }

        // No valid sync found at all. Discard the entire buffer.
        if (bestIt == framerRegistry.end())
        {
            HandleUnknownBytes(pucFrameBuffer_, pclMyFixedRingBuffer->size());
            stMyMetaData.uiLength = static_cast<uint32_t>(pclMyFixedRingBuffer->size());
            stMyMetaData.eFormat = HEADER_FORMAT::UNKNOWN;
            stMetaData_ = &stMyMetaData; // There is no valid MetaData object to use from a Framer so use the MetaDataBase from FramerManager
            return STATUS::UNKNOWN;
        }
        // Step 2: Try to frame using the chosen framer
        FramerBase* activeFramer = bestIt->framerInstance.get();
        stMetaData_ = bestIt->metadataInstance.get();

        eStatus = activeFramer->GetFrame(pucFrameBuffer_, uiFrameBufferSize_, *stMetaData_, /*bMetadataOnly=*/true);

        if (eStatus == STATUS::SUCCESS)
        {
            // Move this framer to the front
            if (bestIt != framerRegistry.begin()) { std::rotate(framerRegistry.begin(), bestIt, bestIt + 1); }

            if (stMetaData_->uiLength > uiFrameBufferSize_) { return STATUS::BUFFER_FULL; }

            pclMyFixedRingBuffer->copy_out(pucFrameBuffer_, stMetaData_->uiLength);
            pclMyFixedRingBuffer->erase_begin(stMetaData_->uiLength);
            return STATUS::SUCCESS;
        }
        else if (eStatus == STATUS::UNKNOWN)
        {
            // Framer is currently handling the unknown bytes. Unsure if this is correct or if FM should be handling the unknown bytes.
            // For now I'm going to keep in the individual framer, but it may be better to move this to the FramerManager for control
            // over the unknown bytes if needed. e.g. it might be that we need to try a different framer if the first one fails.

            // If the framer failed, discard the bytes and retry
            HandleUnknownBytes(pucFrameBuffer_, bestIt->framerInstance->GetMyByteCount());

            /* PROBLEM: The "Unknown" bytes are discarded here, but it could be the case that a different framer
                has a valid frame that begins within those "Unknown" bytes. Potential solution could be to only
                discard bytes until the start of the NEXT valid sync bytes (among all framers) */

            // stMyMetaData.uiLength = stMetaData_->uiLength;
            // stMyMetaData.eFormat = HEADER_FORMAT::UNKNOWN;
            //  stMetaData_ has been set from the current framer so return that back
            return eStatus;
        }
        else if (eStatus == STATUS::INCOMPLETE || eStatus == STATUS::BUFFER_EMPTY) { return STATUS::BUFFER_EMPTY; }
        else if (eStatus == STATUS::BUFFER_FULL) { return STATUS::BUFFER_FULL; }
        // else
        // Framer failed — discard 1 byte and retry
        HandleUnknownBytes(pucFrameBuffer_, 1);
        pucFrameBuffer_ += 1;
        uiFrameBufferSize_ -= 1;
    }

    return STATUS::UNKNOWN;
}

void FramerManager::HandleUnknownBytes(unsigned char* pucBuffer_, size_t uiUnknownBytes_) const
{
    if (bMyReportUnknownBytes && pucBuffer_ != nullptr) { pclMyFixedRingBuffer->copy_out(pucBuffer_, uiUnknownBytes_); }
    pclMyFixedRingBuffer->erase_begin(uiUnknownBytes_);
}
