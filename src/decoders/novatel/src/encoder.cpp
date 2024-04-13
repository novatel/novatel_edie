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
// ! \file encoder.cpp
// ===============================================================================

#include "decoders/novatel/api/encoder.hpp"

#include <bitset>

using namespace novatel::edie;
using namespace novatel::edie::oem;

// -------------------------------------------------------------------------------------------------------
Encoder::Encoder(JsonReader* pclJsonDb_) : EncoderBase(pclJsonDb_)
{
    InitFieldMaps();
    if (pclJsonDb_ != nullptr) { LoadJsonDb(pclJsonDb_); }
}

// -------------------------------------------------------------------------------------------------------
void Encoder::InitEnumDefns()
{
    vMyCommandDefns = pclMyMsgDb->GetEnumDefName("Commands");
    vMyPortAddrDefns = pclMyMsgDb->GetEnumDefName("PortAddress");
    vMyGPSTimeStatusDefns = pclMyMsgDb->GetEnumDefName("GPSTimeStatus");
}

// -------------------------------------------------------------------------------------------------------
void Encoder::InitFieldMaps()
{
    // =========================================================
    // ASCII Field Mapping
    // =========================================================
    asciiFieldMap[CalculateBlockCRC32("%s")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb) {
        return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%c",
                             fc_.field_def->dataType.name == DATA_TYPE::UCHAR ? std::get<uint8_t>(fc_.field_value)
                                                                              : std::get<int8_t>(fc_.field_value));
    };

    asciiFieldMap[CalculateBlockCRC32("%m")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb) {
        return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%s", pclMsgDb->MsgIdToMsgName(std::get<uint32_t>(fc_.field_value)).c_str());
    };

    asciiFieldMap[CalculateBlockCRC32("%T")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb) {
        return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%.3lf", std::get<uint32_t>(fc_.field_value) / 1000.0);
    };

    asciiFieldMap[CalculateBlockCRC32("%id")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                   [[maybe_unused]] JsonReader* pclMsgDb) {
        const uint32_t uiTempID = std::get<uint32_t>(fc_.field_value);
        const uint16_t usSV = uiTempID & 0x0000FFFF;
        const int16_t sGloChan = (uiTempID & 0xFFFF0000) >> 16;
        // short circuit eval when sGloChan == 0, otherwise print to buffer
        return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%u", usSV) && (sGloChan == 0 || PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%+d", sGloChan));
    };

    asciiFieldMap[CalculateBlockCRC32("%P")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb) {
        const uint8_t uiValue = std::get<uint8_t>(fc_.field_value);
        return uiValue == '\\'                 ? PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "\\\\")              // TODO: add description
               : uiValue > 31 && uiValue < 127 ? PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%c", uiValue)       // print the character
                                               : PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "\\x%02x", uiValue); // print as a hex character within ()
    };

    asciiFieldMap[CalculateBlockCRC32("%k")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb) {
        return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, MakeConversionString<float>(fc_).data(), std::get<float>(fc_.field_value));
    };

    asciiFieldMap[CalculateBlockCRC32("%lk")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                   [[maybe_unused]] JsonReader* pclMsgDb) {
        return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, MakeConversionString<double>(fc_).data(), std::get<double>(fc_.field_value));
    };

    asciiFieldMap[CalculateBlockCRC32("%c")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb) {
        return fc_.field_def->dataType.length == 1 ? PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%c", std::get<uint8_t>(fc_.field_value))
               : (fc_.field_def->dataType.length == 4 && fc_.field_def->dataType.name == DATA_TYPE::ULONG)
                   ? PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%c", std::get<uint32_t>(fc_.field_value))
                   : false;
    };

    // =========================================================
    // Json Field Mapping
    // =========================================================
    jsonFieldMap[CalculateBlockCRC32("%P")] = BasicMapEntry<uint8_t>("%hhu");

    jsonFieldMap[CalculateBlockCRC32("%T")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 [[maybe_unused]] JsonReader* pclMsgDb) {
        return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%.3lf", std::get<uint32_t>(fc_.field_value) / 1000.0);
    };

    jsonFieldMap[CalculateBlockCRC32("%m")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 [[maybe_unused]] JsonReader* pclMsgDb) {
        return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("%s")", pclMsgDb->MsgIdToMsgName(std::get<uint32_t>(fc_.field_value)).c_str());
    };

    jsonFieldMap[CalculateBlockCRC32("%id")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb) {
        const auto uiTempID = std::get<uint32_t>(fc_.field_value);
        const uint16_t usSV = uiTempID & 0x0000FFFF;
        const int16_t sGloChan = (uiTempID & 0xFFFF0000) >> 16;
        return sGloChan < 0 ? PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("%u%d")", usSV, sGloChan)
               : sGloChan   ? PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("%u+%d")", usSV, sGloChan)
                            : PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("%u")", usSV);
    };

    jsonFieldMap[CalculateBlockCRC32("%k")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 [[maybe_unused]] JsonReader* pclMsgDb) {
        return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, MakeConversionString<float>(fc_).data(), std::get<float>(fc_.field_value));
    };

    jsonFieldMap[CalculateBlockCRC32("%lk")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb) {
        return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, MakeConversionString<double>(fc_).data(), std::get<double>(fc_.field_value));
    };

    jsonFieldMap[CalculateBlockCRC32("%s")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 [[maybe_unused]] JsonReader* pclMsgDb) {
        return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%c",
                             (fc_.field_def->dataType.name == DATA_TYPE::UCHAR) ? std::get<uint8_t>(fc_.field_value)
                                                                                : std::get<int8_t>(fc_.field_value));
    };

    jsonFieldMap[CalculateBlockCRC32("%c")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 [[maybe_unused]] JsonReader* pclMsgDb) {
        return (fc_.field_def->dataType.length == 1 && PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "\"%c\"", std::get<uint8_t>(fc_.field_value))) ||
               (fc_.field_def->dataType.length == 4 && fc_.field_def->dataType.name == DATA_TYPE::ULONG &&
                PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "\"%c\"", std::get<uint32_t>(fc_.field_value)));
    };
}

