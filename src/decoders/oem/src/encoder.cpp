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

#include "novatel_edie/decoders/oem/encoder.hpp"

#include <charconv>

using namespace novatel::edie;
using namespace novatel::edie::oem;

// -------------------------------------------------------------------------------------------------------
void AppendSiblingId(std::string& sMsgName_, const IntermediateHeader& stInterHeader_)
{
    const uint32_t uiSiblingId = stInterHeader_.ucMessageType & static_cast<uint32_t>(MESSAGE_TYPE_MASK::MEASSRC);

    if (uiSiblingId != 0U)
    {
        // Reserve space for the suffix: "_" + up to 3 digits (max for uint8_t)
        constexpr size_t maxSuffixSize = 4; // "_" + up to 3 digits
        sMsgName_.reserve(sMsgName_.size() + maxSuffixSize);

        // Append the underscore
        sMsgName_.push_back('_');

        // Convert the sibling ID to a string and append it
        std::array<char, maxSuffixSize> buffer; // Enough space for a uint8_t (up to 3 digits) + null terminator
        auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), uiSiblingId, 10);
        if (ec == std::errc()) { sMsgName_.append(buffer.data(), ptr); }
    }
}

// -------------------------------------------------------------------------------------------------------
Encoder::Encoder(MessageDatabase* pclMessageDb_) : EncoderBase(pclMessageDb_)
{
    InitFieldMaps();
    if (pclMessageDb_ != nullptr) { LoadJsonDb(pclMessageDb_); }
}

Encoder::Encoder(MessageDatabase::ConstPtr pclMessageDb_) : EncoderBase(pclMessageDb_)
{
    InitFieldMaps();
    if (pclMessageDb_ != nullptr) { LoadSharedJsonDb(pclMessageDb_); }
}

// -------------------------------------------------------------------------------------------------------
void Encoder::InitEnumDefinitions()
{
    vMyCommandDefinitions = pclMyMsgDb->GetEnumDefName("Commands");
    vMyPortAddressDefinitions = pclMyMsgDb->GetEnumDefName("PortAddress");
    vMyGpsTimeStatusDefinitions = pclMyMsgDb->GetEnumDefName("GPSTimeStatus");
}

