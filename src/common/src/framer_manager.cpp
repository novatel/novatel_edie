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

#include "novatel_edie/common/framer_manager.hpp"

using namespace novatel::edie;

FramerManager::FramerManager()
    : pclMyLogger(Logger::RegisterLogger("FramerManager")), pclMyCircularDataBuffer(std::make_shared<CircularBuffer>()), iActiveFramerId(-1)
{
    pclMyCircularDataBuffer->Clear();
    idMap["UNKNOWN"] = 0;
    pclMyLogger->debug("Framer Manager initialized");
}

void FramerManager::RegisterFramer(std::string framerName, std::unique_ptr<FramerBase> framer_, std::unique_ptr<MetaDataBase> metadata_)
{

    int framerId_ = -1;
    auto it = idMap.find(framerName);
    if (it == idMap.end())
    {
        int newId = static_cast<int>(idMap.size());
        idMap[framerName] = newId;
        framerId_ = newId;
    }
    else { framerId_ = it->second; }

    framerRegistry.emplace_back(framerId_, std::move(framer_), std::move(metadata_), 0);
}

void FramerManager::SortFramers()
{
    auto it = std::min_element(framerRegistry.begin(), framerRegistry.end(), [](const FramerElement& a, const FramerElement& b) {
        return a.framer->uiMyFrameBufferOffset < b.framer->uiMyFrameBufferOffset;
    });

    if (it != framerRegistry.end())
    {
        FramerElement earliestFramerElement = std::move(*it);
        framerRegistry.erase(it);
        framerRegistry.push_front(std::move(earliestFramerElement));
    }
}

MetaDataBase* FramerManager::GetMetaData(const int framerId_)
{
    for (FramerElement& element : framerRegistry)
    {
        if (element.framerId == framerId_) { return element.metadata.get(); }
    }
    return nullptr;
}

void FramerManager::DisplayFramerStack()
{

    // TODO : Delete this for loop - debugging purposes only
    for (const auto& elem : framerRegistry)
    {
        std::string idName;
        for (const auto& pair : idMap)
        {
            if (pair.second == elem.framerId)
            {
                idName = pair.first;
                break;
            }
        }
        pclMyLogger->debug("Framer: {}, Offset: {}, Status: {}", idName, elem.framer->uiMyFrameBufferOffset, elem.framer->eMyCurrentFramerStatus);
    }
}

void FramerManager::ResetInactiveFramerStates(const int& iActiveFramerId_)
{
    for (auto& [framerId, framer, metadata, offset] : framerRegistry)
    {
        if (framerId != iActiveFramerId_)
        {
            framer->ResetStateAndByteCount();
            offset = 0;
        }
    }
}

void FramerManager::ResetAllFramerStates() { ResetInactiveFramerStates(idMap["UNKNOWN"]); }

void FramerManager::ResetInactiveMetaDataStates(const int& iActiveFramerId_)
{
    for (auto& [framerId, framer, metadata, offset] : framerRegistry)
    {
        if (framerId != iActiveFramerId_)
        {
            metadata->uiLength = 0;
            metadata->eFormat = HEADER_FORMAT::UNKNOWN;
        }
    }
}

void FramerManager::ResetAllMetaDataStates() { ResetInactiveMetaDataStates(idMap["UNKNOWN"]); }

FramerElement* FramerManager::GetFramerElement(const int framerId_)
{
    for (FramerElement& element : framerRegistry)
    {
        if (element.framerId == framerId_) { return &element; }
    }
    return nullptr;
}

std::unique_ptr<FramerBase>& FramerManager::GetFramerInstance(std::string framerAlias_) { return GetFramerElement(idMap[framerAlias_])->framer; }

