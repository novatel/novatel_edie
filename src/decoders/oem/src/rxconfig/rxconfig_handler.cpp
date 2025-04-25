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

// Define Static Framer
std::unique_ptr<Framer> RxConfigHandler::pclMyFramer = nullptr;

// -------------------------------------------------------------------------------------------------------
RxConfigHandler::RxConfigHandler(const MessageDatabase::Ptr& pclMessageDb_)
    : clMyHeaderDecoder(pclMessageDb_), clMyMessageDecoder(pclMessageDb_), clMyEncoder(pclMessageDb_),
      pcMyFrameBuffer(std::make_unique<unsigned char[]>(uiInternalBufferSize)),
      pcMyEncodeBuffer(std::make_unique<unsigned char[]>(uiInternalBufferSize))
{
    pclMyLogger = GetBaseLoggerManager()->RegisterLogger("rxconfig_handler");

    pclMyLogger->debug("RxConfigHandler initializing...");

    auto pclMyCircularBuffer = std::make_shared<CircularBuffer>();
    pclMyFramer = std::make_unique<Framer>(pclMyCircularBuffer);

    if (pclMessageDb_ != nullptr) { LoadJsonDb(pclMessageDb_); }

    pclMyLogger->debug("RxConfigHandler initialized");
}

void RxConfigHandler::ValidateMsgDef(const MessageDefinition::ConstPtr& pclMsgDef_)
{
    std::vector<BaseField::Ptr> vFieldDefs = pclMsgDef_->fields.at(pclMsgDef_->latestMessageCrc);
    std::string sMsgName = pclMsgDef_->name;
    if (vFieldDefs.size() != 1) { throw std::invalid_argument(sMsgName + " definition has too many fields."); }
    BaseField::ConstPtr pclFieldDef = vFieldDefs[0];
    if (pclFieldDef->conversion != "%R") { throw std::invalid_argument(sMsgName + " definition has non \"%R\" conversion string."); }
    if (pclFieldDef->type != FIELD_TYPE::VARIABLE_LENGTH_ARRAY)
    {
        throw std::invalid_argument(sMsgName + " definition must have a VARIABLE_LENGTH_ARRAY field type.");
    }
    if (pclFieldDef->dataType.name != DATA_TYPE::UCHAR) { throw std::invalid_argument(sMsgName + " definition must have a UCHAR data type."); }
}

