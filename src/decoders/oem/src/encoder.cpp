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

#include <algorithm>
#include <charconv>
#include <cstring>

using namespace novatel::edie;
using namespace novatel::edie::oem;

// -------------------------------------------------------------------------------------------------------
void AppendSiblingId(std::string& sMsgName_, const IntermediateHeader& stInterHeader_)
{
    const uint32_t ucSiblingId = stInterHeader_.ucMessageType & static_cast<uint32_t>(MESSAGE_TYPE_MASK::MEASSRC);

    if (ucSiblingId != 0U)
    {
        // Reserve space for the suffix: "_" + up to 3 digits (max for uint8_t)
        constexpr size_t maxSuffixSize = 4; // "_" + up to 3 digits
        sMsgName_.reserve(sMsgName_.size() + maxSuffixSize);

        // Append the underscore
        sMsgName_.push_back('_');

        // Convert the sibling ID to a string and append it
        std::array<char, maxSuffixSize> buffer; // Enough space for a uint8_t (up to 3 digits) + null terminator
        auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), ucSiblingId, 10);
        if (ec == std::errc()) { sMsgName_.append(buffer.data(), ptr); }
    }
}

// -------------------------------------------------------------------------------------------------------
Encoder::Encoder(MessageDatabase::ConstPtr pclMessageDb_) : EncoderBase("OEM", pclMessageDb_, OemAlignmentFunction)
{
    if (pclMessageDb_ != nullptr) { LoadJsonDb(pclMessageDb_); }
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
    asciiFieldMap.clear();
    jsonFieldMap.clear();

    // =========================================================
    // ASCII Field Mapping
    // =========================================================
    asciiFieldMap[CalculateBlockCrc32("UB")] = BasicIntMapEntry<uint8_t>();
    asciiFieldMap[CalculateBlockCrc32("B")] = BasicIntMapEntry<int8_t>();
    asciiFieldMap[CalculateBlockCrc32("XB")] = BasicHexMapEntry<uint8_t>(2);

    asciiFieldMap[CalculateBlockCrc32("x")] = [](const BaseField& fd_, const MessageBody& mb_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 const MessageDatabase&, size_t offset) {
        if (fd_.dataType.length == 1) { return WriteHexToBuffer(ppcOutBuf_, uiBytesLeft_, mb_.GetFieldValue<uint8_t>(fd_, offset), 2); }
        if (fd_.dataType.length == 2) { return WriteHexToBuffer(ppcOutBuf_, uiBytesLeft_, mb_.GetFieldValue<uint16_t>(fd_, offset), 4); }
        if (fd_.dataType.length == 4) { return WriteHexToBuffer(ppcOutBuf_, uiBytesLeft_, mb_.GetFieldValue<uint32_t>(fd_, offset), 8); }
        if (fd_.dataType.length == 8) { return WriteHexToBuffer(ppcOutBuf_, uiBytesLeft_, mb_.GetFieldValue<uint64_t>(fd_, offset), 16); }
        return false;
    };

    asciiFieldMap[CalculateBlockCrc32("X")] = asciiFieldMap[CalculateBlockCrc32("x")];
    asciiFieldMap[CalculateBlockCrc32("lx")] = BasicHexMapEntry<uint32_t>(8);
    asciiFieldMap[CalculateBlockCrc32("llx")] = BasicHexMapEntry<uint64_t>(16);

    asciiFieldMap[CalculateBlockCrc32("s")] = [](const BaseField& fd_, const MessageBody& mb_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 const MessageDatabase&, size_t offset) {
        return fd_.dataType.name == DATA_TYPE::UCHAR
                   ? CopyToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<char>(mb_.GetFieldValue<uint8_t>(fd_, offset)))
                   : CopyToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<char>(mb_.GetFieldValue<int8_t>(fd_, offset)));
    };

    asciiFieldMap[CalculateBlockCrc32("m")] = [](const BaseField& fd_, const MessageBody& mb_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 const MessageDatabase& pclMsgDb_, size_t offset) {
        return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, pclMsgDb_.MsgIdToMsgName(mb_.GetFieldValue<uint32_t>(fd_, offset)).c_str());
    };

    asciiFieldMap[CalculateBlockCrc32("T")] = [](const BaseField& fd_, const MessageBody& mb_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 const MessageDatabase&, size_t offset) {
        return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, mb_.GetFieldValue<uint32_t>(fd_, offset) / 1000.0, std::chars_format::fixed, 3);
    };

    asciiFieldMap[CalculateBlockCrc32("id")] = [](const BaseField& fd_, const MessageBody& mb_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                  const MessageDatabase&, size_t offset) {
        const auto uiTempId = mb_.GetFieldValue<uint32_t>(fd_, offset);
        const uint16_t usSv = uiTempId & 0x0000FFFF;
        const int16_t sGloChan = (uiTempId & 0xFFFF0000) >> 16;
        return (sGloChan < 0)    ? CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, usSv, sGloChan)
               : (sGloChan != 0) ? CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, usSv, '+', sGloChan)
                                 : WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, usSv);
    };

    asciiFieldMap[CalculateBlockCrc32("P")] = [](const BaseField& fd_, const MessageBody& mb_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 const MessageDatabase&, size_t offset) {
        const uint8_t uiValue = fd_.dataType.name == DATA_TYPE::CHAR ? static_cast<uint8_t>(mb_.GetFieldValue<int8_t>(fd_, offset))
                                                                     : mb_.GetFieldValue<uint8_t>(fd_, offset);
        if (uiValue == '\\') { return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, "\\\\"); }
        if (uiValue > 31 && uiValue < 127) { return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<char>(uiValue)); }
        return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, "\\x") && WriteHexToBuffer(ppcOutBuf_, uiBytesLeft_, uiValue, 2);
    };

    asciiFieldMap[CalculateBlockCrc32("f")] = [](const BaseField& fd_, const MessageBody& mb_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 const MessageDatabase&, size_t offset) {
        if (fd_.dataType.length == 4)
        {
            return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, mb_.GetFieldValue<float>(fd_, offset), std::chars_format::fixed, fd_.precision);
        }
        if (fd_.dataType.length == 8)
        {
            return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, mb_.GetFieldValue<double>(fd_, offset), std::chars_format::fixed, fd_.precision);
        }
        return false;
    };

    asciiFieldMap[CalculateBlockCrc32("lf")] = [](const BaseField& fd_, const MessageBody& mb_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                  const MessageDatabase&, size_t offset) {
        return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, mb_.GetFieldValue<double>(fd_, offset), std::chars_format::fixed, fd_.precision);
    };

    asciiFieldMap[CalculateBlockCrc32("e")] = [](const BaseField& fd_, const MessageBody& mb_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 const MessageDatabase&, size_t offset) {
        if (fd_.dataType.length == 4)
        {
            return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, mb_.GetFieldValue<float>(fd_, offset), std::chars_format::scientific, fd_.precision);
        }
        if (fd_.dataType.length == 8)
        {
            return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, mb_.GetFieldValue<double>(fd_, offset), std::chars_format::scientific, fd_.precision);
        }
        return false;
    };

    asciiFieldMap[CalculateBlockCrc32("le")] = asciiFieldMap[CalculateBlockCrc32("e")];

    asciiFieldMap[CalculateBlockCrc32("k")] = [](const BaseField& fd_, const MessageBody& mb_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 const MessageDatabase&, size_t offset) {
        const auto val = mb_.GetFieldValue<float>(fd_, offset);
        return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, val, FloatingPointFormat(fd_, val), fd_.precision);
    };

    asciiFieldMap[CalculateBlockCrc32("lk")] = [](const BaseField& fd_, const MessageBody& mb_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                  const MessageDatabase&, size_t offset) {
        const auto val = mb_.GetFieldValue<double>(fd_, offset);
        return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, val, FloatingPointFormat(fd_, val), fd_.precision);
    };

    asciiFieldMap[CalculateBlockCrc32("c")] = [](const BaseField& fd_, const MessageBody& mb_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 const MessageDatabase&, size_t offset) {
        if (fd_.dataType.length == 1) { return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<char>(mb_.GetFieldValue<uint8_t>(fd_, offset))); }
        if (fd_.dataType.length == 4 && fd_.dataType.name == DATA_TYPE::ULONG)
        {
            return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<char>(mb_.GetFieldValue<uint32_t>(fd_, offset)));
        }
        return false;
    };

    // =========================================================
    // Json Field Mapping
    // =========================================================
    jsonFieldMap[CalculateBlockCrc32("P")] = BasicIntMapEntry<uint8_t>();

    jsonFieldMap[CalculateBlockCrc32("T")] = [](const BaseField& fd_, const MessageBody& mb_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                const MessageDatabase&, size_t offset) {
        return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, mb_.GetFieldValue<uint32_t>(fd_, offset) / 1000.0, std::chars_format::fixed, 3);
    };

    jsonFieldMap[CalculateBlockCrc32("m")] = [](const BaseField& fd_, const MessageBody& mb_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                const MessageDatabase& pclMsgDb_, size_t offset) {
        return CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(pclMsgDb_.MsgIdToMsgName(mb_.GetFieldValue<uint32_t>(fd_, offset))),
                               '"');
    };

    jsonFieldMap[CalculateBlockCrc32("id")] = [](const BaseField& fd_, const MessageBody& mb_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 const MessageDatabase&, size_t offset) {
        const auto uiTempId = mb_.GetFieldValue<uint32_t>(fd_, offset);
        const uint16_t usSv = uiTempId & 0x0000FFFF;
        const int16_t sGloChan = (uiTempId & 0xFFFF0000) >> 16;
        return (sGloChan < 0)    ? CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', usSv, sGloChan, '"')
               : (sGloChan != 0) ? CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', usSv, '+', sGloChan, '"')
                                 : CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', usSv, '"');
    };

    jsonFieldMap[CalculateBlockCrc32("f")] = [](const BaseField& fd_, const MessageBody& mb_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                const MessageDatabase&, size_t offset) {
        if (fd_.dataType.length == 4)
        {
            return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, mb_.GetFieldValue<float>(fd_, offset), std::chars_format::fixed, fd_.precision);
        }
        if (fd_.dataType.length == 8)
        {
            return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, mb_.GetFieldValue<double>(fd_, offset), std::chars_format::fixed, fd_.precision);
        }
        return false;
    };

    jsonFieldMap[CalculateBlockCrc32("lf")] = [](const BaseField& fd_, const MessageBody& mb_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 const MessageDatabase&, size_t offset) {
        return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, mb_.GetFieldValue<double>(fd_, offset), std::chars_format::fixed, fd_.precision);
    };

    jsonFieldMap[CalculateBlockCrc32("e")] = [](const BaseField& fd_, const MessageBody& mb_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                const MessageDatabase&, size_t offset) {
        if (fd_.dataType.length == 4)
        {
            return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, mb_.GetFieldValue<float>(fd_, offset), std::chars_format::scientific, fd_.precision);
        }
        if (fd_.dataType.length == 8)
        {
            return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, mb_.GetFieldValue<double>(fd_, offset), std::chars_format::scientific, fd_.precision);
        }
        return false;
    };

    jsonFieldMap[CalculateBlockCrc32("le")] = jsonFieldMap[CalculateBlockCrc32("e")];

    jsonFieldMap[CalculateBlockCrc32("k")] = [](const BaseField& fd_, const MessageBody& mb_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                const MessageDatabase&, size_t offset) {
        const auto val = mb_.GetFieldValue<float>(fd_, offset);
        return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, val, FloatingPointFormat(fd_, val), fd_.precision);
    };

    jsonFieldMap[CalculateBlockCrc32("lk")] = [](const BaseField& fd_, const MessageBody& mb_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                 const MessageDatabase&, size_t offset) {
        const auto val = mb_.GetFieldValue<double>(fd_, offset);
        return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, val, FloatingPointFormat(fd_, val), fd_.precision);
    };

    jsonFieldMap[CalculateBlockCrc32("s")] = [](const BaseField& fd_, const MessageBody& mb_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                const MessageDatabase&, size_t offset) {
        return CopyToBuffer(ppcOutBuf_, uiBytesLeft_,
                            fd_.dataType.name == DATA_TYPE::UCHAR ? static_cast<char>(mb_.GetFieldValue<uint8_t>(fd_, offset))
                                                                  : static_cast<char>(mb_.GetFieldValue<int8_t>(fd_, offset)));
    };

    jsonFieldMap[CalculateBlockCrc32("c")] = [](const BaseField& fd_, const MessageBody& mb_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                                const MessageDatabase&, size_t offset) {
        if (fd_.dataType.length == 1)
        {
            return CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', static_cast<char>(mb_.GetFieldValue<uint8_t>(fd_, offset)), '"');
        }
        if (fd_.dataType.length == 4 && fd_.dataType.name == DATA_TYPE::ULONG)
        {
            return CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', static_cast<char>(mb_.GetFieldValue<uint32_t>(fd_, offset)), '"');
        }
        return false;
    };
}

