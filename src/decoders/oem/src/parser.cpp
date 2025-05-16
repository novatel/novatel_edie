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
// ! \file parser.cpp
// ===============================================================================

#include "novatel_edie/decoders/oem/parser.hpp"

#include "novatel_edie/decoders/common/json_db_reader.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

// -------------------------------------------------------------------------------------------------------
Parser::Parser(const std::filesystem::path& sDbPath_) : pclMyFramer(std::make_shared<Framer>()), clMyRxConfigHandler(pclMyFramer)
{
    try
    {
        auto pclMessageDb = LoadJsonDbFile(sDbPath_);
        LoadJsonDb(pclMessageDb);
        pclMyLogger->info("Parser initialized with JSON DB: {}", sDbPath_.generic_string());
        pclMyLogger->debug("Parser initialized");
    }
    catch (const std::exception& e)
    {
        pclMyLogger->error("Failed to initialize Parser: {}", e.what());
        throw;
    }
}

// -------------------------------------------------------------------------------------------------------
Parser::Parser(MessageDatabase::Ptr pclMessageDb_) : pclMyFramer(std::make_shared<Framer>()), clMyRxConfigHandler(pclMyFramer, pclMessageDb_)
{
    try
    {
        if (pclMessageDb_ != nullptr) { 
            LoadJsonDb(pclMessageDb_); }
        pclMyLogger->debug("Parser initialized");
    }
    catch (const std::exception& e)
    {
        pclMyLogger->error("Failed to initialize Parser: {}", e.what());
        throw;
    }
}

// -------------------------------------------------------------------------------------------------------
void Parser::LoadJsonDb(MessageDatabase::Ptr pclMessageDb_)
{
    if (pclMessageDb_ != nullptr)
    {
        clMyHeaderDecoder.LoadJsonDb(pclMessageDb_);
        clMyMessageDecoder.LoadJsonDb(pclMessageDb_);
        clMyEncoder.LoadJsonDb(pclMessageDb_);
        clMyRangeDecompressor.LoadJsonDb(pclMessageDb_);
        clMyRxConfigHandler.LoadJsonDb(pclMessageDb_);

        clMyRangeCmpFilter.IncludeMessageId(RANGECMP_MSG_ID, HEADER_FORMAT::ALL, static_cast<uint8_t>(MEASUREMENT_SOURCE::PRIMARY));
        clMyRangeCmpFilter.IncludeMessageId(RANGECMP_MSG_ID, HEADER_FORMAT::ALL, static_cast<uint8_t>(MEASUREMENT_SOURCE::SECONDARY));
        clMyRangeCmpFilter.IncludeMessageId(RANGECMP2_MSG_ID, HEADER_FORMAT::ALL, static_cast<uint8_t>(MEASUREMENT_SOURCE::PRIMARY));
        clMyRangeCmpFilter.IncludeMessageId(RANGECMP2_MSG_ID, HEADER_FORMAT::ALL, static_cast<uint8_t>(MEASUREMENT_SOURCE::SECONDARY));
        clMyRangeCmpFilter.IncludeMessageId(RANGECMP3_MSG_ID, HEADER_FORMAT::ALL, static_cast<uint8_t>(MEASUREMENT_SOURCE::PRIMARY));
        clMyRangeCmpFilter.IncludeMessageId(RANGECMP3_MSG_ID, HEADER_FORMAT::ALL, static_cast<uint8_t>(MEASUREMENT_SOURCE::SECONDARY));
        clMyRangeCmpFilter.IncludeMessageId(RANGECMP4_MSG_ID, HEADER_FORMAT::ALL, static_cast<uint8_t>(MEASUREMENT_SOURCE::PRIMARY));
        clMyRangeCmpFilter.IncludeMessageId(RANGECMP4_MSG_ID, HEADER_FORMAT::ALL, static_cast<uint8_t>(MEASUREMENT_SOURCE::SECONDARY));

        pclMyMessageDb = pclMessageDb_;
    }
    else { pclMyLogger->debug("JSON DB is a nullptr."); }
}