// -------------------------------------------------------------------------------------------------------
void RxConfigHandler::LoadJsonDb(const MessageDatabase::Ptr& pclMessageDb_)
{
    pclMyMsgDb = pclMessageDb_;
    clMyHeaderDecoder.LoadJsonDb(pclMessageDb_);
    clMyMessageDecoder.LoadJsonDb(pclMessageDb_);
    clMyEncoder.LoadJsonDb(pclMessageDb_);

    MessageDefinition::ConstPtr pclRxConfigMessageDefinition = pclMyMsgDb->GetMsgDef(US_RX_CONFIG_MSG_ID);
    MessageDefinition::ConstPtr pclRxConfigUserMessageDefinition = pclMyMsgDb->GetMsgDef(US_RX_CONFIG_USER_MSG_ID);

    if (pclRxConfigMessageDefinition != nullptr)
    {
        ValidateMsgDef(pclRxConfigMessageDefinition);
        pclRxConfigFieldDef = GetFieldDefFromMsgDef(pclRxConfigMessageDefinition);
    }

    if (pclRxConfigUserMessageDefinition != nullptr)
    {
        ValidateMsgDef(pclRxConfigUserMessageDefinition);
        pclRxConfigUserFieldDef = GetFieldDefFromMsgDef(pclRxConfigUserMessageDefinition);
    }

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
size_t RxConfigHandler::Write(const unsigned char* pucData_, uint32_t uiDataSize_) { return clMyFramer.Write(pucData_, uiDataSize_); }

// -------------------------------------------------------------------------------------------------------
BaseField::ConstPtr RxConfigHandler::GetFieldDefFromMsgDef(const MessageDefinition::ConstPtr& pclMsgDef_)
{
    return pclMsgDef_->fields.at(pclMsgDef_->latestMessageCrc).at(0);
}

// -------------------------------------------------------------------------------------------------------
// Converts shared pointer into an r-value and increments reference count without invalidating original.
template <typename T> std::shared_ptr<T> CopyAndMove(std::shared_ptr<T> pclPtr_) { return pclPtr_; }

// -------------------------------------------------------------------------------------------------------
STATUS RxConfigHandler::Decode(const unsigned char* pucMessage_, std::vector<FieldContainer>& stInterMessage_,
                               MetaDataStruct& stRxConfigMetaData_) const
{
    BaseField::ConstPtr pclFieldDef;
    if (stRxConfigMetaData_.usMessageId == US_RX_CONFIG_MSG_ID)
    {
        if (!pclRxConfigFieldDef) { return STATUS::NO_DEFINITION; }
        pclFieldDef = pclRxConfigFieldDef;
    }
    else if (stRxConfigMetaData_.usMessageId == US_RX_CONFIG_USER_MSG_ID)
    {
        if (!pclRxConfigUserFieldDef) { return STATUS::NO_DEFINITION; }
        pclFieldDef = pclRxConfigUserFieldDef;
    }
    else { return STATUS::UNSUPPORTED; }

    const unsigned char* pucTempMessagePointer = pucMessage_;
    MetaDataStruct stEmbeddedMetaData_;
    std::vector<FieldContainer> stEmbeddedMessage;
    uint32_t uiTotalPayloadSize = stRxConfigMetaData_.uiLength - stRxConfigMetaData_.uiHeaderLength;

    stInterMessage_.emplace_back(std::vector<FieldContainer>(), CopyAndMove(pclFieldDef));
    auto& stEmbeddedMessageData = std::get<std::vector<FieldContainer>>(stInterMessage_.back().fieldValue);

    // Determine how many bytes to copy from raw message data to the embedded message data.
    uint32_t uiCopyableEmbeddedMsgBytes;
    switch (stRxConfigMetaData_.eFormat)
    {
    case HEADER_FORMAT::ABB_ASCII: {
        // Fix embedded header indentation
        ConsumeAbbrevFormatting(reinterpret_cast<const char**>(&pucTempMessagePointer));
        stEmbeddedMessageData.emplace_back(static_cast<uint8_t>(OEM4_ABBREV_ASCII_SYNC), CopyAndMove(pclFieldDef));
        uiCopyableEmbeddedMsgBytes = uiTotalPayloadSize - (pucTempMessagePointer - pucMessage_);
        break;
    }
    case HEADER_FORMAT::ASCII: {
        // Ignore {CRC}*{CRC}\r\n at end of buffer
        uiCopyableEmbeddedMsgBytes = uiTotalPayloadSize - OEM4_ASCII_CRC_LENGTH - 1 - OEM4_ASCII_CRC_LENGTH - 2;
        break;
    }
    case HEADER_FORMAT::BINARY: {
        // Ignore CRCs at end of buffer
        uiCopyableEmbeddedMsgBytes = uiTotalPayloadSize - 2 * OEM4_BINARY_CRC_LENGTH;
        break;
    }
    default: {
        pclMyLogger->error("Unsupported RXCONFIG decoding format: {}", static_cast<uint8_t>(stRxConfigMetaData_.eFormat));
        return STATUS::UNSUPPORTED;
    }
    }

    // Copy embedded message data into raw representation
    stEmbeddedMessageData.reserve(uiCopyableEmbeddedMsgBytes);
    for (uint32_t i = 0; i < uiCopyableEmbeddedMsgBytes; ++i)
    {
        stEmbeddedMessageData.emplace_back(*(reinterpret_cast<const uint8_t*>(pucTempMessagePointer)), CopyAndMove(pclFieldDef));
        pucTempMessagePointer++;
    }

    // Flip CRC to make the raw reprentation decodable
    switch (stRxConfigMetaData_.eFormat)
    {
    case HEADER_FORMAT::ASCII: {
        // invert ascii crc
        const uint32_t uiCRC = strtoul(reinterpret_cast<const char*>(pucTempMessagePointer), nullptr, 16) ^ 0xFFFFFFFF;
        char pucCRC[OEM4_ASCII_CRC_LENGTH];
        std::to_chars(pucCRC, pucCRC + OEM4_ASCII_CRC_LENGTH, uiCRC, 16);
        for (uint32_t i = 0; i < OEM4_ASCII_CRC_LENGTH; ++i)
        {
            stEmbeddedMessageData.emplace_back(*(reinterpret_cast<uint8_t*>(pucCRC) + i), CopyAndMove(pclFieldDef));
        }
        stEmbeddedMessageData.emplace_back(static_cast<uint8_t>('\r'), CopyAndMove(pclFieldDef));
        stEmbeddedMessageData.emplace_back(static_cast<uint8_t>('\n'), CopyAndMove(pclFieldDef));
        break;
    }
    case HEADER_FORMAT::BINARY: {
        uint32_t uiCRC = *reinterpret_cast<const uint32_t*>(pucTempMessagePointer) ^ 0xFFFFFFFF;
        stEmbeddedMessageData.emplace_back(static_cast<uint8_t>(uiCRC >> 24), CopyAndMove(pclFieldDef));
        stEmbeddedMessageData.emplace_back(static_cast<uint8_t>(uiCRC >> 16), CopyAndMove(pclFieldDef));
        stEmbeddedMessageData.emplace_back(static_cast<uint8_t>(uiCRC >> 8), CopyAndMove(pclFieldDef));
        stEmbeddedMessageData.emplace_back(static_cast<uint8_t>(uiCRC), CopyAndMove(pclFieldDef));
        break;
    }
    default: break;
    }

    return STATUS::SUCCESS;
}

STATUS RxConfigHandler::Encode(unsigned char** ppucBuffer_, uint32_t uiBufferSize_, const IntermediateHeader& stHeader_,
                               const std::vector<FieldContainer>& stMessage_, MessageDataStruct& stMessageData_, ENCODE_FORMAT eFormat_) const
{
    MessageDataStruct stEmbeddedMessageData;
    MetaDataStruct stEmbeddedMetaData;
    return Encode(ppucBuffer_, uiBufferSize_, stHeader_, stMessage_, stMessageData_, stEmbeddedMessageData, stEmbeddedMetaData, eFormat_);
}

STATUS RxConfigHandler::EncodeJSON(unsigned char** ppucBuffer_, uint32_t uiBufferSize_, const IntermediateHeader& stHeader_,
                                   MessageDataStruct& stMessageData_, MessageDataStruct& stEmbeddedMessageData_, MetaDataStruct& stEmbeddedMetaData_,
                                   IntermediateHeader& stEmbeddedHeader_, std::vector<FieldContainer>& stEmbeddedMessage_) const
{
    STATUS eStatus;
    unsigned char* pucTempEncodeBuffer = *ppucBuffer_;
    stMessageData_.pucMessage = *ppucBuffer_;

    // -- Encode RXConfig Header --
    // Abuse the fact that header format is only used for determining whether the header is long or short
    if (!CopyToBuffer(&pucTempEncodeBuffer, uiBufferSize_, R"({"header": )")) { return STATUS::BUFFER_FULL; }

    stMessageData_.pucMessageHeader = *ppucBuffer_;
    eStatus = clMyEncoder.EncodeHeader(&pucTempEncodeBuffer, uiBufferSize_, stHeader_, stMessageData_, HEADER_FORMAT::BINARY, ENCODE_FORMAT::JSON);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }
    pucTempEncodeBuffer += stMessageData_.uiMessageHeaderLength;

    // -- Encode RXConfig Body --
    if (!CopyToBuffer(&pucTempEncodeBuffer, uiBufferSize_, R"(,"body": )")) { return STATUS::BUFFER_FULL; }
    stMessageData_.pucMessageBody = pucTempEncodeBuffer;
    stEmbeddedMessageData_.pucMessage = pucTempEncodeBuffer;

    eStatus = clMyEncoder.Encode(&pucTempEncodeBuffer, uiBufferSize_, stEmbeddedHeader_, stEmbeddedMessage_, stEmbeddedMessageData_,
                                 stEmbeddedMetaData_.eFormat, ENCODE_FORMAT::JSON);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }
    pucTempEncodeBuffer += stEmbeddedMessageData_.uiMessageLength;
    stMessageData_.uiMessageBodyLength = pucTempEncodeBuffer - stMessageData_.pucMessageBody;

    if (!CopyToBuffer(&pucTempEncodeBuffer, uiBufferSize_, '}')) { return STATUS::BUFFER_FULL; }

    stMessageData_.uiMessageLength = pucTempEncodeBuffer - *ppucBuffer_;

    return STATUS::SUCCESS;
}