// -------------------------------------------------------------------------------------------------------
bool Encoder::EncodeBinaryHeader(const IntermediateHeader& stInterHeader_, unsigned char** ppucOutBuf_, uint32_t& uiBytesLeft_)
{
    return CopyToBuffer(ppucOutBuf_, uiBytesLeft_, Oem4BinaryHeader(stInterHeader_));
}

// -------------------------------------------------------------------------------------------------------
bool Encoder::EncodeBinaryShortHeader(const IntermediateHeader& stInterHeader_, unsigned char** ppucOutBuf_, uint32_t& uiBytesLeft_)
{
    return CopyToBuffer(ppucOutBuf_, uiBytesLeft_, Oem4BinaryShortHeader(stInterHeader_));
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
                                    std::string_view(sMsgName),                                                                             //
                                    GetEnumString(vMyPortAddressDefinitions, stInterHeader_.uiPortAddress),                                 //
                                    stInterHeader_.usSequence,                                                                              //
                                    FloatValue<float>{static_cast<float>(stInterHeader_.ucIdleTime) * 0.500F, std::chars_format::fixed, 1}, //
                                    GetEnumString(vMyGpsTimeStatusDefinitions, stInterHeader_.uiTimeStatus),                                //
                                    stInterHeader_.usWeek,                                                                                  //
                                    FloatValue<double>{stInterHeader_.dMilliseconds / 1000.0, std::chars_format::fixed, 3},                 //
                                    HexValue<uint32_t>{stInterHeader_.uiReceiverStatus, 8},                                                 //
                                    HexValue<uint32_t>{stInterHeader_.uiMessageDefinitionCrc, 4},                                           //
                                    stInterHeader_.usReceiverSwVersion                                                                      //
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
                                    std::string_view(sMsgName),                                                                             //
                                    GetEnumString(vMyPortAddressDefinitions, stInterHeader_.uiPortAddress),                                 //
                                    stInterHeader_.usSequence,                                                                              //
                                    FloatValue<float>{static_cast<float>(stInterHeader_.ucIdleTime) * 0.500F, std::chars_format::fixed, 1}, //
                                    GetEnumString(vMyGpsTimeStatusDefinitions, stInterHeader_.uiTimeStatus),                                //
                                    stInterHeader_.usWeek,                                                                                  //
                                    FloatValue<double>{stInterHeader_.dMilliseconds / 1000.0, std::chars_format::fixed, 3},                 //
                                    HexValue<uint32_t>{stInterHeader_.uiReceiverStatus, 8},                                                 //
                                    HexValue<uint32_t>{stInterHeader_.uiMessageDefinitionCrc, 4},                                           //
                                    stInterHeader_.usReceiverSwVersion                                                                      //
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

    return CopyAllToBufferSeparated(ppcOutBuf_, uiBytesLeft_, OEM4_ASCII_FIELD_SEPARATOR,                                  //
                                    std::string_view(sMsgName),                                                            //
                                    stInterHeader_.usWeek,                                                                 //
                                    FloatValue<double>{stInterHeader_.dMilliseconds / 1000.0, std::chars_format::fixed, 3} //
                                    ) &&
           CopyToBuffer(ppcOutBuf_, uiBytesLeft_, OEM4_ASCII_HEADER_TERMINATOR);
}