// -------------------------------------------------------------------------------------------------------
void Encoder::InitFieldMaps()
{
    // =========================================================
    // ASCII Field Mapping
    // =========================================================
    asciiFieldMap[CalculateBlockCrc32("s")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 [[maybe_unused]] const MessageDatabase& pclMsgDb_) {
        return fc_.fieldDef->dataType.name == DATA_TYPE::UCHAR
                   ? CopyToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<char>(std::get<uint8_t>(fc_.fieldValue)))
                   : CopyToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<char>(std::get<int8_t>(fc_.fieldValue)));
    };

    asciiFieldMap[CalculateBlockCrc32("m")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 [[maybe_unused]] const MessageDatabase& pclMsgDb_) {
        return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, pclMsgDb_.MsgIdToMsgName(std::get<uint32_t>(fc_.fieldValue)).c_str());
    };

    asciiFieldMap[CalculateBlockCrc32("T")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 [[maybe_unused]] const MessageDatabase& pclMsgDb_) {
        return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "{:.3f}", std::get<uint32_t>(fc_.fieldValue) / 1000.0);
    };

    asciiFieldMap[CalculateBlockCrc32("id")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                  [[maybe_unused]] const MessageDatabase& pclMsgDb_) {
        const uint32_t uiTempId = std::get<uint32_t>(fc_.fieldValue);
        const uint16_t usSv = uiTempId & 0x0000FFFF;
        const int16_t sGloChan = (uiTempId & 0xFFFF0000) >> 16;
        return (sGloChan < 0)    ? CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, usSv, sGloChan)
               : (sGloChan != 0) ? CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, usSv, '+', sGloChan)
                                 : PrintIntToBuffer(ppcOutBuf_, uiBytesLeft_, usSv);
    };

    asciiFieldMap[CalculateBlockCrc32("P")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 [[maybe_unused]] const MessageDatabase& pclMsgDb_) {
        const uint8_t uiValue = std::get<uint8_t>(fc_.fieldValue);
        if (uiValue == '\\') { return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, "\\\\"); }
        if (uiValue > 31 && uiValue < 127) { return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<char>(uiValue)); }
        return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, "\\x") && PrintHexToBuffer(ppcOutBuf_, uiBytesLeft_, 2, uiValue);
    };

    asciiFieldMap[CalculateBlockCrc32("k")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 [[maybe_unused]] const MessageDatabase& pclMsgDb_) {
        return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, FloatingPointConversionString<float>(fc_).data(), std::get<float>(fc_.fieldValue));
    };

    asciiFieldMap[CalculateBlockCrc32("lk")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                  [[maybe_unused]] const MessageDatabase& pclMsgDb_) {
        return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, FloatingPointConversionString<double>(fc_).data(), std::get<double>(fc_.fieldValue));
    };

    asciiFieldMap[CalculateBlockCrc32("c")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 [[maybe_unused]] const MessageDatabase& pclMsgDb_) {
        if (fc_.fieldDef->dataType.length == 1)
        {
            return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<char>(std::get<uint8_t>(fc_.fieldValue)));
        }
        if (fc_.fieldDef->dataType.length == 4 && fc_.fieldDef->dataType.name == DATA_TYPE::ULONG)
        {
            return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<char>(std::get<uint32_t>(fc_.fieldValue)));
        }
        return false;
    };

    // =========================================================
    // Json Field Mapping
    // =========================================================
    jsonFieldMap[CalculateBlockCrc32("P")] = BasicIntMapEntry<uint8_t>();

    jsonFieldMap[CalculateBlockCrc32("T")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                [[maybe_unused]] const MessageDatabase& pclMsgDb_) {
        return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "{:.3f}", std::get<uint32_t>(fc_.fieldValue) / 1000.0);
    };

    jsonFieldMap[CalculateBlockCrc32("m")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                [[maybe_unused]] const MessageDatabase& pclMsgDb_) {
        return CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, "\"", std::string_view(pclMsgDb_.MsgIdToMsgName(std::get<uint32_t>(fc_.fieldValue))), "\"");
    };

    jsonFieldMap[CalculateBlockCrc32("id")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 [[maybe_unused]] const MessageDatabase& pclMsgDb_) {
        const auto uiTempId = std::get<uint32_t>(fc_.fieldValue);
        const uint16_t usSv = uiTempId & 0x0000FFFF;
        const int16_t sGloChan = (uiTempId & 0xFFFF0000) >> 16;
        return (sGloChan < 0)    ? CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', usSv, sGloChan, '"')
               : (sGloChan != 0) ? CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', usSv, '+', sGloChan, '"')
                                 : CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', usSv, '"');
    };

    jsonFieldMap[CalculateBlockCrc32("k")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                [[maybe_unused]] const MessageDatabase& pclMsgDb_) {
        return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, FloatingPointConversionString<float>(fc_).data(), std::get<float>(fc_.fieldValue));
    };

    jsonFieldMap[CalculateBlockCrc32("lk")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 [[maybe_unused]] const MessageDatabase& pclMsgDb_) {
        return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, FloatingPointConversionString<double>(fc_).data(), std::get<double>(fc_.fieldValue));
    };

    jsonFieldMap[CalculateBlockCrc32("s")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                [[maybe_unused]] const MessageDatabase& pclMsgDb_) {
        return CopyToBuffer(ppcOutBuf_, uiBytesLeft_,
                            fc_.fieldDef->dataType.name == DATA_TYPE::UCHAR ? static_cast<char>(std::get<uint8_t>(fc_.fieldValue))
                                                                            : static_cast<char>(std::get<int8_t>(fc_.fieldValue)));
    };

    jsonFieldMap[CalculateBlockCrc32("c")] = [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                [[maybe_unused]] const MessageDatabase& pclMsgDb_) {
        if (fc_.fieldDef->dataType.length == 1)
        {
            return CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', static_cast<char>(std::get<uint8_t>(fc_.fieldValue)), '"');
        }
        if (fc_.fieldDef->dataType.length == 4 && fc_.fieldDef->dataType.name == DATA_TYPE::ULONG)
        {
            return CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', static_cast<char>(std::get<uint32_t>(fc_.fieldValue)), '"');
        }
        return false;
    };
}