STATUS RxConfigHandler::EncodeAbbrevAscii(unsigned char** ppucBuffer_, uint32_t uiBufferSize_, const IntermediateHeader& stHeader_,
                                          MessageDataStruct& stMessageData_, MessageDataStruct& stEmbeddedMessageData_,
                                          MetaDataStruct& stEmbeddedMetaData_, IntermediateHeader& stEmbeddedHeader_,
                                          std::vector<FieldContainer>& stEmbeddedMessage_) const
{
    STATUS eStatus;
    unsigned char* pucTempEncodeBuffer = *ppucBuffer_;
    stMessageData_.pucMessage = *ppucBuffer_;

    // -- Encode RXConfig Header --
    // Abuse the fact that header format is only used for determining whether the header is long or short
    stMessageData_.pucMessageHeader = *ppucBuffer_;
    eStatus =
        clMyEncoder.EncodeHeader(&pucTempEncodeBuffer, uiBufferSize_, stHeader_, stMessageData_, HEADER_FORMAT::BINARY, ENCODE_FORMAT::ABBREV_ASCII);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }
    pucTempEncodeBuffer += stMessageData_.uiMessageHeaderLength;

    // -- Encode Embedded Header --
    stEmbeddedMessageData_.pucMessage = pucTempEncodeBuffer;
    stMessageData_.pucMessageBody = pucTempEncodeBuffer;

    if (!CopyToBuffer(&pucTempEncodeBuffer, uiBufferSize_, szAbbrevAsciiEmbeddedHeaderPrefix)) { return STATUS::BUFFER_FULL; };
    // Backtrack one byte to give room for new sync byte
    pucTempEncodeBuffer--;
    uiBufferSize_++;

    eStatus = clMyEncoder.EncodeHeader(&pucTempEncodeBuffer, uiBufferSize_, stEmbeddedHeader_, stEmbeddedMessageData_, stEmbeddedMetaData_.eFormat,
                                       ENCODE_FORMAT::ABBREV_ASCII);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }
    pucTempEncodeBuffer += stEmbeddedMessageData_.uiMessageHeaderLength;

    // remove sync byte at start of embedded message header
    stEmbeddedMessageData_.pucMessageHeader[0] = ' ';
    // go back over '\r\n'
    pucTempEncodeBuffer -= 2;
    uiBufferSize_ += 2;
    // add in a space
    if (!CopyToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiBufferSize_, " \r\n")) { return STATUS::BUFFER_FULL; }
    // adjust header pointer for prefix
    stEmbeddedMessageData_.pucMessageHeader -= (szAbbrevAsciiEmbeddedHeaderPrefix.length() - 1);
    // adjust header length for prefix and additoinal ending space
    stEmbeddedMessageData_.uiMessageHeaderLength += static_cast<uint32_t>((szAbbrevAsciiEmbeddedHeaderPrefix.length()));

    // -- Encode Embedded Body --
    eStatus = clMyEncoder.EncodeBody(&pucTempEncodeBuffer, uiBufferSize_, stEmbeddedMessage_, stEmbeddedMessageData_, stEmbeddedMetaData_.eFormat,
                                     ENCODE_FORMAT::ABBREV_ASCII);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }
    pucTempEncodeBuffer += stEmbeddedMessageData_.uiMessageBodyLength;

    // -- Resolve Message Lengths --
    stMessageData_.uiMessageBodyLength = pucTempEncodeBuffer - stMessageData_.pucMessageBody;
    stMessageData_.uiMessageLength = pucTempEncodeBuffer - *ppucBuffer_;
    stEmbeddedMessageData_.uiMessageLength = stEmbeddedMessageData_.uiMessageBodyLength + stEmbeddedMessageData_.uiMessageHeaderLength;

    return STATUS::SUCCESS;
}