// -------------------------------------------------------------------------------------------------------
bool Encoder::EncodeAbbrevAsciiShortHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
{
    if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, OEM4_ABBREV_ASCII_SYNC)) { return false; }

    std::string sMsgName(pclMyMsgDb->GetMsgDef(stInterHeader_.usMessageId)->name);
    AppendSiblingId(sMsgName, stInterHeader_);

    return CopyAllToBufferSeparated(ppcOutBuf_, uiBytesLeft_, OEM4_ABBREV_ASCII_SEPARATOR,                                 //
                                    std::string_view(sMsgName),                                                            //
                                    stInterHeader_.usWeek,                                                                 //
                                    FloatValue<double>{stInterHeader_.dMilliseconds / 1000.0, std::chars_format::fixed, 3} //
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
    return CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_,                                                                //
                           R"({"message": ")", JsonHeaderToMsgName(stInterHeader_).c_str(),                         //
                           R"(","id": )", stInterHeader_.usMessageId,                                               //
                           R"(,"port": ")", GetEnumString(vMyPortAddressDefinitions, stInterHeader_.uiPortAddress), //
                           R"(","sequence_num": )", stInterHeader_.usSequence,                                      //
                           R"(,"percent_idle_time": )",
                           FloatValue<double>{static_cast<double>(stInterHeader_.ucIdleTime) * 0.500, std::chars_format::fixed, 1},     //
                           R"(,"time_status": ")", GetEnumString(vMyGpsTimeStatusDefinitions, stInterHeader_.uiTimeStatus),             //
                           R"(","week": )", stInterHeader_.usWeek,                                                                      //
                           R"(,"seconds": )", FloatValue<double>{(stInterHeader_.dMilliseconds / 1000.0), std::chars_format::fixed, 3}, //
                           R"(,"receiver_status": )", stInterHeader_.uiReceiverStatus,                                                  //
                           R"(,"HEADER_reserved1": )", stInterHeader_.uiMessageDefinitionCrc,                                           //
                           R"(,"receiver_sw_version": )", stInterHeader_.usReceiverSwVersion, '}');
}

