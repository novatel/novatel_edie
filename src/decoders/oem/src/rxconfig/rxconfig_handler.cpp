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

#include "novatel_edie/decoders/oem/rxconfig/rxconfig_handler.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

// -------------------------------------------------------------------------------------------------------
RxConfigHandler::RxConfigHandler(const MessageDatabase::Ptr& pclMessageDb_)
    : clMyHeaderDecoder(pclMessageDb_), clMyMessageDecoder(pclMessageDb_), clMyEncoder(pclMessageDb_),
      pcMyFrameBuffer(std::make_unique<unsigned char[]>(uiInternalBufferSize)),
      pcMyEncodeBuffer(std::make_unique<unsigned char[]>(uiInternalBufferSize))
{
    pclMyLogger = GetBaseLoggerManager()->RegisterLogger("rxconfig_handler");

    pclMyLogger->debug("RxConfigHandler initializing...");

    if (pclMessageDb_ != nullptr) { LoadJsonDb(pclMessageDb_); }

    pclMyLogger->debug("RxConfigHandler initialized");
}

// -------------------------------------------------------------------------------------------------------
void RxConfigHandler::LoadJsonDb(const MessageDatabase::Ptr& pclMessageDb_)
{
    pclMyMsgDb = pclMessageDb_;
    clMyHeaderDecoder.LoadJsonDb(pclMessageDb_);
    clMyMessageDecoder.LoadJsonDb(pclMessageDb_);
    clMyEncoder.LoadJsonDb(pclMessageDb_);

    vMyCommandDefinitions = pclMyMsgDb->GetEnumDefName("Commands");
    vMyPortAddressDefinitions = pclMyMsgDb->GetEnumDefName("PortAddress");
    vMyGpsTimeStatusDefinitions = pclMyMsgDb->GetEnumDefName("GPSTimeStatus");
}

//-----------------------------------------------------------------------
bool RxConfigHandler::IsRxConfigTypeMsg(uint16_t usMessageId_)
{
    return (usMessageId_ == US_RX_CONFIG_MSG_ID || usMessageId_ == US_RX_CONFIG_USER_MSG_ID);
}

// -------------------------------------------------------------------------------------------------------
bool RxConfigHandler::Write(const unsigned char* pucData_, uint32_t uiDataSize_) { return clMyFramer.Write(pucData_, uiDataSize_); }