STATUS RxConfigHandler::EncodeAscii(unsigned char** ppucBuffer_, uint32_t uiBufferSize_, const IntermediateHeader& stHeader_,
                                    MessageDataStruct& stMessageData_, MessageDataStruct& stEmbeddedMessageData_, MetaDataStruct& stEmbeddedMetaData_,
                                    IntermediateHeader& stEmbeddedHeader_, std::vector<FieldContainer>& stEmbeddedMessage_) const
{
    STATUS eStatus;
    unsigned char* pucTempEncodeBuffer = *ppucBuffer_;
    stMessageData_.pucMessage = *ppucBuffer_;

    // -- Encode RXConfig Header --
    // Abuse the fact that header format is only used for determining whether the header is long or short
    stMessageData_.pucMessageHeader = *ppucBuffer_;
    eStatus =
        clMyEncoder.EncodeHeader(&pucTempEncodeBuffer, uiBufferSize_, stHeader_, stMessageData_, stEmbeddedMetaData_.eFormat, ENCODE_FORMAT::ASCII);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }
    pucTempEncodeBuffer += stMessageData_.uiMessageHeaderLength;

    // -- Encode Embedded Message --
    stMessageData_.pucMessageBody = pucTempEncodeBuffer;
    eStatus = clMyEncoder.Encode(&pucTempEncodeBuffer, uiBufferSize_, stEmbeddedHeader_, stEmbeddedMessage_, stEmbeddedMessageData_,
                                 stEmbeddedMetaData_.eFormat, ENCODE_FORMAT::ASCII);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }
    pucTempEncodeBuffer += stEmbeddedMessageData_.uiMessageLength;

    // -- Write CRC --

    // Backtrack over '{CRC}' and '\r\n'
    uint32_t uiBacktrackAmount = OEM4_ASCII_CRC_LENGTH + 2;
    pucTempEncodeBuffer -= uiBacktrackAmount;
    uiBufferSize_ += uiBacktrackAmount;
    // Ignore '#' and '*' in CRC calculation
    uint32_t uiCrc =
        CalculateBlockCrc32(stEmbeddedMessageData_.pucMessageHeader + 1, (pucTempEncodeBuffer - stEmbeddedMessageData_.pucMessageHeader) - 2) ^
        0xFFFFFFFF;
    // Write embedded CRC
    if (!CopyAllToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiBufferSize_, HexValue<uint32_t>{uiCrc, 8})) { return STATUS::BUFFER_FULL; }
    // Fix embedded message length
    stEmbeddedMessageData_.uiMessageBodyLength = pucTempEncodeBuffer - stEmbeddedMessageData_.pucMessageBody;

    // Encode regular CRC
    unsigned char* pucStartPoint = *ppucBuffer_ + 1; // Skip the first byte, which is the sync byte
    uiCrc = CalculateBlockCrc32(pucStartPoint, pucTempEncodeBuffer - pucStartPoint);
    if (!CopyAllToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiBufferSize_, '*', HexValue<uint32_t>{uiCrc, 8}, "\r\n"))
    {
        return STATUS::BUFFER_FULL;
    }

    // -- Resolve Message Lengths --
    stMessageData_.uiMessageBodyLength = pucTempEncodeBuffer - stMessageData_.pucMessageBody;
    stMessageData_.uiMessageLength = pucTempEncodeBuffer - *ppucBuffer_;
    stEmbeddedMessageData_.uiMessageLength = stEmbeddedMessageData_.uiMessageBodyLength + stEmbeddedMessageData_.uiMessageHeaderLength;

    return STATUS::SUCCESS;
}