STATUS FramerManager::GetFrame(unsigned char* pucFrameBuffer_, uint32_t uiFrameBufferSize_)
{
    // State Machine:
    // use framers to identify sync bytes and offsets
    // once sync bytes are identified, set Active Framer and perform framing using type specific framer, reset inactive framers
    // upon successful framing of a log, reset Active Framer ID

    // if STATUS is INCOMPLETE upon entering GetFrame, it is stale
    if (iActiveFramerId != idMap["UNKNOWN"] && framerRegistry.front().framer->eMyCurrentFramerStatus == STATUS::INCOMPLETE)
    {
        framerRegistry.front().framer->eMyCurrentFramerStatus = STATUS::INCOMPLETE_MORE_DATA;
    }

    if (iActiveFramerId == idMap["UNKNOWN"])
    {
        ResetAllMetaDataStates();
        ResetAllFramerStates();
        for (auto& [framerId, framer, metadata, offset] : framerRegistry) { framer->FindNextSyncByte(uiFrameBufferSize_); }
    }

    // A Framer Found A Sync Byte
    // DisplayFramerStack();
    SortFramers();

    // set uiLength for likely framer
    framerRegistry.front().metadata->uiLength = framerRegistry.front().framer->uiMyFrameBufferOffset;
    if (framerRegistry.front().framer->eMyCurrentFramerStatus == STATUS::INCOMPLETE) { return STATUS::INCOMPLETE; }
    iActiveFramerId = framerRegistry.front().framerId;

    if (framerRegistry.front().framer->uiMyFrameBufferOffset > 0 &&
        (framerRegistry.front().framer->eMyCurrentFramerStatus != STATUS::INCOMPLETE ||
         framerRegistry.front().framer->eMyCurrentFramerStatus != STATUS::INCOMPLETE_MORE_DATA))
    {
        HandleUnknownBytes(pucFrameBuffer_, framerRegistry.front().framer->uiMyFrameBufferOffset);
        ResetAllFramerStates();
        return STATUS::UNKNOWN;
    }

    // Once active framer is identified, exclusively use it to frame. Reset active Framer at success
    for (auto& framer_element : framerRegistry)
    {
        if (framer_element.framerId == iActiveFramerId)
        {
            ResetInactiveFramerStates(framer_element.framerId);
            framer_element.framer->eMyCurrentFramerStatus =
                framer_element.framer->GetFrame(pucFrameBuffer_, uiFrameBufferSize_, *framer_element.metadata);
            // DisplayFramerStack();
            if (framer_element.framer->eMyCurrentFramerStatus == STATUS::SUCCESS)
            {
                pclMyCircularDataBuffer->Copy(pucFrameBuffer_, framer_element.metadata->uiLength);
                pclMyCircularDataBuffer->Discard(framer_element.metadata->uiLength);
            }
            else if (framer_element.framer->eMyCurrentFramerStatus == STATUS::UNKNOWN)
            {
                HandleUnknownBytes(pucFrameBuffer_, framer_element.framer->uiMyFrameBufferOffset);
                ResetAllFramerStates();
                return framer_element.framer->eMyCurrentFramerStatus;
            }
            break;
        }
    }

    // A Framer Successfully Framed a log
    auto it_success = std::find_if(framerRegistry.begin(), framerRegistry.end(),
                                   [](FramerElement& element) { return (element.framer->eMyCurrentFramerStatus == STATUS::SUCCESS); });
    if (it_success != framerRegistry.end()) { return it_success->framer->eMyCurrentFramerStatus; }

    // if any framer has full buffer, reset all framers
    auto it_buffer_full = std::find_if(framerRegistry.begin(), framerRegistry.end(),
                                       [](FramerElement& element) { return (element.framer->eMyCurrentFramerStatus == STATUS::BUFFER_FULL); });
    if (it_buffer_full != framerRegistry.end())
    {
        ResetAllFramerStates();
        return STATUS::BUFFER_FULL;
    }

    // if any framer has empty buffer, reset all framers
    auto it_buffer_empty = std::find_if(framerRegistry.begin(), framerRegistry.end(),
                                        [](FramerElement& element) { return (element.framer->eMyCurrentFramerStatus == STATUS::BUFFER_EMPTY); });
    if (it_buffer_empty != framerRegistry.end())
    {
        ResetAllFramerStates();
        return STATUS::BUFFER_EMPTY;
    }

    auto it_null_provided = std::find_if(framerRegistry.begin(), framerRegistry.end(),
                                         [](FramerElement& element) { return (element.framer->eMyCurrentFramerStatus == STATUS::NULL_PROVIDED); });
    if (it_null_provided != framerRegistry.end())
    {
        ResetAllFramerStates();
        return STATUS::NULL_PROVIDED;
    }

    return STATUS::INCOMPLETE;
}

void FramerManager::HandleUnknownBytes(unsigned char* pucBuffer_, const uint32_t& uiUnknownBytes_) const
{
    if (bMyReportUnknownBytes && pucBuffer_ != nullptr) { pclMyCircularDataBuffer->Copy(pucBuffer_, uiUnknownBytes_); }
    pclMyCircularDataBuffer->Discard(uiUnknownBytes_);
}
