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
// ! \file header_decoder.cpp
// ===============================================================================

#include "novatel_edie/decoders/oem/header_decoder.hpp"

#include <bitset>

using namespace novatel::edie;
using namespace novatel::edie::oem;

// -------------------------------------------------------------------------------------------------------
HeaderDecoder::HeaderDecoder(JsonReader* pclJsonDb_)
{
    pclMyLogger->debug("HeaderDecoder initializing...");

    if (pclJsonDb_ != nullptr) { LoadJsonDb(pclJsonDb_); }
    pclMyLogger->debug("HeaderDecoder initialized");
}

// -------------------------------------------------------------------------------------------------------
void HeaderDecoder::LoadJsonDb(JsonReader* pclJsonDb_)
{
    pclMyMsgDb = pclJsonDb_;

    vMyCommandDefinitions = pclMyMsgDb->GetEnumDefName("Commands");
    vMyPortAddressDefinitions = pclMyMsgDb->GetEnumDefName("PortAddress");
    vMyGpsTimeStatusDefinitions = pclMyMsgDb->GetEnumDefName("GPSTimeStatus");
}

// -------------------------------------------------------------------------------------------------------
void HeaderDecoder::SetLoggerLevel(spdlog::level::level_enum eLevel_) const { pclMyLogger->set_level(eLevel_); }

// -------------------------------------------------------------------------------------------------------
std::shared_ptr<spdlog::logger> HeaderDecoder::GetLogger() { return pclMyLogger; }

// -------------------------------------------------------------------------------------------------------
template <AsciiHeader eField> bool HeaderDecoder::DecodeAsciiHeaderField(IntermediateHeader& stInterHeader_, char** ppcLogBuf_) const
{
    constexpr bool bIsLeadingNumberField = eField == AsciiHeader::SEQUENCE || eField == AsciiHeader::IDLE_TIME || eField == AsciiHeader::WEEK ||
                                           eField == AsciiHeader::SECONDS || eField == AsciiHeader::RECEIVER_SW_VERSION;

    constexpr bool bIsLeadingAlphaField = eField == AsciiHeader::MESSAGE_NAME || eField == AsciiHeader::PORT || eField == AsciiHeader::TimeStatus;

    constexpr bool bIsLeadingHexField = eField == AsciiHeader::RECEIVER_STATUS || eField == AsciiHeader::MSG_DEF_CRC;

    static_assert(bIsLeadingNumberField || bIsLeadingAlphaField || bIsLeadingHexField);

    // We check if the first character in the field is in the valid format
    if ((bIsLeadingNumberField && !isdigit(**ppcLogBuf_)) || (bIsLeadingAlphaField && !isalpha(**ppcLogBuf_)) ||
        (bIsLeadingHexField && !isxdigit(**ppcLogBuf_)))
    {
        return false;
    }

    const size_t ullTokenLength = strcspn(*ppcLogBuf_, " ,;\r");

    switch (eField)
    {
    case AsciiHeader::MESSAGE_NAME: {
        uint16_t usLogId = 0;
        uint32_t uiSiblingId = 0;
        uint32_t uiMsgFormat = 0;
        uint32_t uiResponse = 0;
        UnpackMsgId(pclMyMsgDb->MsgNameToMsgId(std::string(*ppcLogBuf_, ullTokenLength)), usLogId, uiSiblingId, uiMsgFormat, uiResponse);
        stInterHeader_.usMessageId = usLogId;
        stInterHeader_.ucMessageType = PackMsgType(uiSiblingId, uiMsgFormat, uiResponse);
        break;
    }
    case AsciiHeader::PORT:
        stInterHeader_.uiPortAddress = static_cast<uint32_t>(GetEnumValue(vMyPortAddressDefinitions, std::string(*ppcLogBuf_, ullTokenLength)));
        break;
    case AsciiHeader::SEQUENCE: stInterHeader_.usSequence = static_cast<uint16_t>(strtoul(*ppcLogBuf_, nullptr, 10)); break;
    case AsciiHeader::IDLE_TIME: stInterHeader_.ucIdleTime = static_cast<uint8_t>(std::lround(2.0F * strtof(*ppcLogBuf_, nullptr))); break;
    case AsciiHeader::TimeStatus:
        stInterHeader_.uiTimeStatus = GetEnumValue(vMyGpsTimeStatusDefinitions, std::string(*ppcLogBuf_, ullTokenLength));
        break;
    case AsciiHeader::WEEK: stInterHeader_.usWeek = static_cast<uint16_t>(strtoul(*ppcLogBuf_, nullptr, 10)); break;
    case AsciiHeader::SECONDS: stInterHeader_.dMilliseconds = 1000.0 * strtod(*ppcLogBuf_, nullptr); break;
    case AsciiHeader::RECEIVER_STATUS: stInterHeader_.uiReceiverStatus = strtoul(*ppcLogBuf_, nullptr, 16); break;
    case AsciiHeader::MSG_DEF_CRC: stInterHeader_.uiMessageDefinitionCrc = strtoul(*ppcLogBuf_, nullptr, 16); break;
    case AsciiHeader::RECEIVER_SW_VERSION: stInterHeader_.usReceiverSwVersion = static_cast<uint16_t>(strtoul(*ppcLogBuf_, nullptr, 10)); break;
    default: return false;
    }
    *ppcLogBuf_ += ullTokenLength + 1; // Consume the token and the trailing delimiter
    return true;
}