STATUS RxConfigHandler::EncodeBinary(unsigned char** ppucBuffer_, uint32_t uiBufferSize_, const IntermediateHeader& stHeader_,
                                     MessageDataStruct& stMessageData_, MessageDataStruct& stEmbeddedMessageData_,
                                     MetaDataStruct& stEmbeddedMetaData_, IntermediateHeader& stEmbeddedHeader_,
                                     std::vector<FieldContainer>& stEmbeddedMessage_) const
{
    STATUS eStatus;
    unsigned char* pucTempEncodeBuffer = *ppucBuffer_;
    stMessageData_.pucMessage = *ppucBuffer_;

    // -- Encode RXConfig Header --
    stMessageData_.pucMessageHeader = *ppucBuffer_;
    eStatus =
        clMyEncoder.EncodeHeader(&pucTempEncodeBuffer, uiBufferSize_, stHeader_, stMessageData_, stEmbeddedMetaData_.eFormat, ENCODE_FORMAT::BINARY);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }
    pucTempEncodeBuffer += stMessageData_.uiMessageHeaderLength;

    // -- Encode Embedded Message --
    stMessageData_.pucMessageBody = pucTempEncodeBuffer;
    eStatus = clMyEncoder.Encode(&pucTempEncodeBuffer, uiBufferSize_, stEmbeddedHeader_, stEmbeddedMessage_, stEmbeddedMessageData_,
                                 stEmbeddedMetaData_.eFormat, ENCODE_FORMAT::BINARY);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }
    pucTempEncodeBuffer += stEmbeddedMessageData_.uiMessageLength;

    // -- Write CRC --
    // Backtrack over CRC
    pucTempEncodeBuffer -= OEM4_BINARY_CRC_LENGTH;
    uiBufferSize_ += OEM4_BINARY_CRC_LENGTH;
    // Fill in Embedded Binary Header length
    uint32_t uiEmbeddedBodyLength = pucTempEncodeBuffer - stEmbeddedMessageData_.pucMessageBody;
    reinterpret_cast<Oem4BinaryHeader*>(stEmbeddedMessageData_.pucMessageHeader)->usLength = static_cast<uint16_t>(uiEmbeddedBodyLength);
    // Fill in Regular Binary Header length
    uint32_t uiRxLength = pucTempEncodeBuffer - stMessageData_.pucMessageBody + OEM4_BINARY_CRC_LENGTH;
    reinterpret_cast<Oem4BinaryHeader*>(stMessageData_.pucMessageHeader)->usLength = static_cast<uint16_t>(uiRxLength);
    // Overwrite Embedded CRC
    uint32_t uiCrc =
        CalculateBlockCrc32(stEmbeddedMessageData_.pucMessageHeader, pucTempEncodeBuffer - stEmbeddedMessageData_.pucMessageHeader) ^ 0xFFFFFFFF;
    if (!CopyToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiBufferSize_, uiCrc)) { return STATUS::BUFFER_FULL; }
    // Add Regular CRC
    uiCrc = CalculateBlockCrc32(stMessageData_.pucMessageHeader, pucTempEncodeBuffer - stMessageData_.pucMessageHeader);
    if (!CopyToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiBufferSize_, uiCrc)) { return STATUS::BUFFER_FULL; }

    // -- Resolve Message Lengths --
    stMessageData_.uiMessageBodyLength = pucTempEncodeBuffer - stMessageData_.pucMessageBody;
    stMessageData_.uiMessageLength = pucTempEncodeBuffer - *ppucBuffer_;
    stEmbeddedMessageData_.uiMessageLength = stEmbeddedMessageData_.uiMessageBodyLength + stEmbeddedMessageData_.uiMessageHeaderLength;

    return STATUS::SUCCESS;
}