// -------------------------------------------------------------------------------------------------------
bool Encoder::EncodeBinaryHeader(const IntermediateHeader& stInterHeader_, unsigned char** ppcOutBuf_, uint32_t& uiBytesLeft_)
{
    OEM4BinaryHeader stBinaryHeader(stInterHeader_);
    return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &stBinaryHeader);
}

// -------------------------------------------------------------------------------------------------------
bool Encoder::EncodeBinaryShortHeader(const IntermediateHeader& stInterHeader_, unsigned char** ppcOutBuf_, uint32_t& uiBytesLeft_)
{
    OEM4BinaryShortHeader stBinaryHeader(stInterHeader_);
    return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &stBinaryHeader);
}

// -------------------------------------------------------------------------------------------------------
bool Encoder::FieldToBinary(const FieldContainer& fc_, unsigned char** ppcOutBuf_, uint32_t& uiBytesLeft_)
{
    switch (fc_.field_def->dataType.name)
    {
    case DATA_TYPE::BOOL: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, reinterpret_cast<const int32_t*>(&std::get<bool>(fc_.field_value)));
    case DATA_TYPE::HEXBYTE: [[fallthrough]];
    case DATA_TYPE::UCHAR: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<uint8_t>(fc_.field_value));
    case DATA_TYPE::CHAR: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<int8_t>(fc_.field_value));
    case DATA_TYPE::USHORT: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<uint16_t>(fc_.field_value));
    case DATA_TYPE::SHORT: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<int16_t>(fc_.field_value));
    case DATA_TYPE::UINT: [[fallthrough]];
    case DATA_TYPE::SATELLITEID: [[fallthrough]];
    case DATA_TYPE::ULONG: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<uint32_t>(fc_.field_value));
    case DATA_TYPE::INT: [[fallthrough]];
    case DATA_TYPE::LONG: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<int32_t>(fc_.field_value));
    case DATA_TYPE::ULONGLONG: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<uint64_t>(fc_.field_value));
    case DATA_TYPE::LONGLONG: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<int64_t>(fc_.field_value));
    case DATA_TYPE::FLOAT: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<float>(fc_.field_value));
    case DATA_TYPE::DOUBLE: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<double>(fc_.field_value));
    default: SPDLOG_LOGGER_CRITICAL(pclMyLogger, "FieldToBinary(): unknown type."); throw std::runtime_error("FieldToBinary(): unknown type.");
    }
}