// -------------------------------------------------------------------------------------------------------
STATUS RxConfigHandler::Decode(const unsigned char* pucMessage_, std::vector<FieldContainer>& stInterMessage_, MetaDataStruct& stRxConfigMetaData_)
{
    const unsigned char* pucTempMessagePointer = pucMessage_;
    IntermediateHeader stEmbeddedHeader;
    MetaDataStruct stEmbeddedMetaData_;
    std::vector<FieldContainer> stEmbeddedMessage;
    uint32_t uiTotalPayloadSize = stRxConfigMetaData_.uiLength - stRxConfigMetaData_.uiHeaderLength;

    BaseField::ConstPtr pclFieldDef = pclMyMsgDb->pclRXConfigField;
    stInterMessage_.emplace_back(std::vector<FieldContainer>(), pclFieldDef);
    auto& stEmbeddedMessageData = std::get<std::vector<FieldContainer>>(stInterMessage_.back().fieldValue);

    uint32_t uiCopyableEmbeddedMsgBytes;
    switch (stRxConfigMetaData_.eFormat)
    {
    case HEADER_FORMAT::ABB_ASCII: {
        // Fix embedded header indentation
        ConsumeAbbrevFormatting(reinterpret_cast<const char**>(&pucTempMessagePointer));
        stEmbeddedMessageData.emplace_back(OEM4_ABBREV_ASCII_SYNC, pclFieldDef);
        uiCopyableEmbeddedMsgBytes = uiTotalPayloadSize - (pucTempMessagePointer - pucMessage_);
        break;
    }
    case HEADER_FORMAT::ASCII: {
        // Ignore {CRC}*{CRC}\r\n at end of buffer
        uiCopyableEmbeddedMsgBytes = uiTotalPayloadSize - OEM4_ASCII_CRC_LENGTH - 1 - OEM4_ASCII_CRC_LENGTH - 2;
        break;
    }
    case HEADER_FORMAT::BINARY: {
        uiCopyableEmbeddedMsgBytes = uiTotalPayloadSize - 2 * OEM4_BINARY_CRC_LENGTH;
        break;
    }
    default: {
        pclMyLogger->error("Unsupported RXCONFIG format: {}", static_cast<uint8_t>(stRxConfigMetaData_.eFormat));
        return STATUS::UNSUPPORTED;
    }
    }

    // Copy embedded message data into raw representation
    stEmbeddedMessageData.reserve(uiCopyableEmbeddedMsgBytes);
    for (uint32_t i = 0; i < uiCopyableEmbeddedMsgBytes; ++i)
    {
        stEmbeddedMessageData.emplace_back(*(reinterpret_cast<const uint8_t*>(pucTempMessagePointer)), pclFieldDef);
        pucTempMessagePointer++;
    }

    switch (stRxConfigMetaData_.eFormat)
    {
    case HEADER_FORMAT::ASCII: {
        // invert ascii crc
        const uint32_t uiCRC = strtoul(reinterpret_cast<const char*>(pucTempMessagePointer), nullptr, 16) ^ 0xFFFFFFFF;
        char pucCRC[OEM4_ASCII_CRC_LENGTH];
        std::to_chars(pucCRC, pucCRC + OEM4_ASCII_CRC_LENGTH, uiCRC, 16);
        for (uint32_t i = 0; i < OEM4_ASCII_CRC_LENGTH; ++i)
        {
            stEmbeddedMessageData.emplace_back(*(reinterpret_cast<uint8_t*>(pucCRC) + i), pclFieldDef);
        }
        stEmbeddedMessageData.emplace_back(static_cast<uint8_t>('\r'), pclFieldDef);
        stEmbeddedMessageData.emplace_back(static_cast<uint8_t>('\n'), pclFieldDef);
        break;
    }
    case HEADER_FORMAT::BINARY: {
        uint32_t uiCRC = *reinterpret_cast<const uint32_t*>(pucTempMessagePointer) ^ 0xFFFFFFFF;
        stEmbeddedMessageData.emplace_back(static_cast<uint8_t>(uiCRC >> 24), pclFieldDef);
        stEmbeddedMessageData.emplace_back(static_cast<uint8_t>(uiCRC >> 16), pclFieldDef);
        stEmbeddedMessageData.emplace_back(static_cast<uint8_t>(uiCRC >> 8), pclFieldDef);
        stEmbeddedMessageData.emplace_back(static_cast<uint8_t>(uiCRC), pclFieldDef);
        break;
    }
    default: break;
    }

    return STATUS::SUCCESS;
}