// -------------------------------------------------------------------------------------------------------
void Parser::EnableFramerDecoderLogging(spdlog::level::level_enum eLevel_, const std::string& sFileName_)
{
    pclMyFramer->SetLoggerLevel(eLevel_);
    clMyHeaderDecoder.SetLoggerLevel(eLevel_);
    clMyMessageDecoder.SetLoggerLevel(eLevel_);

    CPPLoggerManager* pclMyLoggerManager = GetLoggerManager();
    pclMyLoggerManager->AddConsoleLogging(pclMyFramer->GetLogger());
    pclMyLoggerManager->AddConsoleLogging(clMyHeaderDecoder.GetLogger());
    pclMyLoggerManager->AddConsoleLogging(clMyMessageDecoder.GetLogger());
    pclMyLoggerManager->AddRotatingFileLogger(pclMyFramer->GetLogger(), eLevel_, sFileName_);
    pclMyLoggerManager->AddRotatingFileLogger(clMyHeaderDecoder.GetLogger(), eLevel_, sFileName_);
    pclMyLoggerManager->AddRotatingFileLogger(clMyMessageDecoder.GetLogger(), eLevel_, sFileName_);
}

// -------------------------------------------------------------------------------------------------------
MessageDatabase::ConstPtr Parser::MessageDb() const { return std::const_pointer_cast<const MessageDatabase>(pclMyMessageDb); }

// -------------------------------------------------------------------------------------------------------
STATUS
Parser::ReadIntermediate(MessageDataStruct& stMessageData_, IntermediateHeader& stHeader_, std::vector<FieldContainer>& stMessage_,
                         MetaDataStruct& stMetaData_, bool bDecodeIncompleteAbbreviated_)
{
    while (true)
    {
        pucMyFrameBufferPointer = pcMyFrameBuffer.get(); //!< Reset the buffer.
        auto eStatus = pclMyFramer->GetFrame(pucMyFrameBufferPointer, uiParserInternalBufferSize, stMetaData_);

        // Datasets ending with an Abbreviated ASCII message will always return an incomplete framing status
        // as there is no delimiter marking the end of the log.
        // If bDecodeIncompleteAbbreviated is set, the Framer status is STATUS::INCOMPLETE, and MetaData
        // eFormat is HEADER_FORMAT::ABB_ASCII or HEADER_FORMAT::SHORT_ABB_ASCII then flush the framer
        // and attempt to decode that data.

        if (bDecodeIncompleteAbbreviated_ && eStatus == STATUS::INCOMPLETE &&
            (stMetaData_.eFormat == HEADER_FORMAT::ABB_ASCII || stMetaData_.eFormat == HEADER_FORMAT::SHORT_ABB_ASCII))
        {
            uint32_t uiFlushSize = pclMyFramer->Flush(pucMyFrameBufferPointer, uiParserInternalBufferSize);
            if (uiFlushSize > 0)
            {
                eStatus = STATUS::SUCCESS;
                stMetaData_.uiLength = uiFlushSize;
            }
        }

        stMessageData_.pucMessage = pucMyFrameBufferPointer;
        stMessageData_.uiMessageLength = stMetaData_.uiLength;
        if (eStatus == STATUS::UNKNOWN)
        {
            stMessageData_.pucMessageHeader = nullptr;
            stMessageData_.pucMessageBody = nullptr;
            stMessageData_.uiMessageHeaderLength = 0;
            stMessageData_.uiMessageBodyLength = 0;

            if (bMyReturnUnknownBytes) { return STATUS::UNKNOWN; }
        }
        else if (eStatus == STATUS::SUCCESS)
        {
            if (stMetaData_.bResponse && stMetaData_.eFormat == HEADER_FORMAT::ABB_ASCII && bMyIgnoreAbbreviatedAsciiResponse)
            {
                pclMyLogger->debug("Abbreviated ascii response ignored");
                continue;
            }

            eStatus = clMyHeaderDecoder.Decode(pucMyFrameBufferPointer, stHeader_, stMetaData_);
            if (eStatus == STATUS::SUCCESS)
            {
                if ((pclMyUserFilter != nullptr) && (!pclMyUserFilter->DoFiltering(stMetaData_))) { continue; }

                // Should we decompress this?
                if (clMyRangeCmpFilter.DoFiltering(stMetaData_) && bMyDecompressRangeCmp)
                {
                    eStatus = clMyRangeDecompressor.Decompress(pucMyFrameBufferPointer, uiParserInternalBufferSize, stMetaData_);
                    if (eStatus == STATUS::SUCCESS) { stHeader_.usMessageId = stMetaData_.usMessageId; }
                    else
                    {
                        pclMyLogger->info("RangeDecompressor returned status {}", eStatus);
                        return eStatus;
                    }
                    // Continue if we succeeded.
                }

                pucMyFrameBufferPointer += stMetaData_.uiHeaderLength;
                stMessageData_.pucMessageBody = pucMyFrameBufferPointer;
                stMessageData_.uiMessageBodyLength = stMetaData_.uiLength - stMetaData_.uiHeaderLength;
                if (RxConfigHandler::IsRxConfigTypeMsg(stHeader_.usMessageId))
                {
                    eStatus = clMyRxConfigHandler.Decode(pucMyFrameBufferPointer, stMessage_, stMetaData_);
                }
                else { eStatus = clMyMessageDecoder.Decode(pucMyFrameBufferPointer, stMessage_, stMetaData_); }

                if (eStatus == STATUS::SUCCESS || eStatus == STATUS::NO_DEFINITION) { return eStatus; }

                pclMyLogger->info("MessageDecoder returned status {}", eStatus);
                if (bMyReturnUnknownBytes)
                {
                    stMessageData_.pucMessageBody = nullptr;
                    stMessageData_.uiMessageBodyLength = 0;
                    return STATUS::UNKNOWN;
                }
            }
            else
            {
                pclMyLogger->info("HeaderDecoder returned status {}", eStatus);
                if (bMyReturnUnknownBytes)
                {
                    stMessageData_.pucMessageHeader = nullptr;
                    stMessageData_.pucMessageBody = nullptr;
                    stMessageData_.uiMessageHeaderLength = 0;
                    stMessageData_.uiMessageBodyLength = 0;

                    return STATUS::UNKNOWN;
                }
            }
        }
        else if (eStatus == STATUS::INCOMPLETE || eStatus == STATUS::BUFFER_EMPTY) { return STATUS::BUFFER_EMPTY; }
        else { pclMyLogger->info("Framer returned status {}", eStatus); }
    }
}