// -------------------------------------------------------------------------------------------------------
bool Encoder::EncodeAsciiHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_)
{
    // Sync byte
    if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%c", OEM4_ASCII_SYNC)) { return false; }

    // Message name
    const MessageDefinition* pclMessageDef = pclMyMsgDb->GetMsgDef(stInterHeader_.usMessageID);
    std::string sMsgName(pclMessageDef ? pclMessageDef->name : GetEnumString(vMyCommandDefns, stInterHeader_.usMessageID));
    const uint32_t uiSiblingID = stInterHeader_.ucMessageType & static_cast<uint32_t>(MESSAGETYPEMASK::MEASSRC);
    const uint32_t uiResponse = (stInterHeader_.ucMessageType & static_cast<uint32_t>(MESSAGETYPEMASK::RESPONSE)) >> 7;
    sMsgName.append(uiResponse ? "R" : "A"); // Append 'A' for ascii, or 'R' for ascii response
    if (uiSiblingID)                         // Append sibling i.e. the _1 of RANGEA_1
        sMsgName.append("_").append(std::to_string(uiSiblingID));

    return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%s%c", sMsgName.c_str(), OEM4_ASCII_FIELD_SEPARATOR) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%s%c", GetEnumString(vMyPortAddrDefns, stInterHeader_.uiPortAddress).c_str(),
                         OEM4_ASCII_FIELD_SEPARATOR) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%hu%c", stInterHeader_.usSequence, OEM4_ASCII_FIELD_SEPARATOR) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%.1f%c", stInterHeader_.ucIdleTime * 0.500, OEM4_ASCII_FIELD_SEPARATOR) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%s%c", GetEnumString(vMyGPSTimeStatusDefns, stInterHeader_.uiTimeStatus).c_str(),
                         OEM4_ASCII_FIELD_SEPARATOR) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%hu%c", stInterHeader_.usWeek, OEM4_ASCII_FIELD_SEPARATOR) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%.3f%c", stInterHeader_.dMilliseconds / 1000.0, OEM4_ASCII_FIELD_SEPARATOR) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%08lx%c", stInterHeader_.uiReceiverStatus, OEM4_ASCII_FIELD_SEPARATOR) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%04x%c", stInterHeader_.uiMessageDefinitionCRC, OEM4_ASCII_FIELD_SEPARATOR) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%hu%c", stInterHeader_.usReceiverSwVersion, OEM4_ASCII_HEADER_TERMINATOR);
}