// -------------------------------------------------------------------------------------------------------
bool Encoder::EncodeJsonShortHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
{
    return CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_,                                                                                    //
                           R"({"message": ")", JsonHeaderToMsgName(stInterHeader_).c_str(),                                             //
                           R"(","id": )", stInterHeader_.usMessageId,                                                                   //
                           R"(,"week": )", stInterHeader_.usWeek,                                                                       //
                           R"(,"seconds": )", FloatValue<double>{(stInterHeader_.dMilliseconds / 1000.0), std::chars_format::fixed, 3}, //
                           '}');
}

// -------------------------------------------------------------------------------------------------------
STATUS
Encoder::Encode(unsigned char* const* ppucBuffer_, uint32_t uiBufferSize_, const IntermediateHeader& stHeader_, const MessageBody& stMessage_,
                MessageDataStruct& stMessageData_, HEADER_FORMAT eHeaderFormat_, ENCODE_FORMAT eFormat_) const
{
    if (ppucBuffer_ == nullptr || *ppucBuffer_ == nullptr) { return STATUS::NULL_PROVIDED; }

    if (pclMyMsgDb == nullptr) { return STATUS::NO_DATABASE; }

    unsigned char* pucTempEncodeBuffer = *ppucBuffer_;

    if (stMessage_.GetFieldInfo() == nullptr) { return STATUS::NO_DEFINITION; }

    const auto& fieldDefinitions = stMessage_.GetFieldInfo()->messageOrderedFields;

    if (eFormat_ == ENCODE_FORMAT::JSON)
    {
        if (!CopyToBuffer(&pucTempEncodeBuffer, uiBufferSize_, R"({"header": )")) { return STATUS::BUFFER_FULL; }
    }

    if (eFormat_ == ENCODE_FORMAT::ABBREV_ASCII && ((stHeader_.ucMessageType & static_cast<uint8_t>(MESSAGE_TYPE_MASK::RESPONSE)) != 0))
    {
        if (!CopyToBuffer(&pucTempEncodeBuffer, uiBufferSize_, "<")) { return STATUS::BUFFER_FULL; }
        stMessageData_.uiMessageHeaderLength = 1;

        if (fieldDefinitions.size() <= 1) { return STATUS::MALFORMED_INPUT; }
        auto sResponse = stMessage_.GetFieldValue<std::string>(*fieldDefinitions.at(1));
        if (!CopyToBuffer(&pucTempEncodeBuffer, uiBufferSize_, sResponse.c_str())) { return STATUS::BUFFER_FULL; }
        if (!CopyToBuffer(&pucTempEncodeBuffer, uiBufferSize_, "\r\n")) { return STATUS::BUFFER_FULL; }
        stMessageData_.pucMessage = *ppucBuffer_;
        stMessageData_.uiMessageLength = pucTempEncodeBuffer - *ppucBuffer_;
        stMessageData_.uiMessageBodyLength = stMessageData_.uiMessageLength - stMessageData_.uiMessageHeaderLength;

        return STATUS::SUCCESS;
    }

    STATUS eStatus = EncodeHeader(&pucTempEncodeBuffer, uiBufferSize_, stHeader_, stMessageData_, eHeaderFormat_, eFormat_);
    pucTempEncodeBuffer += stMessageData_.uiMessageHeaderLength;
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    if (eFormat_ == ENCODE_FORMAT::JSON)
    {
        if (!CopyToBuffer(&pucTempEncodeBuffer, uiBufferSize_, R"(,"body": )")) { return STATUS::BUFFER_FULL; }
    }

    eStatus = EncodeBody(&pucTempEncodeBuffer, uiBufferSize_, stMessage_, fieldDefinitions, stMessageData_, eHeaderFormat_, eFormat_);
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
Encoder::EncodeHeader(unsigned char* const* ppucBuffer_, uint32_t uiBufferSize_, const IntermediateHeader& stHeader_,
                      MessageDataStruct& stMessageData_, const HEADER_FORMAT eHeaderFormat_, const ENCODE_FORMAT eFormat_,
                      const bool bIsEmbeddedHeader_) const
{
    if (ppucBuffer_ == nullptr || *ppucBuffer_ == nullptr) { return STATUS::NULL_PROVIDED; }

    if (pclMyMsgDb == nullptr) { return STATUS::NO_DATABASE; }

    unsigned char* pucTempBuffer = *ppucBuffer_;

    switch (eFormat_)
    {
    case ENCODE_FORMAT::ASCII: {
        auto* pcTempBuffer = reinterpret_cast<char*>(pucTempBuffer);
        if (IsShortHeaderFormat(eHeaderFormat_) ? !EncodeAsciiShortHeader(stHeader_, &pcTempBuffer, uiBufferSize_)
                                                : !EncodeAsciiHeader(stHeader_, &pcTempBuffer, uiBufferSize_))
        {
            return STATUS::BUFFER_FULL;
        }
        pucTempBuffer = reinterpret_cast<unsigned char*>(pcTempBuffer);
        break;
    }
    case ENCODE_FORMAT::ABBREV_ASCII: {
        auto* pcTempBuffer = reinterpret_cast<char*>(pucTempBuffer);
        if (IsShortHeaderFormat(eHeaderFormat_) ? !EncodeAbbrevAsciiShortHeader(stHeader_, &pcTempBuffer, uiBufferSize_)
                                                : !EncodeAbbrevAsciiHeader(stHeader_, &pcTempBuffer, uiBufferSize_, bIsEmbeddedHeader_))
        {
            return STATUS::BUFFER_FULL;
        }
        pucTempBuffer = reinterpret_cast<unsigned char*>(pcTempBuffer);
        break;
    }
    case ENCODE_FORMAT::FLATTENED_BINARY: [[fallthrough]];
    case ENCODE_FORMAT::BINARY:
        if (IsShortHeaderFormat(eHeaderFormat_) ? !EncodeBinaryShortHeader(stHeader_, &pucTempBuffer, uiBufferSize_)
                                                : !EncodeBinaryHeader(stHeader_, &pucTempBuffer, uiBufferSize_))
        {
            return STATUS::BUFFER_FULL;
        }
        break;
    case ENCODE_FORMAT::JSON: {
        auto* pcTempBuffer = reinterpret_cast<char*>(pucTempBuffer);
        if (IsShortHeaderFormat(eHeaderFormat_) ? !EncodeJsonShortHeader(stHeader_, &pcTempBuffer, uiBufferSize_)
                                                : !EncodeJsonHeader(stHeader_, &pcTempBuffer, uiBufferSize_))
        {
            return STATUS::BUFFER_FULL;
        }
        pucTempBuffer = reinterpret_cast<unsigned char*>(pcTempBuffer);
        break;
    }
    default: return STATUS::UNSUPPORTED;
    }

    // Record the length of the encoded message header.
    stMessageData_.pucMessageHeader = *ppucBuffer_;
    stMessageData_.uiMessageHeaderLength = pucTempBuffer - stMessageData_.pucMessageHeader;
    return STATUS::SUCCESS;
}

// -------------------------------------------------------------------------------------------------------
STATUS
Encoder::EncodeBody(unsigned char* const* ppucBuffer_, uint32_t uiBufferSize_, const MessageBody& stMessage_,
                    const std::vector<BaseField::ConstPtr>& fieldDefinitions, MessageDataStruct& stMessageData_, const HEADER_FORMAT eHeaderFormat_,
                    ENCODE_FORMAT eFormat_) const
{
    // TODO: this entire function should be in common, only header stuff and map redefinitions belong in this file
    if (ppucBuffer_ == nullptr || *ppucBuffer_ == nullptr) { return STATUS::NULL_PROVIDED; }

    if (pclMyMsgDb == nullptr) { return STATUS::NO_DATABASE; }

    auto* pucTempBuffer = *ppucBuffer_;

    switch (eFormat_)
    {
    case ENCODE_FORMAT::ASCII: {
        auto* pcTempBuffer = reinterpret_cast<char*>(pucTempBuffer);
        if (!EncodeAsciiBody<false>(stMessage_, fieldDefinitions, &pcTempBuffer, uiBufferSize_)) { return STATUS::BUFFER_FULL; }
        pcTempBuffer--; // Remove last delimiter ','
        const uint32_t uiCrc = CalculateBlockCrc32(stMessageData_.pucMessageHeader + 1,
                                                   reinterpret_cast<unsigned char*>(pcTempBuffer) - stMessageData_.pucMessageHeader - 1);
        if (!CopyAllToBuffer(&pcTempBuffer, uiBufferSize_, '*', HexValue<uint32_t>{uiCrc, 8}, "\r\n")) { return STATUS::BUFFER_FULL; }
        pucTempBuffer = reinterpret_cast<unsigned char*>(pcTempBuffer);
        break;
    }
    case ENCODE_FORMAT::ABBREV_ASCII: {
        auto* pcTempBuffer = reinterpret_cast<char*>(pucTempBuffer);
        if (!EncodeAsciiBody<true>(stMessage_, fieldDefinitions, &pcTempBuffer, uiBufferSize_)) { return STATUS::BUFFER_FULL; }
        pcTempBuffer--; // Remove last delimiter ' '
        if (!CopyToBuffer(&pcTempBuffer, uiBufferSize_, "\r\n")) { return STATUS::BUFFER_FULL; }
        pucTempBuffer = reinterpret_cast<unsigned char*>(pcTempBuffer);
        break;
    }
    case ENCODE_FORMAT::FLATTENED_BINARY:
        if (!EncodeBinaryBody<true>(stMessage_, fieldDefinitions, &pucTempBuffer, uiBufferSize_)) { return STATUS::BUFFER_FULL; }
        [[fallthrough]];

    case ENCODE_FORMAT::BINARY: {
        if (eFormat_ == ENCODE_FORMAT::BINARY && !EncodeBinaryBody<false>(stMessage_, fieldDefinitions, &pucTempBuffer, uiBufferSize_))
        {
            return STATUS::BUFFER_FULL;
        }
        // MessageData must have a valid MessageHeader pointer to populate the length field.
        if (stMessageData_.pucMessageHeader == nullptr) { return STATUS::FAILURE; }
        // Go back and set the length field in the header.
        // TODO: this block of code below is what's blocking us from moving this function to common (can solve this with dynamic casting or CRTP?)
        if (!IsShortHeaderFormat(eHeaderFormat_))
        {
            auto usLength = static_cast<uint16_t>(pucTempBuffer - *ppucBuffer_);
            std::memcpy(stMessageData_.pucMessageHeader + offsetof(Oem4BinaryHeader, usLength), &usLength, sizeof(usLength));
        }
        else
        {
            auto ucLength = static_cast<uint8_t>(pucTempBuffer - *ppucBuffer_);
            std::memcpy(stMessageData_.pucMessageHeader + offsetof(Oem4BinaryShortHeader, ucLength), &ucLength, sizeof(ucLength));
        }
        if (!CopyToBuffer(&pucTempBuffer, uiBufferSize_,
                          CalculateBlockCrc32(stMessageData_.pucMessageHeader, pucTempBuffer - stMessageData_.pucMessageHeader)))
        {
            return STATUS::BUFFER_FULL;
        }
        break;
    }
    case ENCODE_FORMAT::JSON: {
        auto* pcTempBuffer = reinterpret_cast<char*>(pucTempBuffer);
        if (!EncodeJsonBody(stMessage_, fieldDefinitions, &pcTempBuffer, uiBufferSize_)) { return STATUS::BUFFER_FULL; }
        pucTempBuffer = reinterpret_cast<unsigned char*>(pcTempBuffer);
        break;
    }

    default: return STATUS::UNSUPPORTED;
    }

    // Record the length of the encoded message payload.
    stMessageData_.pucMessageBody = *ppucBuffer_;
    stMessageData_.uiMessageBodyLength = pucTempBuffer - stMessageData_.pucMessageBody;
    return STATUS::SUCCESS;
}
