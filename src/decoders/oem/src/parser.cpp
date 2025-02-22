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
Parser::Parser(const std::filesystem::path& sDbPath_)
{
    auto pclMessageDb = LoadFile(sDbPath_);
    LoadJsonDb(pclMessageDb);
    pclMyLogger->debug("Parser initialized");
}

// -------------------------------------------------------------------------------------------------------
Parser::Parser(MessageDatabase::Ptr pclMessageDb_)
{
    if (pclMessageDb_ != nullptr) { LoadJsonDb(pclMessageDb_); }
    pclMyLogger->debug("Parser initialized");
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

        clMyRangeCmpFilter.IncludeMessageId(RANGECMP_MSG_ID, HEADER_FORMAT::ALL, MEASUREMENT_SOURCE::PRIMARY);
        clMyRangeCmpFilter.IncludeMessageId(RANGECMP_MSG_ID, HEADER_FORMAT::ALL, MEASUREMENT_SOURCE::SECONDARY);
        clMyRangeCmpFilter.IncludeMessageId(RANGECMP2_MSG_ID, HEADER_FORMAT::ALL, MEASUREMENT_SOURCE::PRIMARY);
        clMyRangeCmpFilter.IncludeMessageId(RANGECMP2_MSG_ID, HEADER_FORMAT::ALL, MEASUREMENT_SOURCE::SECONDARY);
        clMyRangeCmpFilter.IncludeMessageId(RANGECMP3_MSG_ID, HEADER_FORMAT::ALL, MEASUREMENT_SOURCE::PRIMARY);
        clMyRangeCmpFilter.IncludeMessageId(RANGECMP3_MSG_ID, HEADER_FORMAT::ALL, MEASUREMENT_SOURCE::SECONDARY);
        clMyRangeCmpFilter.IncludeMessageId(RANGECMP4_MSG_ID, HEADER_FORMAT::ALL, MEASUREMENT_SOURCE::PRIMARY);
        clMyRangeCmpFilter.IncludeMessageId(RANGECMP4_MSG_ID, HEADER_FORMAT::ALL, MEASUREMENT_SOURCE::SECONDARY);
        clMyRxConfigFilter.IncludeMessageId(US_RX_CONFIG_MSG_ID, HEADER_FORMAT::ALL, MEASUREMENT_SOURCE::PRIMARY);
        clMyRxConfigFilter.IncludeMessageId(US_RX_CONFIG_MSG_ID, HEADER_FORMAT::ALL, MEASUREMENT_SOURCE::SECONDARY);

        pclMyMessageDb = pclMessageDb_;
    }
    else { pclMyLogger->debug("JSON DB is a nullptr."); }
}

// -------------------------------------------------------------------------------------------------------
void Parser::EnableFramerDecoderLogging(spdlog::level::level_enum eLevel_, const std::string& sFileName_)
{
    clMyFramer.SetLoggerLevel(eLevel_);
    clMyHeaderDecoder.SetLoggerLevel(eLevel_);
    clMyMessageDecoder.SetLoggerLevel(eLevel_);

    Logger::AddConsoleLogging(clMyFramer.GetLogger());
    Logger::AddConsoleLogging(clMyHeaderDecoder.GetLogger());
    Logger::AddConsoleLogging(clMyMessageDecoder.GetLogger());
    Logger::AddRotatingFileLogger(clMyFramer.GetLogger(), eLevel_, sFileName_);
    Logger::AddRotatingFileLogger(clMyHeaderDecoder.GetLogger(), eLevel_, sFileName_);
    Logger::AddRotatingFileLogger(clMyMessageDecoder.GetLogger(), eLevel_, sFileName_);
}