// -------------------------------------------------------------------------------------------------------
bool Encoder::EncodeAbbrevAsciiHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_, bool bIsEmbeddedHeader_)
{
    // Sync byte
    if (!bIsEmbeddedHeader_ && !PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%c", OEM4_ABBREV_ASCII_SYNC)) { return false; }

    // Message name
    const MessageDefinition* pclMessageDef = pclMyMsgDb->GetMsgDef(stInterHeader_.usMessageID);
    std::string sMsgName(pclMessageDef ? pclMessageDef->name : GetEnumString(vMyCommandDefns, stInterHeader_.usMessageID));
    const uint32_t uiSiblingID = stInterHeader_.ucMessageType & static_cast<uint32_t>(MESSAGETYPEMASK::MEASSRC);
    if (uiSiblingID) // Append sibling i.e. the _1 of RANGEA_1
        sMsgName.append("_").append(std::to_string(uiSiblingID));

    return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%s%c", sMsgName.c_str(), OEM4_ABBREV_ASCII_SEPARATOR) &&
                   PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%s%c", GetEnumString(vMyPortAddrDefns, stInterHeader_.uiPortAddress).c_str(),
                                 OEM4_ABBREV_ASCII_SEPARATOR) &&
                   PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%hu%c", stInterHeader_.usSequence, OEM4_ABBREV_ASCII_SEPARATOR) &&
                   PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%.1f%c", stInterHeader_.ucIdleTime * 0.500, OEM4_ABBREV_ASCII_SEPARATOR) &&
                   PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%s%c", GetEnumString(vMyGPSTimeStatusDefns, stInterHeader_.uiTimeStatus).c_str(),
                                 OEM4_ABBREV_ASCII_SEPARATOR) &&
                   PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%hu%c", stInterHeader_.usWeek, OEM4_ABBREV_ASCII_SEPARATOR) &&
                   PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%.3f%c", stInterHeader_.dMilliseconds / 1000.0, OEM4_ABBREV_ASCII_SEPARATOR) &&
                   PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%08lx%c", stInterHeader_.uiReceiverStatus, OEM4_ABBREV_ASCII_SEPARATOR) &&
                   PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%04x%c", stInterHeader_.uiMessageDefinitionCRC, OEM4_ABBREV_ASCII_SEPARATOR) &&
                   PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%hu", stInterHeader_.usReceiverSwVersion)
               ? bIsEmbeddedHeader_ ? PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%c", OEM4_ABBREV_ASCII_SEPARATOR)
                                    : PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "\r\n")
               : false;
}

// -------------------------------------------------------------------------------------------------------
bool Encoder::EncodeAsciiShortHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_)
{
    // Sync byte
    if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%c", OEM4_SHORT_ASCII_SYNC)) { return false; }

    // Message name
    std::string sMsgName(pclMyMsgDb->GetMsgDef(stInterHeader_.usMessageID)->name);
    const uint32_t uiSiblingID = stInterHeader_.ucMessageType & static_cast<uint32_t>(MESSAGETYPEMASK::MEASSRC);
    const uint32_t uiResponse = (stInterHeader_.ucMessageType & static_cast<uint32_t>(MESSAGETYPEMASK::RESPONSE)) >> 7;
    sMsgName.append(uiResponse ? "R" : "A"); // Append 'A' for ascii, or 'R' for ascii response
    if (uiSiblingID)                         // Append sibling i.e. the _1 of RANGEA_1
        sMsgName.append("_").append(std::to_string(uiSiblingID));

    return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%s%c", sMsgName.c_str(), OEM4_ASCII_FIELD_SEPARATOR) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%hu%c", stInterHeader_.usWeek, OEM4_ASCII_FIELD_SEPARATOR) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%.3f%c", stInterHeader_.dMilliseconds / 1000.0, OEM4_ASCII_HEADER_TERMINATOR);
}

// -------------------------------------------------------------------------------------------------------
bool Encoder::EncodeAbbrevAsciiShortHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_)
{
    // Sync byte
    if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%c", OEM4_ABBREV_ASCII_SYNC)) { return false; }

    // Message name
    std::string sMsgName(pclMyMsgDb->GetMsgDef(stInterHeader_.usMessageID)->name);
    uint32_t uiSiblingID = stInterHeader_.ucMessageType & static_cast<uint32_t>(MESSAGETYPEMASK::MEASSRC);
    if (uiSiblingID) // Append sibling i.e. the _1 of RANGEA_1
        sMsgName.append("_").append(std::to_string(uiSiblingID));

    return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%s%c", sMsgName.c_str(), OEM4_ABBREV_ASCII_SEPARATOR) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%hu%c", stInterHeader_.usWeek, OEM4_ABBREV_ASCII_SEPARATOR) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%.3f", stInterHeader_.dMilliseconds / 1000.0) && PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "\r\n");
}