STATUS RxConfigHandler::Encode(unsigned char** ppucBuffer_, uint32_t uiBufferSize_, const IntermediateHeader& stHeader_,
                               const std::vector<FieldContainer>& stMessage_, MessageDataStruct& stMessageData_,
                               MessageDataStruct& stEmbeddedMessageData_, MetaDataStruct& stEmbeddedMetaData_, ENCODE_FORMAT eFormat_) const
{
    if (ppucBuffer_ == nullptr || *ppucBuffer_ == nullptr) { return STATUS::NULL_PROVIDED; }

    STATUS eStatus;

    // -- Decode Embedded Message Data --
    IntermediateHeader stEmbeddedHeader;
    std::vector<FieldContainer> stEmbeddedMessage;

    // Convert embedded data to a dynamically allocated character array
    const auto& vEmbeddedData = std::get<std::vector<FieldContainer>>(stMessage_[0].fieldValue);
    std::unique_ptr<unsigned char[]> pucEmbeddedDataBuffer = std::make_unique<unsigned char[]>(vEmbeddedData.size());
    unsigned char* pucEmbeddedDataPointer = pucEmbeddedDataBuffer.get();
    for (const auto& stField : vEmbeddedData) { *pucEmbeddedDataPointer++ = std::get<uint8_t>(stField.fieldValue); }

    eStatus = clMyHeaderDecoder.Decode(pucEmbeddedDataBuffer.get(), stEmbeddedHeader, stEmbeddedMetaData_);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    eStatus = clMyMessageDecoder.Decode(pucEmbeddedDataBuffer.get() + stEmbeddedMetaData_.uiHeaderLength, stEmbeddedMessage, stEmbeddedMetaData_);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    // -- Encode --
    switch (eFormat_)
    {
    case ENCODE_FORMAT::BINARY: {
        return EncodeBinary(ppucBuffer_, uiBufferSize_, stHeader_, stMessageData_, stEmbeddedMessageData_, stEmbeddedMetaData_, stEmbeddedHeader,
                            stEmbeddedMessage);
    }
    case ENCODE_FORMAT::ASCII: {
        return EncodeAscii(ppucBuffer_, uiBufferSize_, stHeader_, stMessageData_, stEmbeddedMessageData_, stEmbeddedMetaData_, stEmbeddedHeader,
                           stEmbeddedMessage);
    }

    case ENCODE_FORMAT::ABBREV_ASCII: {
        return EncodeAbbrevAscii(ppucBuffer_, uiBufferSize_, stHeader_, stMessageData_, stEmbeddedMessageData_, stEmbeddedMetaData_, stEmbeddedHeader,
                                 stEmbeddedMessage);
    }
    case ENCODE_FORMAT::JSON: {
        return EncodeJSON(ppucBuffer_, uiBufferSize_, stHeader_, stMessageData_, stEmbeddedMessageData_, stEmbeddedMetaData_, stEmbeddedHeader,
                          stEmbeddedMessage);
    }
    default: return STATUS::UNSUPPORTED;
    }
}

