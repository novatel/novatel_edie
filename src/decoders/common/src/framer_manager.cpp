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

// Define static member
// std::unordered_map<std::string, FramerManager::FramerFactory> FramerManager::framerFactories;

namespace {
static int forceInit = []() {
    std::cerr << "[DEBUG] Ensuring FramerManager initialized before registration.\n";
    FramerManager::GetFramerFactories(); // Force static initialization
    return 0;
}();
}

FramerManager::FramerManager(const std::vector<std::string>& selectedFramers)
    : pclMyLogger(Logger::RegisterLogger("FramerManager")), pclMyCircularDataBuffer(std::make_shared<CircularBuffer>())
{
    pclMyCircularDataBuffer->Clear();
    pclMyLogger->debug("Framer Manager initialized");

    for (const auto& name : selectedFramers)
    {
        auto it = GetFramerFactories().find(name);
        if (it != GetFramerFactories().end())
        {
            auto& constructors = it->second;
            auto metadataInstance = constructors.second();
            auto framerInstance = constructors.first(pclMyCircularDataBuffer);
            framerRegistry.emplace_back(name, std::move(framerInstance), std::move(metadataInstance));
        }
        else { pclMyLogger->error("Warning: Framer {} not registered.\n", name); }
    }
}

void FramerManager::RegisterFramer(const std::string& name, std::function<std::unique_ptr<FramerBase>(std::shared_ptr<CircularBuffer>)> framerFactory,
                                   std::function<std::unique_ptr<MetaDataBase>()> metadataConstructor)
{
    auto& factoryMap = GetFramerFactories();
    try
    {
        std::cerr << "[DEBUG] Attempting to insert into framerFactories: " << name << "\n";
        factoryMap[name] = {std::move(framerFactory), std::move(metadataConstructor)};
        std::cerr << "[DEBUG] Successfully inserted framer: " << name << "\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "[ERROR] Exception inserting into framerFactories: " << e.what() << "\n";
    }
}

// FramerManager.cpp
std::unordered_map<std::string, std::pair<std::function<std::unique_ptr<FramerBase>(std::shared_ptr<CircularBuffer>)>,
                                          std::function<std::unique_ptr<MetaDataBase>()>>>&
FramerManager::GetFramerFactories()
{
    static std::unordered_map<std::string, std::pair<std::function<std::unique_ptr<FramerBase>(std::shared_ptr<CircularBuffer>)>,
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

    while (true)
    {
        if (pclMyCircularDataBuffer->GetLength() == 0)
        {
            return STATUS::BUFFER_EMPTY;
        }

        // Step 1: Scan for first sync (offset == 0) or lowest valid offset
        for (auto it = framerRegistry.begin(); it != framerRegistry.end(); ++it)
        {
            int offset = it->framerInstance->FindSyncOffset(uiFrameBufferSize_, eStatus);
            //if (eStatus != STATUS::SUCCESS) { continue; }

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

        // Sync found, but not at offset 0. Discard those bytes first
        if (bestOffset != 0)
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
            HandleUnknownBytes(pucFrameBuffer_, pclMyCircularDataBuffer->GetLength());
            stMyMetaData.uiLength = pclMyCircularDataBuffer->GetLength();
            stMyMetaData.eFormat = HEADER_FORMAT::UNKNOWN;
            stMetaData_ = &stMyMetaData;  // There is no valid MetaData object to use from a Framer so use the MetaDataBase from FramerManager
            return STATUS::UNKNOWN;
        }

        if (bestIt != framerRegistry.end() && 
            eStatus == STATUS::INCOMPLETE || eStatus == STATUS::BUFFER_EMPTY) 
        {
            return STATUS::BUFFER_EMPTY;
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

            pclMyCircularDataBuffer->Copy(pucFrameBuffer_, stMetaData_->uiLength);
            pclMyCircularDataBuffer->Discard(stMetaData_->uiLength);
            return STATUS::SUCCESS;
        }
        else if (eStatus == STATUS::UNKNOWN)
        {
            // Framer is currently handling the unknown bytes. Unsure if this is correct or if FM should be handling the unknown bytes.
            // For now I'm going to keep in the individual framer, but it may be better to move this to the FramerManager for control
            // over the unknown bytes if needed. e.g. it might be that we need to try a different framer if the first one fails.

            // If the framer failed, discard the bytes and retry
            //HandleUnknownBytes(pucFrameBuffer_, stMetaData_->uiLength);
            //stMyMetaData.uiLength = stMetaData_->uiLength;
            //stMyMetaData.eFormat = HEADER_FORMAT::UNKNOWN;
            // stMetaData_ has been set from the current framer so return that back
            return eStatus;
        }
        else if (eStatus == STATUS::INCOMPLETE || eStatus == STATUS::BUFFER_EMPTY) 
        {
            return STATUS::BUFFER_EMPTY;
        }
        else // Unsure if this is required.
        {
            // Framer failed — discard 1 byte and retry
            HandleUnknownBytes(pucFrameBuffer_, 1);
            pucFrameBuffer_ += 1;
            uiFrameBufferSize_ -= 1;
        }
    }

    return STATUS::UNKNOWN;
}

void FramerManager::HandleUnknownBytes(unsigned char* pucBuffer_, const uint32_t& uiUnknownBytes_) const
{
    if (bMyReportUnknownBytes && pucBuffer_ != nullptr) { pclMyCircularDataBuffer->Copy(pucBuffer_, uiUnknownBytes_); }
    pclMyCircularDataBuffer->Discard(uiUnknownBytes_);
}
