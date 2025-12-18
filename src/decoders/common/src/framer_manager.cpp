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

FramerManager::FramerManager(const std::vector<std::string>& selectedFramers)
    : pclMyLogger(GetBaseLoggerManager()->RegisterLogger("FramerManager")), pclMyFixedRingBuffer(std::make_shared<UCharFixedRingBuffer>())
{
    auto& factoryMap = GetFramerFactories();

    for (const auto& name : selectedFramers)
    {
        auto framerId = CalculateBlockCrc32(name);
        auto it = factoryMap.find(framerId);
        if (it != factoryMap.end())
        {
            auto& constructors = it->second;
            auto metadataInstance = constructors.second();
            auto framerInstance = constructors.first(pclMyFixedRingBuffer);
            framerRegistry.emplace_back(name, framerId, std::move(framerInstance), std::move(metadataInstance));
            pclMyLogger->info("Registered framer '{}'", name);
        }
        else { pclMyLogger->warn("Framer '{}' not found in registered framer factories.", name); }
    }
}

void FramerManager::RegisterFramer(const std::string& framerName_,
                                   std::function<std::unique_ptr<FramerBase>(std::shared_ptr<UCharFixedRingBuffer>)> framerFactory_,
                                   std::function<std::unique_ptr<MetaDataBase>()> metadataConstructor_)
{
    auto& factoryMap = GetFramerFactories();
    auto framerId = CalculateBlockCrc32(framerName_);

    // Check for collision in framer name hash value
    if (factoryMap.find(framerId) != factoryMap.end())
    {
        throw std::runtime_error("Framer with ID " + std::to_string(framerId) +
                                 " is already registered."
                                 " Please choose a different name for '" +
                                 framerName_ + "'.");
    }
    factoryMap[framerId] = {std::move(framerFactory_), std::move(metadataConstructor_)};
}

std::unordered_map<std::uint32_t, std::pair<std::function<std::unique_ptr<FramerBase>(std::shared_ptr<UCharFixedRingBuffer>)>,
                                            std::function<std::unique_ptr<MetaDataBase>()>>>&
FramerManager::GetFramerFactories()
{
    static std::unordered_map<uint32_t, std::pair<std::function<std::unique_ptr<FramerBase>(std::shared_ptr<UCharFixedRingBuffer>)>,
                                                  std::function<std::unique_ptr<MetaDataBase>()>>>
        factories;
    return factories;
}

MetaDataBase* FramerManager::GetMetaData(const std::string framerName_)
{
    for (auto& framer : framerRegistry)
    {
        if (framer.framerName == framerName_) { return framer.metadataInstance.get(); }
    }
    return nullptr;
}

void FramerManager::ResetAllFramerStates() const
{
    for (const auto& framer : framerRegistry)
    {
        framer.framerInstance->InitAttributes();
        framer.framerInstance->ResetState();
    }
}

FramerEntry* FramerManager::GetFramerElement(const std::string framerName_)
{
    for (auto& framer : framerRegistry)
    {
        if (framer.framerName == framerName_) { return &framer; }
    }
    return nullptr;
}

STATUS FramerManager::GetFrame(unsigned char* pucFrameBuffer_, uint32_t uiFrameBufferSize_, MetaDataBase*& stMetaData_)
{
    while (true)
    {
        if (pclMyFixedRingBuffer->empty()) { return STATUS::BUFFER_EMPTY; }

        STATUS eStatus = STATUS::UNKNOWN;
        STATUS bestStatus = STATUS::UNKNOWN;
        MetaDataBase* currentMetaData;
        auto bestOffset = static_cast<uint32_t>(pclMyFixedRingBuffer->size());
        auto framerIt = framerRegistry.begin();
        auto bestFramerIt = framerRegistry.end();

        // Scan for first frame offset among all registered framers
        for (; framerIt != framerRegistry.end(); framerIt++)
        {
            currentMetaData = framerIt->metadataInstance.get();
            eStatus = framerIt->framerInstance->GetFrame(pucFrameBuffer_, uiFrameBufferSize_, *currentMetaData, /*bMetadataOnly=*/true);

            // If any framer returns a known status, keep it as the best candidate. If multiple framers
            // return known statuses but no framer returns SUCCESS, one of the known statuses will be returned.
            if (eStatus != STATUS::UNKNOWN)
            {
                bestFramerIt = framerIt;
                bestStatus = eStatus;
                stMetaData_ = currentMetaData;
                // If a framer sees a valid, complete frame, then use it immediately
                if (eStatus == STATUS::SUCCESS) { break; }
            }
            // Track the smallest offset among framers with UNKNOWN statuses to possibly discard unknown bytes later
            else if (currentMetaData->uiLength < bestOffset) { bestOffset = currentMetaData->uiLength; }
        }

        assert((bestStatus == STATUS::UNKNOWN && bestFramerIt == framerRegistry.end()) ||
               (bestStatus != STATUS::UNKNOWN && bestFramerIt != framerRegistry.end()));

        // No frame found at all
        if (bestFramerIt == framerRegistry.end())
        {
            assert(bestOffset > 0 && bestOffset <= static_cast<uint32_t>(pclMyFixedRingBuffer->size()));
            HandleUnknownBytes(pucFrameBuffer_, bestOffset);
            stMyMetaData.uiLength = bestOffset;
            stMyMetaData.eFormat = HEADER_FORMAT::UNKNOWN;
            stMetaData_ = &stMyMetaData; // There is no valid MetaData object to use from a Framer so use the MetaDataBase from FramerManager
            return STATUS::UNKNOWN;
        }

        if (bestStatus == STATUS::SUCCESS)
        {
            // Move this framer to the front
            if (framerIt != framerRegistry.begin()) { std::rotate(framerRegistry.begin(), framerIt, framerIt + 1); }

            if (stMetaData_->uiLength > uiFrameBufferSize_) { return STATUS::BUFFER_FULL; }

            pclMyFixedRingBuffer->copy_out(pucFrameBuffer_, stMetaData_->uiLength);
            pclMyFixedRingBuffer->erase_begin(stMetaData_->uiLength);
            ResetAllFramerStates();
            return STATUS::SUCCESS;
        }

        if (bestStatus != STATUS::UNKNOWN) { return bestStatus; }

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
    ResetAllFramerStates();
}