[[nodiscard]] STATUS RxConfigHandler::Encode(unsigned char** ppucBuffer_, uint32_t uiBufferSize_, const IntermediateHeader& stHeader_,
                                             const std::vector<FieldContainer>& stMessage_, MessageDataStruct& stMessageData_,
                                             ENCODE_FORMAT eFormat_) const
{
    if (ppucBuffer_ == nullptr || *ppucBuffer_ == nullptr) { return STATUS::NULL_PROVIDED; }

    unsigned char* pucTempEncodeBuffer = *ppucBuffer_;
    stMessageData_.pucMessage = *ppucBuffer_;
    stMessageData_.pucMessageHeader = *ppucBuffer_;

    // Abuse the fact that header format is only used for determining whether the header is long or short
    STATUS eStatus = clMyEncoder.EncodeHeader(ppucBuffer_, uiBufferSize_, stHeader_, stMessageData_, HEADER_FORMAT::BINARY, eFormat_);
    if (eStatus != STATUS::SUCCESS)
    {
        pclMyLogger->error("Encoder returned status {}", eStatus);
        return eStatus;
    }
    pucTempEncodeBuffer += stMessageData_.uiMessageHeaderLength;

    // Encode the RXCONFIG message body.
    IntermediateHeader stEmbeddedHeader;
    std::vector<FieldContainer> stEmbeddedMessage;
    MetaDataStruct stEmbeddedMetaData;
    MessageDataStruct stEmbeddedMessageData;

    auto& vEmbeddedData = std::get<std::vector<FieldContainer>>(stMessage_[0].fieldValue);

    // Convert data to a dynamically allocated character array
    std::unique_ptr<unsigned char[]> pucEmbeddedDataBuffer = std::make_unique<unsigned char[]>(vEmbeddedData.size());
    unsigned char* pucEmbeddedDataPointer = pucEmbeddedDataBuffer.get();
    for (const auto& stField : vEmbeddedData) { *pucEmbeddedDataPointer++ = std::get<uint8_t>(stField.fieldValue); }

    eStatus = clMyHeaderDecoder.Decode(pucEmbeddedDataBuffer.get(), stEmbeddedHeader, stEmbeddedMetaData);
    if (eStatus != STATUS::SUCCESS)
    {
        pclMyLogger->error("HeaderDecoder returned status {}", eStatus);
        return eStatus;
    }

    eStatus = clMyMessageDecoder.Decode(pucEmbeddedDataBuffer.get() + stEmbeddedMetaData.uiHeaderLength, stEmbeddedMessage, stEmbeddedMetaData);
    if (eStatus != STATUS::SUCCESS)
    {
        pclMyLogger->error("MessageDecoder returned status {}", eStatus);
        return eStatus;
    }

    eStatus = clMyEncoder.Encode(&pucTempEncodeBuffer, uiBufferSize_, stEmbeddedHeader, stEmbeddedMessage, stEmbeddedMessageData,
                                 stEmbeddedMetaData.eFormat, eFormat_);
    if (eStatus != STATUS::SUCCESS)
    {
        pclMyLogger->error("Encoder returned status {}", eStatus);
        return eStatus;
    }

    // Copy over CRC
    switch (eFormat_)
    {
    case ENCODE_FORMAT::FLATTENED_BINARY: [[fallthrough]];
    case ENCODE_FORMAT::BINARY: {
        break;
    }
    case ENCODE_FORMAT::ABBREV_ASCII: {
        break;
    }
    case ENCODE_FORMAT::ASCII: {
        // Encode embedded CRC
        // Backtrack over '{CRC}' and '\r\n'
        uint32_t uiBacktrackAmount = OEM4_ASCII_CRC_LENGTH + 2;
        pucTempEncodeBuffer = stEmbeddedMessageData.pucMessage + stEmbeddedMessageData.uiMessageLength - uiBacktrackAmount;
        uiBufferSize_ += uiBacktrackAmount;
        // Ignore '#' and '*' in CRC calculation
        uint32_t uiCrc =
            CalculateBlockCrc32(stEmbeddedMessageData.pucMessage + 1, stEmbeddedMessageData.uiMessageLength - 1 - 1 - uiBacktrackAmount) ^ 0xFFFFFFFF;
        if (!CopyAllToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiBufferSize_, HexValue<uint32_t>{uiCrc, 8}))
        {
            return STATUS::BUFFER_FULL;
        }

        // Encode regular CRC
        unsigned char* pucStartPoint = *ppucBuffer_ + 1; // Skip the first byte, which is the sync byte
        uiCrc = CalculateBlockCrc32(pucStartPoint, pucTempEncodeBuffer - pucStartPoint);
        if (!CopyAllToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiBufferSize_, '*', HexValue<uint32_t>{uiCrc, 8}, "\r\n"))
        {
            return STATUS::BUFFER_FULL;
        }
    }
    }
    stMessageData_.uiMessageLength = pucTempEncodeBuffer - *ppucBuffer_;
    stMessageData_.pucMessageBody = *ppucBuffer_ + stMessageData_.uiMessageHeaderLength;
    stMessageData_.uiMessageBodyLength = stMessageData_.uiMessageLength - stMessageData_.uiMessageHeaderLength;

    return STATUS::SUCCESS;
}

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
        // Abbreviated ASCII RXCONFIG logs have indentations on the embedded header. The
        // HeaderDecoder does not expect this and the spaces must be removed. Remove "<     ", then
        // put '<' back at the beginning so the header is treated correctly.
        ConsumeAbbrevFormatting(const_cast<const char**>(reinterpret_cast<char**>(&pucTempMessagePointer)));
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

    if (eEncodeFormat_ == ENCODE_FORMAT::JSON && !CopyToBuffer(&pucTempEncodeBuffer, uiMyBufferBytesRemaining, R"({"header":)"))
    {
        return STATUS::BUFFER_FULL;
    }

    eStatus = clMyEncoder.EncodeHeader(&pucTempEncodeBuffer, uiMyBufferBytesRemaining, stRxConfigHeader, stRxConfigMessageData_,
                                       stRxConfigMetaData_.eFormat, eEncodeFormat_);
    if (eStatus == STATUS::NO_DEFINITION) { return STATUS::NO_DEFINITION_EMBEDDED; }
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    pucTempEncodeBuffer += stRxConfigMessageData_.uiMessageHeaderLength;

    if (eEncodeFormat_ == ENCODE_FORMAT::JSON && !CopyToBuffer(&pucTempEncodeBuffer, uiMyBufferBytesRemaining, R"(,"body":)"))
    {
        return STATUS::BUFFER_FULL;
    }

    stEmbeddedMessageData_.pucMessage = pucTempEncodeBuffer;
    stRxConfigMessageData_.pucMessageBody = pucTempEncodeBuffer;

    switch (eEncodeFormat_)
    {
    case ENCODE_FORMAT::JSON:
        if (!CopyToBuffer(&pucTempEncodeBuffer, uiMyBufferBytesRemaining, R"({"embedded_header":)")) { return STATUS::BUFFER_FULL; }
        break;

    case ENCODE_FORMAT::ABBREV_ASCII:
        if (!CopyToBuffer(&pucTempEncodeBuffer, uiMyBufferBytesRemaining, szAbbrevAsciiEmbeddedHeaderPrefix)) { return STATUS::BUFFER_FULL; }
        break;

    default: break;
    }

    eStatus = clMyEncoder.EncodeHeader(&pucTempEncodeBuffer, uiMyBufferBytesRemaining, stEmbeddedHeader, stEmbeddedMessageData_,
                                       stEmbeddedMetaData_.eFormat, eEncodeFormat_, true);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    pucTempEncodeBuffer += stEmbeddedMessageData_.uiMessageHeaderLength;
    uiMyBufferBytesRemaining -= stEmbeddedMessageData_.uiMessageHeaderLength;

    switch (eEncodeFormat_)
    {
    case ENCODE_FORMAT::JSON:
        if (!CopyToBuffer(&pucTempEncodeBuffer, uiMyBufferBytesRemaining, R"(,"embedded_body":)")) { return STATUS::BUFFER_FULL; }
        break;

    case ENCODE_FORMAT::ABBREV_ASCII:
        // The header is going to be pointing to the wrong location, so reverse it back to
        // before the "<     " characters.
        stEmbeddedMessageData_.pucMessageHeader -= szAbbrevAsciiEmbeddedHeaderPrefix.size();
        stEmbeddedMessageData_.uiMessageHeaderLength += static_cast<uint32_t>(szAbbrevAsciiEmbeddedHeaderPrefix.size());
        // A normal abbreviated ASCII log would remove the final ' ' delimiter, however since
        // this is part of a message body, we should encode it to follow the standard of
        // trailing spaces in the message body. EncodeHeader() would have removed this, so add
        // it back in the message MessageHeaderLength count.
        stEmbeddedMessageData_.uiMessageHeaderLength++;

        if (!CopyToBuffer(&pucTempEncodeBuffer, uiMyBufferBytesRemaining, "\r\n")) { return STATUS::BUFFER_FULL; }

        stEmbeddedMessageData_.uiMessageHeaderLength++;
        break;

    default: break;
    }

    eStatus = clMyEncoder.EncodeBody(&pucTempEncodeBuffer, uiMyBufferBytesRemaining, stEmbeddedMessage, stEmbeddedMessageData_,
                                     stEmbeddedMetaData_.eFormat, eEncodeFormat_);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    uiMyBufferBytesRemaining -= stEmbeddedMessageData_.uiMessageBodyLength;
    pucTempEncodeBuffer += stEmbeddedMessageData_.uiMessageBodyLength;

    // The last CRC would have been written correctly. Pull it out, flip it, put it back in.
    // This will be done differently depending on how we encoded the message.
    switch (eEncodeFormat_)
    {
    case ENCODE_FORMAT::ASCII: {
        // Move back over CRLF.
        pucTempEncodeBuffer -= 2;
        uiMyBufferBytesRemaining += 2;
        stEmbeddedMessageData_.uiMessageBodyLength -= 2;
        // Move back over the CRC.
        pucTempEncodeBuffer -= OEM4_ASCII_CRC_LENGTH;
        uiMyBufferBytesRemaining += OEM4_ASCII_CRC_LENGTH;
        // Grab the CRC from the encode buffer and invert it.
        const uint32_t crc = strtoul(reinterpret_cast<char*>(pucTempEncodeBuffer), nullptr, 16) ^ 0xFFFFFFFF;
        if (!WriteHexToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiMyBufferBytesRemaining, crc, 8)) { return STATUS::BUFFER_FULL; }
        break;
    }
    case ENCODE_FORMAT::BINARY: {
        // Move back over the CRC.
        pucTempEncodeBuffer -= OEM4_BINARY_CRC_LENGTH;
        uiMyBufferBytesRemaining += OEM4_BINARY_CRC_LENGTH;
        // Grab the CRC from the encode buffer and invert it.
        const uint32_t crc = *reinterpret_cast<uint32_t*>(pucTempEncodeBuffer) ^ 0xFFFFFFFF;
        if (!CopyToBuffer(&pucTempEncodeBuffer, uiMyBufferBytesRemaining, crc)) { return STATUS::BUFFER_FULL; }
        break;
    }
    case ENCODE_FORMAT::JSON:
        if (!CopyToBuffer(&pucTempEncodeBuffer, uiMyBufferBytesRemaining, '}')) { return STATUS::BUFFER_FULL; }
        break;

    default: break;
    }

    stEmbeddedMessageData_.uiMessageLength = static_cast<uint32_t>(pucTempEncodeBuffer - stRxConfigMessageData_.pucMessageBody);

    // Put the final CRC at the end.
    switch (eEncodeFormat_)
    {
    case ENCODE_FORMAT::ASCII: {
        const uint32_t crc = CalculateBlockCrc32(pcMyEncodeBuffer.get() + 1, static_cast<uint32_t>(pucTempEncodeBuffer - pcMyEncodeBuffer.get() - 1));
        if (!CopyAllToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiMyBufferBytesRemaining, '*', HexValue<uint32_t>{crc, 8}, "\r\n"))
        {
            return STATUS::BUFFER_FULL;
        }
        break;
    }
    case ENCODE_FORMAT::BINARY: {
        const uint32_t crc = CalculateBlockCrc32(pcMyEncodeBuffer.get(), static_cast<uint32_t>(pucTempEncodeBuffer - pcMyEncodeBuffer.get()));
        if (!CopyToBuffer(&pucTempEncodeBuffer, uiMyBufferBytesRemaining, crc)) { return STATUS::BUFFER_FULL; }
        break;
    }
    default: break;
    }

    stRxConfigMessageData_.uiMessageBodyLength = static_cast<uint32_t>(pucTempEncodeBuffer - stRxConfigMessageData_.pucMessageBody);

    // Add the closing '}' character, but don't count it as part of the message body length.
    if (eEncodeFormat_ == ENCODE_FORMAT::JSON && !CopyToBuffer(&pucTempEncodeBuffer, uiMyBufferBytesRemaining, '}')) { return STATUS::BUFFER_FULL; }

    stRxConfigMessageData_.pucMessage = pcMyEncodeBuffer.get();
    stRxConfigMessageData_.uiMessageLength = static_cast<uint32_t>(pucTempEncodeBuffer - pcMyEncodeBuffer.get());

    return STATUS::SUCCESS;
}