// -------------------------------------------------------------------------------------------------------
STATUS
RxConfigHandler::Convert(MessageDataStruct& stRxConfigMessageData_, MetaDataStruct& stRxConfigMetaData_, MessageDataStruct& stEmbeddedMessageData_,
                         MetaDataStruct& stEmbeddedMetaData_, ENCODE_FORMAT eEncodeFormat_)
{
    IntermediateHeader stRxConfigHeader;
    std::vector<FieldContainer> stEmbeddedMessage;

    unsigned char* pucTempMessagePointer = pcMyFrameBuffer.get();
    unsigned char* pucTempEncodeBuffer = pcMyEncodeBuffer.get();

    // Get an RXCONFIG log.
    STATUS eStatus = pclMyFramer->GetFrame(pcMyFrameBuffer.get(), uiInternalBufferSize, stRxConfigMetaData_);
    if (eStatus == STATUS::BUFFER_EMPTY || eStatus == STATUS::INCOMPLETE) { return STATUS::BUFFER_EMPTY; }
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    // Decode the RXCONFIG log.
    eStatus = clMyHeaderDecoder.Decode(pcMyFrameBuffer.get(), stRxConfigHeader, stRxConfigMetaData_);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }
    pucTempMessagePointer += stRxConfigMetaData_.uiHeaderLength;

    // If we have something that isn't RXCONFIG, get rid of it.
    if (!IsRxConfigTypeMsg(stRxConfigMetaData_.usMessageId)) { return STATUS::UNKNOWN; }

    eStatus = Decode(pucTempMessagePointer, stEmbeddedMessage, stRxConfigMetaData_);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    eStatus = Encode(&pucTempEncodeBuffer, uiInternalBufferSize, stRxConfigHeader, stEmbeddedMessage, stRxConfigMessageData_, stEmbeddedMessageData_,
                     stEmbeddedMetaData_, eEncodeFormat_);

    return eStatus;
}
