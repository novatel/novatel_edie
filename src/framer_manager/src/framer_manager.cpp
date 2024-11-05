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

#include "framer_manager.hpp"

using namespace novatel::edie;

FramerManager::FramerManager() : pclMyLogger(Logger::RegisterLogger("FramerManager")), pclMyCircularDataBuffer(std::make_shared<CircularBuffer>())
{
    pclMyCircularDataBuffer->Clear();
    pclMyLogger->debug("Framer Manager initialized");
}

std::shared_ptr<CircularBuffer> FramerManager::GetCircularBuffer() const { return pclMyCircularDataBuffer; }

// FramerManager::FramerManager(const std::string& strLoggerName_)
//     : pclMyLogger(Logger::RegisterLogger(strLoggerName_)), pclMyCircularDataBuffer(std::make_shared<CircularBuffer>())
//{
//     //// instantiate the framers
//     // nmeaFramer = std::make_unique<nmea::NmeaFramer>(pclMyCircularDataBuffer);
//
//     // uint32_t uiNovatelFrameBufferOffset = 0;
//     // uint32_t uiNmeaFrameBufferOffset = 0;
//     // auto novatelStatus = STATUS::UNKNOWN;
//     // auto nmeaStatus = STATUS::UNKNOWN;
//
//     // framerStack.emplace_back(FRAMER_ID::NMEA, uiNmeaFrameBufferOffset, nmeaStatus);
//     // framerStack.emplace_back(FRAMER_ID::NOVATEL, uiNovatelFrameBufferOffset, novatelStatus);
//
//     pclMyCircularDataBuffer->Clear();
//     pclMyLogger->debug("Internal Framer Manager initialized");
// }

void FramerManager::RegisterFramer(const FRAMER_ID framerId_, std::unique_ptr<FramerBase> framer_, std::unique_ptr<MetaDataBase> metadata_)
{
    framerRegistry.emplace_back(FramerElement(framerId_, std::move(framer_), std::move(metadata_)));
}

uint32_t FramerManager::Write(const unsigned char* pucDataBuffer_, uint32_t uiDataBytes_)
{
    return pclMyCircularDataBuffer->Append(pucDataBuffer_, uiDataBytes_);
}

[[nodiscard]] std::shared_ptr<spdlog::logger> FramerManager::GetLogger() const { return pclMyLogger; }

void FramerManager::SetLoggerLevel(const spdlog::level::level_enum eLevel_) const { pclMyLogger->set_level(eLevel_); }

void FramerManager::ShutdownLogger() { ShutdownLogger(); }

// void reorderFramers(std::deque<FramerElement>& framerStack_)
//{
//     auto it = std::min_element(framerStack_.begin(), framerStack_.end(),
//                                [](const FramerElement& a, const FramerElement& b) { return (a.offset < b.offset); });
//     if (it != framerStack_.end())
//     {
//         FramerElement earliestFramerElement = *it;
//         framerStack_.erase(it);
//         framerStack_.emplace_front(earliestFramerElement);
//     }
// }
//
// void FramerManager::ResetFramerStack()
//{
//    for (auto& element : framerStack)
//    {
//        element.offset = 0;
//        element.status = STATUS::UNKNOWN;
//    }
//}

// void DisplayFramerStack(std::deque<FramerElement>& framerStack_)
//{
//     // TODO : Delete this for loop - debugging purposes only
//     for (const auto& elem : framerStack_)
//     {
//         if (elem.framer == FRAMER_ID::NOVATEL)
//         {
//             std::cout << "Framer: "
//                       << "NOVATEL"
//                       << ", Offset: " << elem.offset << ", Status: " << elem.status << std::endl;
//         }
//         else if (elem.framer == FRAMER_ID::NMEA)
//         {
//             std::cout << "Framer: "
//                       << "NMEA"
//                       << ", Offset: " << elem.offset << ", Status: " << elem.status << std::endl;
//         }
//         else
//         {
//             std::cout << "Framer: "
//                       << "UNKNOWN"
//                       << ", Offset: " << elem.offset << ", Status: " << elem.status << std::endl;
//         }
//     }
// }

void FramerManager::ResetInactiveFramerStates(const FRAMER_ID& eActiveFramerId_)
{
    for (auto& [framerId, framer, metadata] : framerRegistry)
    {
        if (framerId != eActiveFramerId_) { framer->ResetStateAndByteCount(); }
    }
}