// -------------------------------------------------------------------------------------------------------
std::string Encoder::JsonHeaderToMsgName(const IntermediateHeader& stInterHeader_) const
{
    // Message name
    const MessageDefinition* pclMessageDef = pclMyMsgDb->GetMsgDef(stInterHeader_.usMessageID);
    std::string sMsgName(pclMessageDef ? pclMessageDef->name : GetEnumString(vMyCommandDefns, stInterHeader_.usMessageID));
    uint32_t uiSiblingID = stInterHeader_.ucMessageType & 0b00011111;
    // Append sibling i.e. the _1 of RANGEA_1
    if (uiSiblingID) { sMsgName.append("_").append(std::to_string(uiSiblingID)); }

    return sMsgName;
}

// -------------------------------------------------------------------------------------------------------
bool Encoder::EncodeJsonHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_)
{
    return CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiBytesLeft_, "{") &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("message": "%s",)", JsonHeaderToMsgName(stInterHeader_).c_str()) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("id": %hu,)", stInterHeader_.usMessageID) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("port": "%s",)", GetEnumString(vMyPortAddrDefns, stInterHeader_.uiPortAddress).c_str()) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("sequence_num": %hu,)", stInterHeader_.usSequence) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("percent_idle_time": %.1f,)", static_cast<float>(stInterHeader_.ucIdleTime) * 0.500) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("time_status": "%s",)",
                         GetEnumString(vMyGPSTimeStatusDefns, stInterHeader_.uiTimeStatus).c_str()) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("week": %hu,)", stInterHeader_.usWeek) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("seconds": %.3f,)", (stInterHeader_.dMilliseconds / 1000.0)) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("receiver_status": %ld,)", stInterHeader_.uiReceiverStatus) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("HEADER_reserved1": %d,)", stInterHeader_.uiMessageDefinitionCRC) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("receiver_sw_version": %hu)", stInterHeader_.usReceiverSwVersion) &&
           CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiBytesLeft_, "}");
}

// -------------------------------------------------------------------------------------------------------
bool Encoder::EncodeJsonShortHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_)
{
    return CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiBytesLeft_, "{") &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("message": "%s",)", JsonHeaderToMsgName(stInterHeader_).c_str()) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("id": %hu,)", stInterHeader_.usMessageID) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("week": %hu,)", stInterHeader_.usWeek) &&
           PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("seconds": %.3f)", (stInterHeader_.dMilliseconds / 1000.0)) &&
           CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiBytesLeft_, "}");
}

// -------------------------------------------------------------------------------------------------------
STATUS
Encoder::Encode(unsigned char** ppucBuffer_, uint32_t uiBufferSize_, IntermediateHeader& stHeader_, IntermediateMessage& stMessage_,
                MessageDataStruct& stMessageData_, MetaDataStruct& stMetaData_, ENCODEFORMAT eFormat_)
{
    if (ppucBuffer_ == nullptr || *ppucBuffer_ == nullptr) { return STATUS::NULL_PROVIDED; }

    if (!pclMyMsgDb) { return STATUS::NO_DATABASE; }

    STATUS eStatus = STATUS::UNKNOWN;
    unsigned char* pucTempEncodeBuffer = *ppucBuffer_;

    if (eFormat_ == ENCODEFORMAT::JSON)
    {
        if (!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiBufferSize_, R"({"header": )")) { return STATUS::BUFFER_FULL; }
    }

    eStatus = EncodeHeader(&pucTempEncodeBuffer, uiBufferSize_, stHeader_, stMessageData_, stMetaData_, eFormat_);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    pucTempEncodeBuffer += stMessageData_.uiMessageHeaderLength;

    if (eFormat_ == ENCODEFORMAT::JSON)
    {
        if (!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiBufferSize_, R"(,"body": )")) { return STATUS::BUFFER_FULL; }
    }

    eStatus = EncodeBody(&pucTempEncodeBuffer, uiBufferSize_, stMessage_, stMessageData_, stMetaData_, eFormat_);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    pucTempEncodeBuffer += stMessageData_.uiMessageBodyLength;

    if (eFormat_ == ENCODEFORMAT::JSON)
    {
        if (!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiBufferSize_, R"(})")) { return STATUS::BUFFER_FULL; }
    }

    stMessageData_.pucMessage = *ppucBuffer_;
    stMessageData_.uiMessageLength = pucTempEncodeBuffer - *ppucBuffer_;

    return STATUS::SUCCESS;
}