// -------------------------------------------------------------------------------------------------------
STATUS
Parser::Read(MessageDataStruct& stMessageData_, MetaDataStruct& stMetaData_, bool bDecodeIncompleteAbbreviated_)
{
    IntermediateHeader stHeader;
    std::vector<FieldContainer> stMessage;

    while (true)
    {
        pucMyFrameBufferPointer = pcMyFrameBuffer.get();   //!< Reset the buffer.
        pucMyEncodeBufferPointer = pcMyEncodeBuffer.get(); //!< Reset the buffer.
        auto eStatus = clMyFramer.GetFrame(pucMyFrameBufferPointer, uiParserInternalBufferSize, stMetaData_);

        // Datasets ending with an Abbreviated ASCII message will always return an incomplete framing status
        // as there is no delimiter marking the end of the log.
        // If bDecodeIncompleteAbbreviated is set, the Framer status is STATUS::INCOMPLETE, and MetaData
        // eFormat is HEADER_FORMAT::ABB_ASCII or HEADER_FORMAT::SHORT_ABB_ASCII then flush the framer
        // and attempt to decode that data.

        if (bDecodeIncompleteAbbreviated_ && eStatus == STATUS::INCOMPLETE &&
            (stMetaData_.eFormat == HEADER_FORMAT::ABB_ASCII || stMetaData_.eFormat == HEADER_FORMAT::SHORT_ABB_ASCII))
        {
            uint32_t uiFlushSize = clMyFramer.Flush(pucMyFrameBufferPointer, uiParserInternalBufferSize);
            if (uiFlushSize > 0)
            {
                eStatus = STATUS::SUCCESS;
                stMetaData_.uiLength = uiFlushSize;
            }
        }

        if (eStatus == STATUS::UNKNOWN)
        {
            stMessageData_.uiMessageHeaderLength = 0;
            stMessageData_.uiMessageBodyLength = 0;

            if (bMyReturnUnknownBytes)
            {
                stMessageData_.pucMessageHeader = pucMyFrameBufferPointer;
                stMessageData_.uiMessageHeaderLength = stMetaData_.uiLength;
                stMessageData_.pucMessageBody = nullptr;
                return STATUS::UNKNOWN;
            }
        }
        else if (eStatus == STATUS::SUCCESS)
        {
            if ((!bMyIgnoreAbbreviatedAsciiResponse) && (stMetaData_.bResponse) && (stMetaData_.eFormat == HEADER_FORMAT::ABB_ASCII))
            {
                stMessageData_.uiMessageHeaderLength = 0;
                stMessageData_.pucMessageHeader = nullptr;
                stMessageData_.uiMessageBodyLength = 0;
                stMessageData_.pucMessageBody = nullptr;
                stMessageData_.pucMessage = pucMyFrameBufferPointer;
                stMessageData_.uiMessageLength = stMetaData_.uiLength;
                return STATUS::SUCCESS;
            }

            eStatus = clMyHeaderDecoder.Decode(pucMyFrameBufferPointer, stHeader, stMetaData_);
            if (eStatus == STATUS::SUCCESS)
            {
                if ((pclMyUserFilter != nullptr) && (!pclMyUserFilter->DoFiltering(stMetaData_))) { continue; }

                // Should we decompress this?
                if (clMyRangeCmpFilter.DoFiltering(stMetaData_) && bMyDecompressRangeCmp)
                {
                    eStatus = clMyRangeDecompressor.Decompress(pucMyFrameBufferPointer, uiParserInternalBufferSize, stMetaData_);
                    if (eStatus == STATUS::SUCCESS) { stHeader.usMessageId = stMetaData_.usMessageId; }
                    else
                    {
                        pclMyLogger->info("RangeDecompressor returned status {}", eStatus);
                        return eStatus;
                    }
                    // Continue if we succeeded.
                }

                if (clMyRxConfigFilter.DoFiltering(stMetaData_))
                {
                    // Use some dummy stuff for the embedded message. The parser won't handle that now.
                    MessageDataStruct stEmbeddedMessageData;
                    MetaDataStruct stEmbeddedMetaData;
                    clMyRxConfigHandler.Write(pucMyFrameBufferPointer, stMetaData_.uiLength);
                    eStatus = clMyRxConfigHandler.Convert(stMessageData_, stMetaData_, stEmbeddedMessageData, stEmbeddedMetaData, eMyEncodeFormat);
                    if (eStatus != STATUS::SUCCESS) { pclMyLogger->info("RxConfigHandler returned status {}", eStatus); }
                    return eStatus;
                }

                pucMyFrameBufferPointer += stMetaData_.uiHeaderLength;
                eStatus = clMyMessageDecoder.Decode(pucMyFrameBufferPointer, stMessage, stMetaData_);
                if (eStatus == STATUS::SUCCESS)
                {
                    eStatus = clMyEncoder.Encode(&pucMyEncodeBufferPointer, uiParserInternalBufferSize, stHeader, stMessage, stMessageData_,
                                                 stMetaData_, eMyEncodeFormat);
                    if (eStatus == STATUS::SUCCESS) { return STATUS::SUCCESS; }

                    pclMyLogger->info("Encoder returned status {}", eStatus);
                }
                else { pclMyLogger->info("MessageDecoder returned status {}", eStatus); }
            }
            else { pclMyLogger->info("HeaderDecoder returned status {}", eStatus); }
        }
        else if (eStatus == STATUS::INCOMPLETE || eStatus == STATUS::BUFFER_EMPTY) { return STATUS::BUFFER_EMPTY; }
        else { pclMyLogger->info("Framer returned status {}", eStatus); }
    }
}

// -------------------------------------------------------------------------------------------------------
uint32_t Parser::Flush(unsigned char* pucBuffer_, uint32_t uiBufferSize_)
{
    clMyRangeDecompressor.Reset();
    return clMyFramer.Flush(pucBuffer_, uiBufferSize_);
}