void FramerManager::ResetFramerStates()
{
    for (auto& [framerId, framer, metadata] : framerRegistry)
    {
        framer->ResetStateAndByteCount();
    }
}

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

    // TODO: are these statuses needed?
    STATUS novatelStatus = STATUS::UNKNOWN;
    STATUS nmeaStatus = STATUS::UNKNOWN;

    if (eActiveFramerId_ == FRAMER_ID::UNKNOWN)
    {
        for (auto& [framerId, framer, metadata] : framerRegistry)
        {
            framer->eMyCurrentFramerStatus = framer->FindNextSyncByte(pucFrameBuffer_, uiFrameBufferSize_);
            if (framer->eMyCurrentFramerStatus == STATUS::SUCCESS)
            {
                eActiveFramerId_ = framerId;
                return framer->eMyCurrentFramerStatus;
            }
        }
        // ResetFramerStack();
        // for (auto& element : framerStack)
        //{
        //     switch (element.framer)
        //     {
        //     case FRAMER_ID::NMEA:
        //         element.status = nmeaFramer->GetFrame(pucFrameBuffer_, uiFrameBufferSize_, nmeaMetaDataStruct, element.offset);
        //         // if (element.status == STATUS::SUCCESS) { eActiveFramerId_ = FRAMER_ID::NMEA; }
        //         break;
        //     case FRAMER_ID::NOVATEL:
        //         element.status = novatelFramer->GetFrame(pucFrameBuffer_, uiFrameBufferSize_, novatelMetaDataStruct, element.offset);
        //         // if (element.status == STATUS::SUCCESS) { eActiveFramerId_ = FRAMER_ID::NOVATEL; }
        //         break;
        //     case FRAMER_ID::UNKNOWN: break;
        //     }
        // }
    }
    // Once active framer is identified, exclusively use it to frame. Reset active Framer at success
    else
    {
        // auto& framer_element = std::find_if(framerRegistry.begin(), framerRegistry.end(),
        //                                     [eActiveFramerId_](FramerElement& element) { return (element.framerId == eActiveFramerId_); });
        // ResetInactiveFramerStates(eActiveFramerId_);

        // framer_element->framer->GetFrame(pucFrameBuffer_, uiFrameBufferSize_, framer_element->metadata);
        //
        //
        for (auto& framer_element : framerRegistry)
        {
            if (framer_element.framerId == eActiveFramerId_)
            {
                auto dingle =  framer_element.framer->GetFrame(pucFrameBuffer_, uiFrameBufferSize_, *framer_element.metadata);
                return dingle;
            }
        }
        //
        // if (framer_element->first == FRAMER_ID::NMEA)
        //{
        //     framer_element->second->eMyCurrentFramerStatus = nmeaFramer->GetFrame(pucFrameBuffer_, uiFrameBufferSize_, nmeaMetaDataStruct,

        //     framer_element->offset);
        // }
        // if (framer_element->first == FRAMER_ID::NOVATEL) { framer_element->second->GetFrame(pucFrameBuffer_, uiFrameBufferSize_, );
        // }
    }

    //// A Framer Successfully Framed a log
    // auto it_success =
    //     std::find_if(framerStack.begin(), framerStack.end(), [](FramerElement& element) { return (element.status == STATUS::SUCCESS); });
    // if (it_success != framerStack.end()) { return it_success->status; }

    // A Framer Found A Sync Byte
    auto it_sync = std::find_if(framerRegistry.begin(), framerRegistry.end(),
                                [](const FramerElement& element) { return (element.framer->eMyCurrentFramerStatus == STATUS::SYNC_BYTES_FOUND); });
    if (it_sync != framerRegistry.end())
    {
        // reorderFramers(framerStack);
        // DisplayFramerStack(framerStack);
        eActiveFramerId_ = framerRegistry.front().framerId;
        return framerRegistry.front().framer->eMyCurrentFramerStatus;
    }

    // Prove all framers returned UNKNOWN
    auto it_not_unknown = std::find_if(framerRegistry.begin(), framerRegistry.end(),
                                       [](const FramerElement& element) { return (element.framer->eMyCurrentFramerStatus != STATUS::UNKNOWN); });

    // it_not_unknown == end -> All framers returned UNKNOWN -> use closest framer to handle unknown bytes
    if (it_not_unknown == framerRegistry.end())
    {
        // reorderFramers(framerStack);
        // DisplayFramerStack(framerStack);
        eActiveFramerId_ = framerRegistry.front().framerId;
        HandleUnknownBytes(pucFrameBuffer_, framerRegistry.front().framer->uiMySyncByteOffset);

        // TODO reset all framers since bytes have been removed

        return framerRegistry.front().framer->eMyCurrentFramerStatus;
    }

    //// if any framer has buffer full, reset all framers
    //// TODO remove initialization of it_buffer_full = framerStack.end() when ready to clean
    // auto it_buffer_full = framerStack.end();
    // it_buffer_full =
    //     std::find_if(framerStack.begin(), framerStack.end(), [](FramerElement& element) { return (element.status == STATUS::BUFFER_FULL); });
    // if (it_buffer_full != framerStack.end())
    //{
    //     ResetInactiveFramerStates(FRAMER_ID::UNKNOWN);
    //     return STATUS::BUFFER_FULL;
    // }

    // auto it_buffer_empty =
    //     std::find_if(framerStack.begin(), framerStack.end(), [](FramerElement& element) { return (element.status == STATUS::BUFFER_EMPTY); });
    // if (it_buffer_empty != framerStack.end()) { return STATUS::BUFFER_EMPTY; }

    // DisplayFramerStack(framerStack);
    //// Possible other options (Some UNKNOWN, Some BUFFER_FULL
    return STATUS::INCOMPLETE;
}

void FramerManager::HandleUnknownBytes(unsigned char* pucBuffer_, const uint32_t& uiUnknownBytes_)
{
    if (bMyReportUnknownBytes && pucBuffer_ != nullptr) { pclMyCircularDataBuffer->Copy(pucBuffer_, uiUnknownBytes_); }
    pclMyCircularDataBuffer->Discard(uiUnknownBytes_);

    // novatelFramer->ResetStateAndByteCount();
    // nmeaFramer->ResetStateAndByteCount();
}