// -------------------------------------------------------------------------------------------------------
STATUS
Encoder::EncodeHeader(unsigned char** ppucBuffer_, uint32_t uiBufferSize_, IntermediateHeader& stHeader_, MessageDataStruct& stMessageData_,
                      MetaDataStruct& stMetaData_, ENCODEFORMAT eFormat_, bool bIsEmbeddedHeader_)
{
    if (ppucBuffer_ == nullptr || *ppucBuffer_ == nullptr) { return STATUS::NULL_PROVIDED; }

    if (!pclMyMsgDb) { return STATUS::NO_DATABASE; }

    unsigned char* pucTempBuffer = *ppucBuffer_;
    auto ppcTempBuffer = reinterpret_cast<char**>(&pucTempBuffer);

    switch (eFormat_)
    {
    case ENCODEFORMAT::ASCII:
        if (isShortHeaderFormat(stMetaData_.eFormat) ? !EncodeAsciiShortHeader(stHeader_, ppcTempBuffer, uiBufferSize_)
                                                     : !EncodeAsciiHeader(stHeader_, ppcTempBuffer, uiBufferSize_))
        {
            return STATUS::BUFFER_FULL;
        }
        break;
    case ENCODEFORMAT::ABBREV_ASCII:
        if (isShortHeaderFormat(stMetaData_.eFormat) ? !EncodeAbbrevAsciiShortHeader(stHeader_, ppcTempBuffer, uiBufferSize_)
                                                     : !EncodeAbbrevAsciiHeader(stHeader_, ppcTempBuffer, uiBufferSize_, bIsEmbeddedHeader_))
        {
            return STATUS::BUFFER_FULL;
        }
        break;
    case ENCODEFORMAT::FLATTENED_BINARY: [[fallthrough]];
    case ENCODEFORMAT::BINARY:
        if (isShortHeaderFormat(stMetaData_.eFormat) ? !EncodeBinaryShortHeader(stHeader_, &pucTempBuffer, uiBufferSize_)
                                                     : !EncodeBinaryHeader(stHeader_, &pucTempBuffer, uiBufferSize_))
        {
            return STATUS::BUFFER_FULL;
        }
        break;
    case ENCODEFORMAT::JSON:
        if (isShortHeaderFormat(stMetaData_.eFormat) ? !EncodeJsonShortHeader(stHeader_, ppcTempBuffer, uiBufferSize_)
                                                     : !EncodeJsonHeader(stHeader_, ppcTempBuffer, uiBufferSize_))
        {
            return STATUS::BUFFER_FULL;
        }
        break;
    default: return STATUS::UNSUPPORTED;
    }

    // Record the length of the encoded message header.
    stMessageData_.pucMessageHeader = *ppucBuffer_;
    stMessageData_.uiMessageHeaderLength = pucTempBuffer - stMessageData_.pucMessageHeader;
    return STATUS::SUCCESS;
}

