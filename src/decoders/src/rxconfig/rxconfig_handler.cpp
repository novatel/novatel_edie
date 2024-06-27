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
// ! \file rxconfig_handler.cpp
// ===============================================================================

#include "novatel_edie/decoders/rxconfig/rxconfig_handler.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

// -------------------------------------------------------------------------------------------------------
RxConfigHandler::RxConfigHandler(JsonReader* pclJsonDB_)
    : clMyHeaderDecoder(pclJsonDB_), clMyMessageDecoder(pclJsonDB_), clMyEncoder(pclJsonDB_),
      pcMyFrameBuffer(std::make_unique<unsigned char[]>(uiInternalBufferSize)),
      pcMyEncodeBuffer(std::make_unique<unsigned char[]>(uiInternalBufferSize))
{
    pclMyLogger = Logger::RegisterLogger("rxconfig_handler");

    pclMyLogger->debug("RxConfigHandler initializing...");

    if (pclJsonDB_ != nullptr) { LoadJsonDb(pclJsonDB_); }

    pclMyLogger->debug("RxConfigHandler initialized");
}

// -------------------------------------------------------------------------------------------------------
void RxConfigHandler::LoadJsonDb(JsonReader* pclJsonDB_)
{
    pclMyMsgDb = pclJsonDB_;
    clMyHeaderDecoder.LoadJsonDb(pclJsonDB_);
    clMyMessageDecoder.LoadJsonDb(pclJsonDB_);
    clMyEncoder.LoadJsonDb(pclJsonDB_);

    vMyCommandDefinitions = pclMyMsgDb->GetEnumDefName("Commands");
    vMyPortAddressDefinitions = pclMyMsgDb->GetEnumDefName("PortAddress");
    vMyGpsTimeStatusDefinitions = pclMyMsgDb->GetEnumDefName("GPSTimeStatus");
}

// -------------------------------------------------------------------------------------------------------
void RxConfigHandler::SetLoggerLevel(spdlog::level::level_enum eLevel_) const { pclMyLogger->set_level(eLevel_); }

// -------------------------------------------------------------------------------------------------------
void RxConfigHandler::ShutdownLogger() { Logger::Shutdown(); }

// -------------------------------------------------------------------------------------------------------
std::shared_ptr<spdlog::logger> RxConfigHandler::GetLogger() const { return pclMyLogger; }

//-----------------------------------------------------------------------
bool RxConfigHandler::IsRxConfigTypeMsg(uint16_t usMessageId_)
{
    return (usMessageId_ == US_RX_CONFIG_MSG_ID || usMessageId_ == US_RX_CONFIG_USER_MSG_ID);
}

// -------------------------------------------------------------------------------------------------------
uint32_t RxConfigHandler::Write(unsigned char* pucData_, uint32_t uiDataSize_) { return clMyFramer.Write(pucData_, uiDataSize_); }