// -------------------------------------------------------------------------------------------------------
bool Encoder::EncodeBinaryHeader(const IntermediateHeader& stInterHeader_, unsigned char** ppcOutBuf_, uint32_t& uiBytesLeft_)
{
    return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, Oem4BinaryHeader(stInterHeader_));
}

// -------------------------------------------------------------------------------------------------------
bool Encoder::EncodeBinaryShortHeader(const IntermediateHeader& stInterHeader_, unsigned char** ppcOutBuf_, uint32_t& uiBytesLeft_)
{
    return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, Oem4BinaryShortHeader(stInterHeader_));
}

// -------------------------------------------------------------------------------------------------------
bool Encoder::FieldToBinary(const FieldContainer& fc_, unsigned char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
{
    if (const auto* pValue = std::get_if<bool>(&fc_.fieldValue)) { return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<int32_t>(*pValue)); }

    return EncoderBase::FieldToBinary(fc_, ppcOutBuf_, uiBytesLeft_);
}

// -------------------------------------------------------------------------------------------------------
bool Encoder::EncodeAsciiHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
{
    if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, OEM4_ASCII_SYNC)) { return false; }

    MessageDefinition::ConstPtr pclMessageDef = pclMyMsgDb->GetMsgDef(stInterHeader_.usMessageId);
    std::string sMsgName(pclMessageDef != nullptr ? pclMessageDef->name : GetEnumString(vMyCommandDefinitions, stInterHeader_.usMessageId));
    const uint32_t uiResponse = (stInterHeader_.ucMessageType & static_cast<uint32_t>(MESSAGE_TYPE_MASK::RESPONSE)) >> 7;
    sMsgName.push_back(uiResponse != 0U ? 'R' : 'A'); // Append 'A' for ascii, or 'R' for ascii response
    AppendSiblingId(sMsgName, stInterHeader_);

    return CopyAllToBufferSeparated(ppcOutBuf_, uiBytesLeft_, OEM4_ASCII_FIELD_SEPARATOR,
                                    std::string_view(sMsgName),                                                              //
                                    GetEnumString(vMyPortAddressDefinitions, stInterHeader_.uiPortAddress),                  //
                                    stInterHeader_.usSequence,                                                               //
                                    FormattedValue<float>{static_cast<float>(stInterHeader_.ucIdleTime) * 0.500F, "{:.1f}"}, //
                                    GetEnumString(vMyGpsTimeStatusDefinitions, stInterHeader_.uiTimeStatus),                 //
                                    stInterHeader_.usWeek,                                                                   //
                                    FormattedValue<double>{stInterHeader_.dMilliseconds / 1000.0, "{:.3f}"},                 //
                                    HexValue<uint32_t>{stInterHeader_.uiReceiverStatus, 8},                                  //
                                    HexValue<uint32_t>{stInterHeader_.uiMessageDefinitionCrc, 4},                            //
                                    stInterHeader_.usReceiverSwVersion                                                       //
                                    ) &&
           CopyToBuffer(ppcOutBuf_, uiBytesLeft_, OEM4_ASCII_HEADER_TERMINATOR);
}