// -------------------------------------------------------------------------------------------------------
template <AsciiHeader... eFields> bool HeaderDecoder::DecodeAsciiHeaderFields(IntermediateHeader& stInterHeader_, char** ppcLogBuf_) const
{
    return (DecodeAsciiHeaderField<eFields>(stInterHeader_, ppcLogBuf_) && ...);
}

// -------------------------------------------------------------------------------------------------------
void HeaderDecoder::DecodeJsonHeader(json clJsonHeader_, IntermediateHeader& stInterHeader_) const
{
    stInterHeader_.usMessageId = clJsonHeader_["id"].get<uint16_t>();
    stInterHeader_.uiPortAddress = static_cast<uint32_t>(GetEnumValue(vMyPortAddressDefinitions, clJsonHeader_["port"].get<std::string>()));
    stInterHeader_.usSequence = clJsonHeader_["sequence_num"].get<uint16_t>();
    stInterHeader_.ucIdleTime = static_cast<uint8_t>(clJsonHeader_["percent_idle_time"].get<float>() * 2.0);
    stInterHeader_.uiTimeStatus = static_cast<uint32_t>(GetEnumValue(vMyGpsTimeStatusDefinitions, clJsonHeader_["time_status"].get<std::string>()));
    stInterHeader_.usWeek = clJsonHeader_["week"].get<uint16_t>();
    stInterHeader_.dMilliseconds = clJsonHeader_["seconds"].get<double>() * 1000.0;
    stInterHeader_.uiReceiverStatus = clJsonHeader_["receiver_status"].get<uint32_t>();
    stInterHeader_.usReceiverSwVersion = clJsonHeader_["receiver_sw_version"].get<uint16_t>();
    stInterHeader_.uiMessageDefinitionCrc = clJsonHeader_["HEADER_reserved1"].get<uint32_t>();
}