// -------------------------------------------------------------------------------------------------------
STATUS
RxConfigHandler::Convert(MessageDataStruct& stRxConfigMessageData_, MetaDataStruct& stRxConfigMetaData_, MessageDataStruct& stEmbeddedMessageData_,
                         MetaDataStruct& stEmbeddedMetaData_, ENCODE_FORMAT eEncodeFormat_)
{
    IntermediateHeader stRxConfigHeader;
    IntermediateHeader stEmbeddedHeader;
    std::vector<FieldContainer> stEmbeddedMessage;

    unsigned char* pucTempMessagePointer = pcMyFrameBuffer.get();

    // Get an RXCONFIG log.
    STATUS eStatus = clMyFramer.GetFrame(pcMyFrameBuffer.get(), uiInternalBufferSize, stRxConfigMetaData_);
    if (eStatus == STATUS::BUFFER_EMPTY || eStatus == STATUS::INCOMPLETE) { return STATUS::BUFFER_EMPTY; }
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    // Decode the RXCONFIG log.
    eStatus = clMyHeaderDecoder.Decode(pcMyFrameBuffer.get(), stRxConfigHeader, stRxConfigMetaData_);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }
    pucTempMessagePointer += stRxConfigMetaData_.uiHeaderLength;

    // If we have something that isn't RXCONFIG, get rid of it.
    if (!IsRxConfigTypeMsg(stRxConfigMetaData_.usMessageId)) { return STATUS::UNKNOWN; }

    if (stRxConfigMetaData_.eFormat == HEADER_FORMAT::ABB_ASCII)
    {
        // Abbreviated ASCII RXCONFIG logs have indentations on the embedded header.  The
        // HeaderDecoder does not expect this and the spaces must be removed.  Remove "<     ", then
        // put '<' back at the beginning so the header is treated correctly.
        ConsumeAbbrevFormatting(0, reinterpret_cast<char**>(&pucTempMessagePointer));
        pucTempMessagePointer -= OEM4_ASCII_SYNC_LENGTH;
        *pucTempMessagePointer = OEM4_ABBREV_ASCII_SYNC;
    }

    eStatus = clMyHeaderDecoder.Decode(pucTempMessagePointer, stEmbeddedHeader, stEmbeddedMetaData_);
    if (eStatus == STATUS::NO_DEFINITION) { return STATUS::NO_DEFINITION_EMBEDDED; }
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    // Put a std::vector<FieldContainer> into the RXCONFIG IntermediateMessage, then
    // pass it to the message decoder as a destination for the embedded message.
    eStatus = clMyMessageDecoder.Decode((pucTempMessagePointer + stEmbeddedMetaData_.uiHeaderLength), stEmbeddedMessage, stEmbeddedMetaData_);
    if (eStatus == STATUS::NO_DEFINITION) { return STATUS::NO_DEFINITION_EMBEDDED; }
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    // Encode the RXCONFIG log.
    unsigned char* pucTempEncodeBuffer = pcMyEncodeBuffer.get();
    uint32_t uiMyBufferBytesRemaining = uiInternalBufferSize;

    if (eEncodeFormat_ == ENCODE_FORMAT::JSON &&
        !PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiMyBufferBytesRemaining, R"({"header":)"))
    {
        return STATUS::BUFFER_FULL;
    }

    eStatus = clMyEncoder.EncodeHeader(&pucTempEncodeBuffer, uiMyBufferBytesRemaining, stRxConfigHeader, stRxConfigMessageData_, stRxConfigMetaData_,
                                       eEncodeFormat_);
    if (eStatus == STATUS::NO_DEFINITION) { return STATUS::NO_DEFINITION_EMBEDDED; }
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    pucTempEncodeBuffer += stRxConfigMessageData_.uiMessageHeaderLength;

    if (eEncodeFormat_ == ENCODE_FORMAT::JSON &&
        !PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiMyBufferBytesRemaining, R"(,"body":)"))
    {
        return STATUS::BUFFER_FULL;
    }

    stEmbeddedMessageData_.pucMessage = pucTempEncodeBuffer;
    stRxConfigMessageData_.pucMessageBody = pucTempEncodeBuffer;

    // This is just dummy args that we must pass to the encoder.  They will not be used.
    uint32_t uiCRC = 0;

    switch (eEncodeFormat_)
    {
    case ENCODE_FORMAT::JSON:
        if (!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiMyBufferBytesRemaining, R"({"embedded_header":)"))
        {
            return STATUS::BUFFER_FULL;
        }
        break;

    case ENCODE_FORMAT::ABBREV_ASCII:
        if (!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiMyBufferBytesRemaining, szAbbrevAsciiEmbeddedHeaderPrefix))
        {
            return STATUS::BUFFER_FULL;
        }
        break;

    default: break;
    }

    eStatus = clMyEncoder.EncodeHeader(&pucTempEncodeBuffer, uiMyBufferBytesRemaining, stEmbeddedHeader, stEmbeddedMessageData_, stEmbeddedMetaData_,
                                       eEncodeFormat_, true);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    pucTempEncodeBuffer += stEmbeddedMessageData_.uiMessageHeaderLength;
    uiMyBufferBytesRemaining -= stEmbeddedMessageData_.uiMessageHeaderLength;

    switch (eEncodeFormat_)
    {
    case ENCODE_FORMAT::JSON:
        if (!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiMyBufferBytesRemaining, R"(,"embedded_body":)"))
        {
            return STATUS::BUFFER_FULL;
        }
        break;

    case ENCODE_FORMAT::ABBREV_ASCII:
        // The header is going to be pointing to the wrong location, so reverse it back to
        // before the "<     " characters.
        stEmbeddedMessageData_.pucMessageHeader -= strlen(szAbbrevAsciiEmbeddedHeaderPrefix);
        stEmbeddedMessageData_.uiMessageHeaderLength += static_cast<uint32_t>(strlen(szAbbrevAsciiEmbeddedHeaderPrefix));
        // A normal abbreviated ASCII log would remove the final ' ' delimiter, however since
        // this is part of a message body, we should encode it to follow the standard of
        // trailing spaces in the message body.  EncodeHeader() would have removed this, so add
        // it back in the message MessageHeaderLength count.
        stEmbeddedMessageData_.uiMessageHeaderLength++;

        if (!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiMyBufferBytesRemaining, "\r\n")) { return STATUS::BUFFER_FULL; }

        stEmbeddedMessageData_.uiMessageHeaderLength++;
        break;

    default: break;
    }

    eStatus = clMyEncoder.EncodeBody(&pucTempEncodeBuffer, uiMyBufferBytesRemaining, stEmbeddedMessage, stEmbeddedMessageData_, stEmbeddedMetaData_,
                                     eEncodeFormat_);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    uiMyBufferBytesRemaining -= stEmbeddedMessageData_.uiMessageBodyLength;
    pucTempEncodeBuffer += stEmbeddedMessageData_.uiMessageBodyLength;

    // The last CRC would have been written correctly.  Pull it out, flip it, put it back in.
    // This will be done differently depending on how we encoded the message.
    switch (eEncodeFormat_)
    {
    case ENCODE_FORMAT::ASCII:
        // Move back over CRLF.
        pucTempEncodeBuffer -= 2;
        uiMyBufferBytesRemaining += 2;
        stEmbeddedMessageData_.uiMessageBodyLength -= 2;
        // Move back over the CRC.
        pucTempEncodeBuffer -= OEM4_ASCII_CRC_LENGTH;
        uiMyBufferBytesRemaining += OEM4_ASCII_CRC_LENGTH;
        // Grab the CRC from the encode buffer and invert it.
        uiCRC = strtoul(reinterpret_cast<char*>(pucTempEncodeBuffer), nullptr, 16) ^ 0xFFFFFFFF;
        if (!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiMyBufferBytesRemaining, "%08x", uiCRC)) { return STATUS::BUFFER_FULL; }
        break;

    case ENCODE_FORMAT::BINARY:
        // Move back over the CRC.
        pucTempEncodeBuffer -= OEM4_BINARY_CRC_LENGTH;
        uiMyBufferBytesRemaining += OEM4_BINARY_CRC_LENGTH;
        // Grab the CRC from the encode buffer and invert it.
        uiCRC = *(reinterpret_cast<uint32_t*>(pucTempEncodeBuffer)) ^ 0xFFFFFFFF;
        if (!CopyToBuffer(&pucTempEncodeBuffer, uiMyBufferBytesRemaining, &uiCRC)) { return STATUS::BUFFER_FULL; }
        break;

    case ENCODE_FORMAT::JSON:
        if (!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiMyBufferBytesRemaining, R"(})")) { return STATUS::BUFFER_FULL; }
        break;

    default: break;
    }

    stEmbeddedMessageData_.uiMessageLength = static_cast<uint32_t>(pucTempEncodeBuffer - stRxConfigMessageData_.pucMessageBody);

    // Put the final CRC at the end.
    switch (eEncodeFormat_)
    {
    case ENCODE_FORMAT::ASCII:
        uiCRC = CalculateBlockCrc32(static_cast<uint32_t>(pucTempEncodeBuffer - (pcMyEncodeBuffer.get() + 1)), 0, pcMyEncodeBuffer.get() + 1);
        if (!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiMyBufferBytesRemaining, "*%08x\r\n", uiCRC))
        {
            return STATUS::BUFFER_FULL;
        }
        break;

    case ENCODE_FORMAT::BINARY:
        uiCRC = CalculateBlockCrc32(static_cast<uint32_t>(pucTempEncodeBuffer - pcMyEncodeBuffer.get()), 0, pcMyEncodeBuffer.get());
        if (!CopyToBuffer(&pucTempEncodeBuffer, uiMyBufferBytesRemaining, &uiCRC)) { return STATUS::BUFFER_FULL; }
        break;

    default: break;
    }

    stRxConfigMessageData_.uiMessageBodyLength = static_cast<uint32_t>(pucTempEncodeBuffer - stRxConfigMessageData_.pucMessageBody);

    // Add the closing '}' character, but don't count it as part of the message body length.
    if (eEncodeFormat_ == ENCODE_FORMAT::JSON && !PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiMyBufferBytesRemaining, R"(})"))
    {
        return STATUS::BUFFER_FULL;
    }

    stRxConfigMessageData_.pucMessage = pcMyEncodeBuffer.get();
    stRxConfigMessageData_.uiMessageLength = static_cast<uint32_t>(pucTempEncodeBuffer - pcMyEncodeBuffer.get());

    return STATUS::SUCCESS;
}

// -------------------------------------------------------------------------------------------------------
uint32_t RxConfigHandler::Flush(unsigned char* pucBuffer_, uint32_t uiBufferSize_) { return clMyFramer.Flush(pucBuffer_, uiBufferSize_); }
