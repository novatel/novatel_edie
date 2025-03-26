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
    idMap["UNKNOWN"] = 0;
    pclMyLogger->debug("Framer Manager initialized");

    for (const auto& name : selectedFramers)
    {
        // std::cout << name;

        auto it = GetFramerFactories().find(name);
        if (it != GetFramerFactories().end())
        {
            auto& constructors = it->second;

            auto metadataInstance = constructors.second();

            auto framerInstance = constructors.first(pclMyCircularDataBuffer);

            framerRegistry.emplace_back(framerRegistry.size(), std::move(framerInstance), std::move(metadataInstance));
        }
        else { std::cerr << "Warning: Framer '" << name << "' not registered.\n"; }
    }
}

void FramerManager::RegisterFramer(const std::string& name, std::function<std::unique_ptr<FramerBase>(std::shared_ptr<CircularBuffer>)> framerFactory,
                                   std::function<std::unique_ptr<MetaDataBase>()> metadataConstructor)
{
    /*auto& factoryMap = GetFramerFactories();
    factoryMap[name] = {std::move(framerFactory), std::move(metadataConstructor)};*/

    auto& factoryMap = GetFramerFactories();

    // Check if framerFactories is valid before insertion
    std::cerr << "[DEBUG] framerFactories address: " << &factoryMap << "\n";

    // Ensure map isn't corrupted
    if (&factoryMap == nullptr)
    {
        std::cerr << "[ERROR] framerFactories is NULL before insertion!\n";
        return;
    }

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

// void RegisterFramer(const std::string& name,
//                     std::function<std::unique_ptr<FramerBase>(std::shared_ptr<CircularBuffer>, MetaDataBase&)> framerFactory,
//                     MetaDataBase& metadata)
//{
//     auto& factoryMap = GetFramerFactories();
//     factoryMap[name] = {factoryMap.size(), std::move(framerFactory), metadata};
//
//     //// Check if framerFactories is valid before insertion
//     //std::cerr << "[DEBUG] framerFactories address: " << &factoryMap << "\n";
//
//     //// Ensure map isn't corrupted
//     //if (&factoryMap == nullptr)
//     //{
//     //    std::cerr << "[ERROR] framerFactories is NULL before insertion!\n";
//     //    return;
//     //}
//
//     //try
//     //{
//     //    std::cerr << "[DEBUG] Attempting to insert into framerFactories: " << name << "\n";
//     //    factoryMap[name] = { factoryMap.size(), std::move(factory), std::make_unique<MetaDataBase>(metadata)};
//     //    factoryMap[name] = {factoryMap.size(), std::move(FramerFactory), metadata};
//
//     //    std::cerr << "[DEBUG] Successfully inserted framer: " << name << "\n";
//     //}
//     //catch (const std::exception& e)
//     //{
//     //    std::cerr << "[ERROR] Exception inserting into framerFactories: " << e.what() << "\n";
//     //}
// }

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

void FramerManager::SortFramers()
{
    auto it = std::min_element(framerRegistry.begin(), framerRegistry.end(), [](const FramerEntry& a, const FramerEntry& b) {
        return a.framerInstance->uiMyFrameBufferOffset < b.framerInstance->uiMyFrameBufferOffset;
    });

    if (it != framerRegistry.end())
    {
        FramerEntry earliestFramerEntry = std::move(*it);
        framerRegistry.erase(it);
        framerRegistry.push_front(std::move(earliestFramerEntry));
    }
}

MetaDataBase* FramerManager::GetMetaData(const int framerId_)
{
    for (FramerEntry& element : framerRegistry)
    {
        if (element.framerId == framerId_) { return element.metadataInstance.get(); }
    }
    return nullptr;
}

void FramerManager::DisplayFramerStack()
{

    // TODO : Delete this for loop - debugging purposes only
    for (const auto& elem : framerRegistry)
    {
        std::cout << "Framer: " << elem.framerId << ", Offset: " << elem.framerInstance->uiMyFrameBufferOffset
                  << ", Status: " << elem.framerInstance->eMyCurrentFramerStatus << std::endl;

        if (elem.framerId == 1)
        {
            std::cout << "Framer: "
                      << "NOVATEL"
                      << ", Offset: " << elem.framerInstance->uiMyFrameBufferOffset << ", Status: " << elem.framerInstance->eMyCurrentFramerStatus
                      << std::endl;
        }
        else if (elem.framerId == 2)
        {
            std::cout << "Framer: "
                      << "NMEA"
                      << ", Offset: " << elem.framerInstance->uiMyFrameBufferOffset << ", Status: " << elem.framerInstance->eMyCurrentFramerStatus
                      << std::endl;
        }
        else
        {
            std::cout << "Framer: "
                      << "UNKNOWN"
                      << ", Offset: " << elem.framerInstance->uiMyFrameBufferOffset << ", Status: " << elem.framerInstance->eMyCurrentFramerStatus
                      << std::endl;
        }
    }
}

void FramerManager::ResetInactiveFramerStates(const int& eActiveFramerId_)
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

void FramerManager::ResetAllFramerStates() { ResetInactiveFramerStates(idMap["UNKNOWN"]); }

void FramerManager::ResetInactiveMetaDataStates(const int& eActiveFramerId_)
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

void FramerManager::ResetAllMetaDataStates() { ResetInactiveMetaDataStates(idMap["UNKNOWN"]); }

FramerEntry* FramerManager::GetFramerElement(const int framerId_)
{
    for (FramerEntry& element : framerRegistry)
    {
        if (element.framerId == framerId_) { return &element; }
    }
    return nullptr;
}

STATUS FramerManager::GetFrame(unsigned char* pucFrameBuffer_, uint32_t uiFrameBufferSize_, int& eActiveFramerId_)
{
    // State Machine:
    // use framers to identify sync bytes and offsets
    // once sync bytes are identified, set Active Framer and perform framing using type specific framer, reset inactive framers
    // upon successful framing of a log, reset Active Framer ID

    // if STATUS is INCOMPLETE upon entering GetFrame, it is stale
    if (eActiveFramerId_ != idMap["UNKNOWN"] && framerRegistry.front().framerInstance->eMyCurrentFramerStatus == STATUS::INCOMPLETE)
    {
        framerRegistry.front().framerInstance->eMyCurrentFramerStatus = STATUS::INCOMPLETE_MORE_DATA;
    }

    if (eActiveFramerId_ == idMap["UNKNOWN"])
    {
        ResetAllMetaDataStates();
        for (auto& [framerId, framer, metadata, offset] : framerRegistry) { framer->FindNextSyncByte(uiFrameBufferSize_); }
    }

    // A Framer Found A Sync Byte
    // DisplayFramerStack();
    SortFramers();

    // set uiLength for likely framer
    framerRegistry.front().metadataInstance->uiLength = framerRegistry.front().framerInstance->uiMyFrameBufferOffset;
    if (framerRegistry.front().framerInstance->eMyCurrentFramerStatus == STATUS::INCOMPLETE) { return STATUS::INCOMPLETE; }

    eActiveFramerId_ = framerRegistry.front().framerId;

    if (framerRegistry.front().framerInstance->uiMyFrameBufferOffset > 0 &&
        (framerRegistry.front().framerInstance->eMyCurrentFramerStatus != STATUS::INCOMPLETE ||
         framerRegistry.front().framerInstance->eMyCurrentFramerStatus != STATUS::INCOMPLETE_MORE_DATA))
    {
        HandleUnknownBytes(pucFrameBuffer_, framerRegistry.front().framerInstance->uiMyFrameBufferOffset);
        ResetAllFramerStates();
        return STATUS::UNKNOWN;
    }

    // Once active framer is identified, exclusively use it to frame. Reset active Framer at success
    for (auto& framer_element : framerRegistry)
    {
        if (framer_element.framerId == eActiveFramerId_)
        {
            ResetInactiveFramerStates(framer_element.framerId);
            framer_element.framerInstance->eMyCurrentFramerStatus =
                framer_element.framerInstance->GetFrame(pucFrameBuffer_, uiFrameBufferSize_, *framer_element.metadataInstance);
            DisplayFramerStack();
            if (framer_element.framerInstance->eMyCurrentFramerStatus == STATUS::SUCCESS)
            {
                pclMyCircularDataBuffer->Copy(pucFrameBuffer_, framer_element.metadataInstance->uiLength);
                pclMyCircularDataBuffer->Discard(framer_element.metadataInstance->uiLength);
            }
            else if (framer_element.framerInstance->eMyCurrentFramerStatus == STATUS::UNKNOWN)
            {
                HandleUnknownBytes(pucFrameBuffer_, framer_element.framerInstance->uiMyFrameBufferOffset);
                ResetAllFramerStates();
                return framer_element.framerInstance->eMyCurrentFramerStatus;
            }
            break;
        }
    }

    // A Framer Successfully Framed a log
    auto& it_success = std::find_if(framerRegistry.begin(), framerRegistry.end(),
                                    [](FramerEntry& element) { return (element.framerInstance->eMyCurrentFramerStatus == STATUS::SUCCESS); });
    if (it_success != framerRegistry.end()) { return it_success->framerInstance->eMyCurrentFramerStatus; }

    // if any framer has buffer full, reset all framers
    auto it_buffer_full = std::find_if(framerRegistry.begin(), framerRegistry.end(),
                                       [](FramerEntry& element) { return (element.framerInstance->eMyCurrentFramerStatus == STATUS::BUFFER_FULL); });
    if (it_buffer_full != framerRegistry.end())
    {
        ResetAllFramerStates();
        return STATUS::BUFFER_FULL;
    }

    auto it_null_provided = std::find_if(framerRegistry.begin(), framerRegistry.end(), [](FramerEntry& element) {
        return (element.framerInstance->eMyCurrentFramerStatus == STATUS::NULL_PROVIDED);
    });
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