// -------------------------------------------------------------------------------------------------------
Status HeaderDecoder::Decode(unsigned char* pucLogBuf_, IntermediateHeader& stInterHeader_, MetaDataStruct& stMetaData_) const
{
    if (pucLogBuf_ == nullptr) { return Status::NULL_PROVIDED; }

    if (pclMyMsgDb == nullptr) { return Status::NO_DATABASE; }

    auto* pcTempBuf = reinterpret_cast<char*>(pucLogBuf_);
    auto* pstBinaryHeader = reinterpret_cast<Oem4BinaryHeader*>(pucLogBuf_);

    stMetaData_.eFormat = pstBinaryHeader->ucSync1 == OEM4_ASCII_SYNC                                                      ? HeaderFormat::ASCII
                          : pstBinaryHeader->ucSync1 == OEM4_SHORT_ASCII_SYNC                                              ? HeaderFormat::SHORT_ASCII
                          : pstBinaryHeader->ucSync1 == OEM4_ABBREV_ASCII_SYNC                                             ? HeaderFormat::ABB_ASCII
                          : pstBinaryHeader->ucSync1 == NMEA_SYNC                                                          ? HeaderFormat::NMEA
                          : pstBinaryHeader->ucSync1 == '{'                                                                ? HeaderFormat::JSON
                          : pstBinaryHeader->ucSync1 == OEM4_BINARY_SYNC1 && pstBinaryHeader->ucSync3 == OEM4_BINARY_SYNC3 ? HeaderFormat::BINARY
                          : pstBinaryHeader->ucSync1 == OEM4_BINARY_SYNC1 && pstBinaryHeader->ucSync3 == OEM4_SHORT_BINARY_SYNC3
                              ? HeaderFormat::SHORT_BINARY
                              : HeaderFormat::UNKNOWN;

    switch (stMetaData_.eFormat)
    {
    case HeaderFormat::ASCII:
        ++pcTempBuf; // Move the input buffer past the sync char '#'
        if (!DecodeAsciiHeaderFields<AsciiHeader::MESSAGE_NAME, AsciiHeader::PORT, AsciiHeader::SEQUENCE, AsciiHeader::IDLE_TIME,
                                     AsciiHeader::TimeStatus, AsciiHeader::WEEK, AsciiHeader::SECONDS, AsciiHeader::RECEIVER_STATUS,
                                     AsciiHeader::MSG_DEF_CRC, AsciiHeader::RECEIVER_SW_VERSION>(stInterHeader_, &pcTempBuf))
        {
            return Status::FAILURE;
        }
        break;

    case HeaderFormat::ABB_ASCII:
        ++pcTempBuf; // Move the input buffer past the sync char '<'
        // At this point, we do not know if the format is short or not, but both have a message
        // field
        if (!DecodeAsciiHeaderFields<AsciiHeader::MESSAGE_NAME>(stInterHeader_, &pcTempBuf)) { return Status::FAILURE; }
        if (DecodeAsciiHeaderFields<AsciiHeader::PORT>(stInterHeader_, &pcTempBuf))
        {
            // Port field succeeded, so this is not short format
            if (!DecodeAsciiHeaderFields<AsciiHeader::SEQUENCE, AsciiHeader::IDLE_TIME, AsciiHeader::TimeStatus, AsciiHeader::WEEK,
                                         AsciiHeader::SECONDS, AsciiHeader::RECEIVER_STATUS, AsciiHeader::MSG_DEF_CRC,
                                         AsciiHeader::RECEIVER_SW_VERSION>(stInterHeader_, &pcTempBuf))
            {
                return Status::FAILURE;
            }
        }
        else
        {
            // Port field failed, so we (unsafely) assume this is short
            stMetaData_.eFormat = HeaderFormat::SHORT_ABB_ASCII;
            if (!DecodeAsciiHeaderFields<AsciiHeader::WEEK, AsciiHeader::SECONDS>(stInterHeader_, &pcTempBuf)) { return Status::FAILURE; }
        }
        ++pcTempBuf; // Move the input buffer past the trailing delimiter '\n'
        break;

    case HeaderFormat::SHORT_ASCII:
        ++pcTempBuf; // Move the input buffer past the sync char '%'
        if (!DecodeAsciiHeaderFields<AsciiHeader::MESSAGE_NAME, AsciiHeader::WEEK, AsciiHeader::SECONDS>(stInterHeader_, &pcTempBuf))
        {
            return Status::FAILURE;
        }
        break;

    case HeaderFormat::BINARY:
        stInterHeader_.usMessageId = pstBinaryHeader->usMsgNumber;
        stInterHeader_.ucMessageType = pstBinaryHeader->ucMsgType;
        stInterHeader_.uiPortAddress = pstBinaryHeader->ucPort;
        stInterHeader_.usLength = pstBinaryHeader->usLength;
        stInterHeader_.usSequence = pstBinaryHeader->usSequenceNumber;
        stInterHeader_.ucIdleTime = pstBinaryHeader->ucIdleTime;
        stInterHeader_.uiTimeStatus = pstBinaryHeader->ucTimeStatus;
        stInterHeader_.usWeek = pstBinaryHeader->usWeekNo;
        stInterHeader_.dMilliseconds = pstBinaryHeader->uiWeekMSec;
        stInterHeader_.uiReceiverStatus = pstBinaryHeader->uiStatus;
        stInterHeader_.usReceiverSwVersion = pstBinaryHeader->usReceiverSwVersion;
        stInterHeader_.uiMessageDefinitionCrc = pstBinaryHeader->usMsgDefCrc;
        pcTempBuf += sizeof(Oem4BinaryHeader);
        break;

    case HeaderFormat::SHORT_BINARY: {
        const auto* pstBinaryShortHeader = reinterpret_cast<Oem4BinaryShortHeader*>(pucLogBuf_);
        stInterHeader_.usLength = pstBinaryShortHeader->ucLength;
        stInterHeader_.usMessageId = pstBinaryShortHeader->usMessageId;
        stInterHeader_.usWeek = pstBinaryShortHeader->usWeekNo;
        stInterHeader_.dMilliseconds = pstBinaryShortHeader->uiWeekMSec;
        pcTempBuf += sizeof(Oem4BinaryShortHeader);
        break;
    }
    case HeaderFormat::JSON:
        try
        {
            DecodeJsonHeader(json::parse(pcTempBuf)["header"], stInterHeader_);
        }
        catch (std::exception& e)
        {
            SPDLOG_LOGGER_CRITICAL(pclMyLogger, e.what());
            return Status::FAILURE;
        }
        break;

    case HeaderFormat::NMEA: return Status::UNSUPPORTED;

    default: return Status::UNKNOWN;
    }

    stMetaData_.eMeasurementSource = static_cast<MeasurementSource>(stInterHeader_.ucMessageType & static_cast<uint32_t>(MessageTypeMask::MEASSRC));
    stMetaData_.eTimeStatus = static_cast<TimeStatus>(stInterHeader_.uiTimeStatus);
    stMetaData_.bResponse =
        static_cast<uint32_t>(MessageTypeMask::RESPONSE) == (stInterHeader_.ucMessageType & static_cast<uint32_t>(MessageTypeMask::RESPONSE));
    stMetaData_.usWeek = static_cast<uint32_t>(stInterHeader_.usWeek);
    stMetaData_.dMilliseconds = static_cast<uint32_t>(stInterHeader_.dMilliseconds);
    stMetaData_.usMessageId = static_cast<uint32_t>(stInterHeader_.usMessageId);
    stMetaData_.uiMessageCrc = stInterHeader_.uiMessageDefinitionCrc;
    stMetaData_.uiHeaderLength = static_cast<uint32_t>(pcTempBuf - reinterpret_cast<char*>(pucLogBuf_));
    stMetaData_.uiBinaryMsgLength = static_cast<uint32_t>(stInterHeader_.usLength);

    // Reconstruct a message name that won't have a suffix of any kind.
    stMetaData_.MessageName(pclMyMsgDb->MsgIdToMsgName(CreateMsgId(stInterHeader_.usMessageId, static_cast<uint32_t>(MeasurementSource::PRIMARY),
                                                                   static_cast<uint32_t>(MessageFormat::ABBREV), 0U)));

    return Status::SUCCESS;
}