// -------------------------------------------------------------------------------------------------------
bool Encoder::EncodeAbbrevAsciiHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_, bool bIsEmbedded_) const
{
    if (!bIsEmbedded_ && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, OEM4_ABBREV_ASCII_SYNC)) { return false; }

    MessageDefinition::ConstPtr pclMessageDef = pclMyMsgDb->GetMsgDef(stInterHeader_.usMessageId);
    std::string sMsgName(pclMessageDef != nullptr ? pclMessageDef->name : GetEnumString(vMyCommandDefinitions, stInterHeader_.usMessageId));
    AppendSiblingId(sMsgName, stInterHeader_);

    return CopyAllToBufferSeparated(ppcOutBuf_, uiBytesLeft_, OEM4_ABBREV_ASCII_SEPARATOR,
                                    std::string_view(sMsgName),                                                              //
                                    GetEnumString(vMyPortAddressDefinitions, stInterHeader_.uiPortAddress),                  //
                                    stInterHeader_.usSequence,                                                               //
                                    FormattedValue<float>{static_cast<float>(stInterHeader_.ucIdleTime) * 0.500F, "{:.1f}"}, //
                                    GetEnumString(vMyGpsTimeStatusDefinitions, stInterHeader_.uiTimeStatus),                 //
                                    stInterHeader_.usWeek,                                                                   //
                                    FormattedValue<double>{stInterHeader_.dMilliseconds / 1000.0, "{:.3f}"},                 //
                                    HexValue<uint32_t>{stInterHeader_.uiReceiverStatus, 8},                                  //
                                    HexValue<uint32_t>{stInterHeader_.uiMessageDefinitionCrc, 4},                            //
                                    stInterHeader_.usReceiverSwVersion                                                       //
                                    ) &&
           (bIsEmbedded_ ? CopyToBuffer(ppcOutBuf_, uiBytesLeft_, OEM4_ABBREV_ASCII_SEPARATOR) : CopyToBuffer(ppcOutBuf_, uiBytesLeft_, "\r\n"));
}

// -------------------------------------------------------------------------------------------------------
bool Encoder::EncodeAsciiShortHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
{
    if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, OEM4_SHORT_ASCII_SYNC)) { return false; }

    std::string sMsgName(pclMyMsgDb->GetMsgDef(stInterHeader_.usMessageId)->name);
    const uint32_t uiResponse = (stInterHeader_.ucMessageType & static_cast<uint32_t>(MESSAGE_TYPE_MASK::RESPONSE)) >> 7;
    sMsgName.push_back(uiResponse != 0U ? 'R' : 'A'); // Append 'A' for ascii, or 'R' for ascii response
    AppendSiblingId(sMsgName, stInterHeader_);

    return CopyAllToBufferSeparated(ppcOutBuf_, uiBytesLeft_, OEM4_ASCII_FIELD_SEPARATOR,                   //
                                    std::string_view(sMsgName),                                             //
                                    stInterHeader_.usWeek,                                                  //
                                    FormattedValue<double>{stInterHeader_.dMilliseconds / 1000.0, "{:.3f}"} //
                                    ) &&
           CopyToBuffer(ppcOutBuf_, uiBytesLeft_, OEM4_ASCII_HEADER_TERMINATOR);
}

// -------------------------------------------------------------------------------------------------------
bool Encoder::EncodeAbbrevAsciiShortHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
{
    if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, OEM4_ABBREV_ASCII_SYNC)) { return false; }

    std::string sMsgName(pclMyMsgDb->GetMsgDef(stInterHeader_.usMessageId)->name);
    AppendSiblingId(sMsgName, stInterHeader_);

    return CopyAllToBufferSeparated(ppcOutBuf_, uiBytesLeft_, OEM4_ABBREV_ASCII_SEPARATOR,                  //
                                    std::string_view(sMsgName),                                             //
                                    stInterHeader_.usWeek,                                                  //
                                    FormattedValue<double>{stInterHeader_.dMilliseconds / 1000.0, "{:.3f}"} //
                                    ) &&
           CopyToBuffer(ppcOutBuf_, uiBytesLeft_, "\r\n");
}

