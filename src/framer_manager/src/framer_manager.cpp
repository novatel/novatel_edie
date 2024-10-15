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

FramerManager::FramerManager(const std::string& strLoggerName_)
    : pclMyLogger(Logger::RegisterLogger(strLoggerName_)), pclMyCircularDataBuffer(std::make_shared<CircularBuffer>())
{
    //// instantiate the framers
    // nmeaFramer = std::make_unique<nmea::NmeaFramer>(pclMyCircularDataBuffer);

    // uint32_t uiNovatelFrameBufferOffset = 0;
    // uint32_t uiNmeaFrameBufferOffset = 0;
    // auto novatelStatus = STATUS::UNKNOWN;
    // auto nmeaStatus = STATUS::UNKNOWN;

    // framerStack.emplace_back(FRAMER_ID::NMEA, uiNmeaFrameBufferOffset, nmeaStatus);
    // framerStack.emplace_back(FRAMER_ID::NOVATEL, uiNovatelFrameBufferOffset, novatelStatus);

    pclMyCircularDataBuffer->Clear();
    pclMyLogger->debug("Internal Framer Manager initialized");
}

void FramerManager::RegisterFramer(const FRAMER_ID framerId_, std::unique_ptr<FramerBase> framer_)
{ framerRegistry[framerId_] = std::move(framer_); }

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
//     for (auto& element : framerStack)
//     {
//         element.offset = 0;
//         element.status = STATUS::UNKNOWN;
//     }
// }

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

// void FramerManager::ResetInactiveFramerStates(const FRAMER_ID& eActiveFramerId_)
//{
//     if (eActiveFramerId_ != FRAMER_ID::NMEA) { nmeaFramer->ResetStateAndByteCount(); }
//     if (eActiveFramerId_ != FRAMER_ID::NOVATEL) { novatelFramer->ResetStateAndByteCount(); }
// }

STATUS
FramerManager::GetFrame(unsigned char* pucFrameBuffer_, uint32_t uiFrameBufferSize_, FRAMER_ID& eActiveFramerId_)
{
    
    
    //// State Machine:
    //// use framers to identify sync bytes and offsets
    //// once sync bytes are identified, set Active Framer and perform framing using type specific framer, reset inactive framers
    //// upon successful framing of a log, reset Active Framer ID

    //// TODO: are these statuses needed?
    // STATUS novatelStatus = STATUS::UNKNOWN;
    // STATUS nmeaStatus = STATUS::UNKNOWN;
    // if (eActiveFramerId_ == FRAMER_ID::UNKNOWN)
    //{
    //     ResetFramerStack();
    //     for (auto& element : framerStack)
    //     {
    //         switch (element.framer)
    //         {
    //         case FRAMER_ID::NMEA:
    //             element.status = nmeaFramer->GetFrame(pucFrameBuffer_, uiFrameBufferSize_, nmeaMetaDataStruct, element.offset);
    //             // if (element.status == STATUS::SUCCESS) { eActiveFramerId_ = FRAMER_ID::NMEA; }
    //             break;
    //         case FRAMER_ID::NOVATEL:
    //             element.status =
    //                 novatelFramer->GetFrame(pucFrameBuffer_, uiFrameBufferSize_, novatelMetaDataStruct, element.offset);
    //             // if (element.status == STATUS::SUCCESS) { eActiveFramerId_ = FRAMER_ID::NOVATEL; }
    //             break;
    //         case FRAMER_ID::UNKNOWN: break;
    //         }
    //     }
    // }
    //// Once active framer is identified, exclusively use it to frame. Reset active Framer at success
    // else
    //{
    //     auto framer_element = std::find_if(framerStack.begin(), framerStack.end(),
    //                                        [eActiveFramerId_](FramerElement& element) { return (element.framer == eActiveFramerId_); });
    //     ResetInactiveFramerStates(eActiveFramerId_);
    //     if (framer_element->framer == FRAMER_ID::NMEA)
    //     {
    //         framer_element->status = nmeaFramer->GetFrame(pucFrameBuffer_, uiFrameBufferSize_, nmeaMetaDataStruct, framer_element->offset);
    //     }
    //     else if (framer_element->framer == FRAMER_ID::NOVATEL)
    //     {
    //         framer_element->status = novatelFramer->GetFrame(pucFrameBuffer_, uiFrameBufferSize_, novatelMetaDataStruct,
    //                                                                        framer_element->offset);
    //     }
    // }

    //// A Framer Successfully Framed a log
    // auto it_success =
    //     std::find_if(framerStack.begin(), framerStack.end(), [](FramerElement& element) { return (element.status == STATUS::SUCCESS); });
    // if (it_success != framerStack.end()) { return it_success->status; }

    //// A Framer Found A Sync Byte
    // auto it_sync =
    //     std::find_if(framerStack.begin(), framerStack.end(), [](FramerElement& element) { return (element.status == STATUS::SYNC_BYTES_FOUND); });
    // if (it_sync != framerStack.end())
    //{
    //     reorderFramers(framerStack);
    //     DisplayFramerStack(framerStack);
    //     eActiveFramerId_ = framerStack.front().framer;
    //     return framerStack.front().status;
    // }

    //// Prove all framers returned UNKNOWN
    // auto it_not_unknown =
    //     std::find_if(framerStack.begin(), framerStack.end(), [](FramerElement& element) { return (element.status != STATUS::UNKNOWN); });

    //// All framers returned UNKNOWN -> use closest framer to handle unknown bytes
    // if (it_not_unknown == framerStack.end())
    //{
    //     reorderFramers(framerStack);
    //     DisplayFramerStack(framerStack);
    //     eActiveFramerId_ = framerStack.front().framer;
    //     HandleUnknownBytes(pucFrameBuffer_, framerStack.front().offset, eActiveFramerId_);
    //     return framerStack.front().status;
    // }

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
    //// Possible other options (Some UNKNOWN, Some BUFFER_FULL??}
    return STATUS::INCOMPLETE;
}

// void FramerManager::HandleUnknownBytes(unsigned char* pucBuffer_, const uint32_t uiUnknownBytes_, FRAMER_ID& framerId_)
//{
//     if (bMyReportUnknownBytes && pucBuffer_ != nullptr) { pclMyCircularDataBuffer->Copy(pucBuffer_, uiUnknownBytes_); }
//     pclMyCircularDataBuffer->Discard(uiUnknownBytes_);
//
//     novatelFramer->ResetStateAndByteCount();
//     nmeaFramer->ResetStateAndByteCount();
// }
