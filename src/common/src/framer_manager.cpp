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

FramerManager::FramerManager() : pclMyLogger(Logger::RegisterLogger("FramerManager")), pclMyCircularDataBuffer(std::make_shared<CircularBuffer>())
{
    pclMyCircularDataBuffer->Clear();
    pclMyLogger->debug("Framer Manager initialized");
}

void FramerManager::RegisterFramer(const FRAMER_ID framerId_, std::unique_ptr<FramerBase> framer_, std::unique_ptr<MetaDataBase> metadata_)
{
    framerRegistry.emplace_back(framerId_, std::move(framer_), std::move(metadata_), 0);
}

void FramerManager::ReorderFramers()
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

MetaDataBase* FramerManager::GetMetaData(const FRAMER_ID framerId_)
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
        if (elem.framerId == FRAMER_ID::NOVATEL)
        {
            std::cout << "Framer: "
                      << "NOVATEL"
                      << ", Offset: " << elem.framer->uiMyFrameBufferOffset << ", Status: " << elem.framer->eMyCurrentFramerStatus << std::endl;
        }
        else if (elem.framerId == FRAMER_ID::NMEA)
        {
            std::cout << "Framer: "
                      << "NMEA"
                      << ", Offset: " << elem.framer->uiMyFrameBufferOffset << ", Status: " << elem.framer->eMyCurrentFramerStatus << std::endl;
        }
        else
        {
            std::cout << "Framer: "
                      << "UNKNOWN"
                      << ", Offset: " << elem.framer->uiMyFrameBufferOffset << ", Status: " << elem.framer->eMyCurrentFramerStatus << std::endl;
        }
    }
}

void FramerManager::ResetInactiveFramerStates(const FRAMER_ID& eActiveFramerId_)
{
    for (auto& [framerId, framer, metadata, offset] : framerRegistry)
    {
        if (framerId != eActiveFramerId_)
        {
            framer->ResetStateAndByteCount();
            offset = 0;
        }
    }
}

void FramerManager::ResetAllFramerStates() { ResetInactiveFramerStates(FRAMER_ID::UNKNOWN); }

void FramerManager::ResetInactiveMetaDataStates(const FRAMER_ID& eActiveFramerId_)
{
    for (auto& [framerId, framer, metadata, offset] : framerRegistry)
    {
        if (framerId != eActiveFramerId_)
        {
            metadata->uiLength = 0;
            metadata->eFormat = HEADER_FORMAT::UNKNOWN;
        }
    }
}

void FramerManager::ResetAllMetaDataStates() { ResetInactiveMetaDataStates(FRAMER_ID::UNKNOWN); }

FramerElement* FramerManager::GetFramerElement(const FRAMER_ID framerId_)
{
    for (FramerElement& element : framerRegistry)
    {
        if (element.framerId == framerId_) { return &element; }
    }
    return nullptr;
}

STATUS FramerManager::GetFrame(unsigned char* pucFrameBuffer_, uint32_t uiFrameBufferSize_, FRAMER_ID& eActiveFramerId_)
{
    // State Machine:
    // use framers to identify sync bytes and offsets
    // once sync bytes are identified, set Active Framer and perform framing using type specific framer, reset inactive framers
    // upon successful framing of a log, reset Active Framer ID

    // if STATUS is INCOMPLETE upon entering GetFrame, it is stale
    if (eActiveFramerId_ != FRAMER_ID::UNKNOWN && framerRegistry.front().framer->eMyCurrentFramerStatus == STATUS::INCOMPLETE)
    {
        framerRegistry.front().framer->eMyCurrentFramerStatus = STATUS::INCOMPLETE_MORE_DATA;
    }

    if (eActiveFramerId_ == FRAMER_ID::UNKNOWN)
    {
        ResetAllMetaDataStates();
        for (auto& [framerId, framer, metadata, offset] : framerRegistry) { framer->FindNextSyncByte(uiFrameBufferSize_); }
    }

    // A Framer Found A Sync Byte
    DisplayFramerStack();
    // TODO rename this to sort possibly (sort, prioritize, etc)
    ReorderFramers();

    // set uiLength for likely framer
    framerRegistry.front().metadata->uiLength = framerRegistry.front().framer->uiMyFrameBufferOffset;
    if (framerRegistry.front().framer->eMyCurrentFramerStatus == STATUS::INCOMPLETE) { return STATUS::INCOMPLETE; }

    eActiveFramerId_ = framerRegistry.front().framerId;

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
        if (framer_element.framerId == eActiveFramerId_)
        {
            ResetInactiveFramerStates(framer_element.framerId);
            framer_element.framer->eMyCurrentFramerStatus =
                framer_element.framer->GetFrame(pucFrameBuffer_, uiFrameBufferSize_, *framer_element.metadata);
            DisplayFramerStack();
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
    auto& it_success = std::find_if(framerRegistry.begin(), framerRegistry.end(),
                                    [](FramerElement& element) { return (element.framer->eMyCurrentFramerStatus == STATUS::SUCCESS); });
    if (it_success != framerRegistry.end()) { return it_success->framer->eMyCurrentFramerStatus; }

    // if any framer has buffer full, reset all framers
    auto it_buffer_full = std::find_if(framerRegistry.begin(), framerRegistry.end(),
                                       [](FramerElement& element) { return (element.framer->eMyCurrentFramerStatus == STATUS::BUFFER_FULL); });
    if (it_buffer_full != framerRegistry.end())
    {
        ResetAllFramerStates();
        return STATUS::BUFFER_FULL;
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

void FramerManager::HandleUnknownBytes(unsigned char* pucBuffer_, const uint32_t& uiUnknownBytes_)
{
    if (bMyReportUnknownBytes && pucBuffer_ != nullptr) { pclMyCircularDataBuffer->Copy(pucBuffer_, uiUnknownBytes_); }
    pclMyCircularDataBuffer->Discard(uiUnknownBytes_);
}