// -------------------------------------------------------------------------------------------------------
std::string Encoder::JsonHeaderToMsgName(const IntermediateHeader& stInterHeader_) const
{
    MessageDefinition::ConstPtr pclMessageDef = pclMyMsgDb->GetMsgDef(stInterHeader_.usMessageId);
    std::string sMsgName(pclMessageDef != nullptr ? pclMessageDef->name : GetEnumString(vMyCommandDefinitions, stInterHeader_.usMessageId));
    AppendSiblingId(sMsgName, stInterHeader_);

    return sMsgName;
}

// -------------------------------------------------------------------------------------------------------
bool Encoder::EncodeJsonHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
{
    return CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_,                                                                                              //
                           R"({"message": ")", JsonHeaderToMsgName(stInterHeader_).c_str(),                                                       //
                           R"(","id": )", stInterHeader_.usMessageId,                                                                             //
                           R"(,"port": ")", GetEnumString(vMyPortAddressDefinitions, stInterHeader_.uiPortAddress),                               //
                           R"(","sequence_num": )", stInterHeader_.usSequence,                                                                    //
                           R"(,"percent_idle_time": )", FormattedValue<double>{static_cast<double>(stInterHeader_.ucIdleTime) * 0.500, "{:.1f}"}, //
                           R"(,"time_status": ")", GetEnumString(vMyGpsTimeStatusDefinitions, stInterHeader_.uiTimeStatus),                       //
                           R"(","week": )", stInterHeader_.usWeek,                                                                                //
                           R"(,"seconds": )", FormattedValue<double>{(stInterHeader_.dMilliseconds / 1000.0), "{:.3f}"},                          //
                           R"(,"receiver_status": )", stInterHeader_.uiReceiverStatus,                                                            //
                           R"(,"HEADER_reserved1": )", stInterHeader_.uiMessageDefinitionCrc,                                                     //
                           R"(,"receiver_sw_version": )", stInterHeader_.usReceiverSwVersion, '}');
}

// -------------------------------------------------------------------------------------------------------
bool Encoder::EncodeJsonShortHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
{
    return CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_,                                        //
                           R"({"message": ")", JsonHeaderToMsgName(stInterHeader_).c_str(), //
                           R"(","id": )", stInterHeader_.usMessageId,                       //
                           R"(,"week": )", stInterHeader_.usWeek,                           //
                           R"(,"seconds": )", FormattedValue<double>{(stInterHeader_.dMilliseconds / 1000.0), "{:.3f}}"});
}

// -------------------------------------------------------------------------------------------------------
STATUS
Encoder::Encode(unsigned char** ppucBuffer_, uint32_t uiBufferSize_, const IntermediateHeader& stHeader_,
                const std::vector<FieldContainer>& stMessage_, MessageDataStruct& stMessageData_, HEADER_FORMAT eHeaderFormat_,
                const ENCODE_FORMAT eFormat_) const
{
    if (ppucBuffer_ == nullptr || *ppucBuffer_ == nullptr) { return STATUS::NULL_PROVIDED; }

    if (pclMyMsgDb == nullptr) { return STATUS::NO_DATABASE; }

    unsigned char* pucTempEncodeBuffer = *ppucBuffer_;

    if (eFormat_ == ENCODE_FORMAT::JSON)
    {
        if (!CopyToBuffer(&pucTempEncodeBuffer, uiBufferSize_, R"({"header": )")) { return STATUS::BUFFER_FULL; }
    }

    STATUS eStatus = EncodeHeader(&pucTempEncodeBuffer, uiBufferSize_, stHeader_, stMessageData_, eHeaderFormat_, eFormat_);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    pucTempEncodeBuffer += stMessageData_.uiMessageHeaderLength;

    if (eFormat_ == ENCODE_FORMAT::JSON)
    {
        if (!CopyToBuffer(&pucTempEncodeBuffer, uiBufferSize_, R"(,"body": )")) { return STATUS::BUFFER_FULL; }
    }

    eStatus = EncodeBody(&pucTempEncodeBuffer, uiBufferSize_, stMessage_, stMessageData_, eHeaderFormat_, eFormat_);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    pucTempEncodeBuffer += stMessageData_.uiMessageBodyLength;

    if (eFormat_ == ENCODE_FORMAT::JSON)
    {
        if (!CopyToBuffer(&pucTempEncodeBuffer, uiBufferSize_, '}')) { return STATUS::BUFFER_FULL; }
    }

    stMessageData_.pucMessage = *ppucBuffer_;
    stMessageData_.uiMessageLength = pucTempEncodeBuffer - *ppucBuffer_;

    return STATUS::SUCCESS;
}