STATUS
Parser::Read(MessageDataStruct& stMessageData_, MetaDataStruct& stMetaData_, bool bDecodeIncompleteAbbreviated_)
{
    while (true)
    {
        IntermediateHeader stHeader;
        std::vector<FieldContainer> stMessage;
        STATUS eStatus = ReadIntermediate(stMessageData_, stHeader, stMessage, stMetaData_, bDecodeIncompleteAbbreviated_);
        pucMyEncodeBufferPointer = pcMyEncodeBuffer.get(); //!< Reset the buffer.

        if (eStatus != STATUS::SUCCESS) { return eStatus; }

        // Encode RxConfig messages
        if (RxConfigHandler::IsRxConfigTypeMsg((stHeader.usMessageId)))
        {
            eStatus = clMyRxConfigHandler.Encode(&pucMyEncodeBufferPointer, uiParserInternalBufferSize, stHeader, stMessage, stMessageData_,
                                                 eMyEncodeFormat);
        }
        else
        {
            eStatus = clMyEncoder.Encode(&pucMyEncodeBufferPointer, uiParserInternalBufferSize, stHeader, stMessage, stMessageData_,
                                         stMetaData_.eFormat, eMyEncodeFormat);
        }

        if (eStatus == STATUS::SUCCESS) { return STATUS::SUCCESS; }

        pclMyLogger->info("Encoder returned status {}", eStatus);
    }
}

// -------------------------------------------------------------------------------------------------------
uint32_t Parser::Flush(unsigned char* pucBuffer_, uint32_t uiBufferSize_)
{
    clMyRangeDecompressor.Reset();
    return pclMyFramer->Flush(pucBuffer_, uiBufferSize_);
}