// -------------------------------------------------------------------------------------------------------
STATUS
Encoder::EncodeBody(unsigned char** ppucBuffer_, uint32_t uiBufferSize_, IntermediateMessage& stMessage_, MessageDataStruct& stMessageData_,
                    MetaDataStruct& stMetaData_, ENCODEFORMAT eFormat_)
{
    // TODO: this entire function should be in common, only header stuff and map redefinitions belong in this file
    if (ppucBuffer_ == nullptr || *ppucBuffer_ == nullptr) { return STATUS::NULL_PROVIDED; }

    if (!pclMyMsgDb) { return STATUS::NO_DATABASE; }

    unsigned char* pucTempBuffer = *ppucBuffer_;

    switch (eFormat_)
    {
    case ENCODEFORMAT::ASCII: {
        if (!EncodeAsciiBody<false>(stMessage_, reinterpret_cast<char**>(&pucTempBuffer), uiBufferSize_)) { return STATUS::BUFFER_FULL; }
        pucTempBuffer--; // Remove last delimiter ','
        uint32_t uiCRC = CalculateBlockCRC32(pucTempBuffer - stMessageData_.pucMessageHeader - 1, 0, stMessageData_.pucMessageHeader + 1);
        if (!PrintToBuffer(reinterpret_cast<char**>(&pucTempBuffer), uiBufferSize_, "*%08x\r\n", uiCRC)) { return STATUS::BUFFER_FULL; }
        break;
    }
    case ENCODEFORMAT::ABBREV_ASCII:
        if (!EncodeAsciiBody<true>(stMessage_, reinterpret_cast<char**>(&pucTempBuffer), uiBufferSize_)) { return STATUS::BUFFER_FULL; }
        pucTempBuffer--; // Remove last delimiter ' '
        if (!PrintToBuffer(reinterpret_cast<char**>(&pucTempBuffer), uiBufferSize_, "\r\n")) { return STATUS::BUFFER_FULL; }
        break;

    case ENCODEFORMAT::FLATTENED_BINARY:
        if (!EncodeBinaryBody<true>(stMessage_, &pucTempBuffer, uiBufferSize_)) { return STATUS::BUFFER_FULL; }
        [[fallthrough]];

    case ENCODEFORMAT::BINARY: {
        if (eFormat_ == ENCODEFORMAT::BINARY && !EncodeBinaryBody<false>(stMessage_, &pucTempBuffer, uiBufferSize_)) { return STATUS::BUFFER_FULL; }
        // MessageData must have a valid MessageHeader pointer to populate the length field.
        if (stMessageData_.pucMessageHeader == nullptr) { return STATUS::FAILURE; }
        // Go back and set the length field in the header.
        // TODO: this little block of code below is whats blocking us from moving this function to common
        if (stMetaData_.eFormat == HEADERFORMAT::ASCII || stMetaData_.eFormat == HEADERFORMAT::BINARY ||
            stMetaData_.eFormat == HEADERFORMAT::ABB_ASCII)
        {
            reinterpret_cast<OEM4BinaryHeader*>(stMessageData_.pucMessageHeader)->usLength = static_cast<uint16_t>(pucTempBuffer - *ppucBuffer_);
        }
        else
        {
            reinterpret_cast<OEM4BinaryShortHeader*>(stMessageData_.pucMessageHeader)->ucLength = static_cast<uint8_t>(pucTempBuffer - *ppucBuffer_);
        }
        uint32_t uiCRC = CalculateBlockCRC32(pucTempBuffer - stMessageData_.pucMessageHeader, 0, stMessageData_.pucMessageHeader);
        if (!CopyToBuffer(&pucTempBuffer, uiBufferSize_, &uiCRC)) { return STATUS::BUFFER_FULL; }
        break;
    }
    case ENCODEFORMAT::JSON:
        if (!EncodeJsonBody(stMessage_, reinterpret_cast<char**>(&pucTempBuffer), uiBufferSize_)) { return STATUS::BUFFER_FULL; }
        break;

    default: return STATUS::UNSUPPORTED;
    }

    // Record the length of the encoded message body.
    stMessageData_.pucMessageBody = *ppucBuffer_;
    stMessageData_.uiMessageBodyLength = pucTempBuffer - stMessageData_.pucMessageBody;
    return STATUS::SUCCESS;
}