// -------------------------------------------------------------------------------------------------------
STATUS
Encoder::EncodeHeader(unsigned char** ppucBuffer_, uint32_t uiBufferSize_, const IntermediateHeader& stHeader_, MessageDataStruct& stMessageData_,
                      const HEADER_FORMAT eHeaderFormat_, const ENCODE_FORMAT eFormat_, const bool bIsEmbeddedHeader_) const
{
    if (ppucBuffer_ == nullptr || *ppucBuffer_ == nullptr) { return STATUS::NULL_PROVIDED; }

    if (pclMyMsgDb == nullptr) { return STATUS::NO_DATABASE; }

    unsigned char* pucTempBuffer = *ppucBuffer_;
    auto** ppcTempBuffer = reinterpret_cast<char**>(&pucTempBuffer);

    switch (eFormat_)
    {
    case ENCODE_FORMAT::ASCII:
        if (IsShortHeaderFormat(eHeaderFormat_) ? !EncodeAsciiShortHeader(stHeader_, ppcTempBuffer, uiBufferSize_)
                                                : !EncodeAsciiHeader(stHeader_, ppcTempBuffer, uiBufferSize_))
        {
            return STATUS::BUFFER_FULL;
        }
        break;
    case ENCODE_FORMAT::ABBREV_ASCII:
        if (IsShortHeaderFormat(eHeaderFormat_) ? !EncodeAbbrevAsciiShortHeader(stHeader_, ppcTempBuffer, uiBufferSize_)
                                                : !EncodeAbbrevAsciiHeader(stHeader_, ppcTempBuffer, uiBufferSize_, bIsEmbeddedHeader_))
        {
            return STATUS::BUFFER_FULL;
        }
        break;
    case ENCODE_FORMAT::FLATTENED_BINARY: [[fallthrough]];
    case ENCODE_FORMAT::BINARY:
        if (IsShortHeaderFormat(eHeaderFormat_) ? !EncodeBinaryShortHeader(stHeader_, &pucTempBuffer, uiBufferSize_)
                                                : !EncodeBinaryHeader(stHeader_, &pucTempBuffer, uiBufferSize_))
        {
            return STATUS::BUFFER_FULL;
        }
        break;
    case ENCODE_FORMAT::JSON:
        if (IsShortHeaderFormat(eHeaderFormat_) ? !EncodeJsonShortHeader(stHeader_, ppcTempBuffer, uiBufferSize_)
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
Encoder::EncodeBody(unsigned char** ppucBuffer_, uint32_t uiBufferSize_, const std::vector<FieldContainer>& stMessage_,
                    MessageDataStruct& stMessageData_, const HEADER_FORMAT eHeaderFormat_, ENCODE_FORMAT eFormat_) const
{
    // TODO: this entire function should be in common, only header stuff and map redefinitions belong in this file
    if (ppucBuffer_ == nullptr || *ppucBuffer_ == nullptr) { return STATUS::NULL_PROVIDED; }

    if (pclMyMsgDb == nullptr) { return STATUS::NO_DATABASE; }

    unsigned char* pucTempBuffer = *ppucBuffer_;

    switch (eFormat_)
    {
    case ENCODE_FORMAT::ASCII: {
        if (!EncodeAsciiBody<false>(stMessage_, reinterpret_cast<char**>(&pucTempBuffer), uiBufferSize_)) { return STATUS::BUFFER_FULL; }
        pucTempBuffer--; // Remove last delimiter ','
        const uint32_t uiCrc = CalculateBlockCrc32(stMessageData_.pucMessageHeader + 1, pucTempBuffer - stMessageData_.pucMessageHeader - 1);
        if (!CopyAllToBuffer(reinterpret_cast<char**>(&pucTempBuffer), uiBufferSize_, '*', HexValue<uint32_t>{uiCrc, 8}, "\r\n"))
        {
            return STATUS::BUFFER_FULL;
        }
        break;
    }
    case ENCODE_FORMAT::ABBREV_ASCII:
        if (!EncodeAsciiBody<true>(stMessage_, reinterpret_cast<char**>(&pucTempBuffer), uiBufferSize_)) { return STATUS::BUFFER_FULL; }
        pucTempBuffer--; // Remove last delimiter ' '
        if (!CopyToBuffer(&pucTempBuffer, uiBufferSize_, "\r\n")) { return STATUS::BUFFER_FULL; }
        break;

    case ENCODE_FORMAT::FLATTENED_BINARY:
        if (!EncodeBinaryBody<true, true>(stMessage_, &pucTempBuffer, uiBufferSize_)) { return STATUS::BUFFER_FULL; }
        [[fallthrough]];

    case ENCODE_FORMAT::BINARY: {
        if (eFormat_ == ENCODE_FORMAT::BINARY && !EncodeBinaryBody<false, true>(stMessage_, &pucTempBuffer, uiBufferSize_))
        {
            return STATUS::BUFFER_FULL;
        }
        // MessageData must have a valid MessageHeader pointer to populate the length field.
        if (stMessageData_.pucMessageHeader == nullptr) { return STATUS::FAILURE; }
        // Go back and set the length field in the header.
        // TODO: this block of code below is what's blocking us from moving this function to common (can solve this with dynamic casting or CRTP?)
        if (eHeaderFormat_ == HEADER_FORMAT::ASCII || eHeaderFormat_ == HEADER_FORMAT::BINARY || eHeaderFormat_ == HEADER_FORMAT::ABB_ASCII)
        {
            reinterpret_cast<Oem4BinaryHeader*>(stMessageData_.pucMessageHeader)->usLength = static_cast<uint16_t>(pucTempBuffer - *ppucBuffer_);
        }
        else
        {
            reinterpret_cast<Oem4BinaryShortHeader*>(stMessageData_.pucMessageHeader)->ucLength = static_cast<uint8_t>(pucTempBuffer - *ppucBuffer_);
        }
        if (!CopyToBuffer(&pucTempBuffer, uiBufferSize_,
                          CalculateBlockCrc32(stMessageData_.pucMessageHeader, pucTempBuffer - stMessageData_.pucMessageHeader)))
        {
            return STATUS::BUFFER_FULL;
        }
        break;
    }
    case ENCODE_FORMAT::JSON:
        if (!EncodeJsonBody(stMessage_, reinterpret_cast<char**>(&pucTempBuffer), uiBufferSize_)) { return STATUS::BUFFER_FULL; }
        break;

    default: return STATUS::UNSUPPORTED;
    }

    // Record the length of the encoded message body.
    stMessageData_.pucMessageBody = *ppucBuffer_;
    stMessageData_.uiMessageBodyLength = pucTempBuffer - stMessageData_.pucMessageBody;
    return STATUS::SUCCESS;
}
