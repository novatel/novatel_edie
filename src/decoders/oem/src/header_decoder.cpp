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

#include <nlohmann/json.hpp>

using namespace novatel::edie;
using namespace novatel::edie::oem;

using json = nlohmann::json;

// -------------------------------------------------------------------------------------------------------
HeaderDecoder::HeaderDecoder(MessageDatabase::Ptr pclMessageDb_)
{
    pclMyLogger->debug("HeaderDecoder initializing...");

    if (pclMessageDb_ != nullptr) { LoadJsonDb(pclMessageDb_); }
    pclMyLogger->debug("HeaderDecoder initialized");
}

// -------------------------------------------------------------------------------------------------------
void HeaderDecoder::LoadJsonDb(MessageDatabase::Ptr pclMessageDb_)
{
    pclMyMsgDb = pclMessageDb_;

    vMyCommandDefinitions = pclMyMsgDb->GetEnumDefName("Commands");
    vMyPortAddressDefinitions = pclMyMsgDb->GetEnumDefName("PortAddress");
    vMyGpsTimeStatusDefinitions = pclMyMsgDb->GetEnumDefName("GPSTimeStatus");
}

// cons
template <size_t N> constexpr size_t constexpr_strlen(const char (&)[N])
{
    return N - 1; // Subtract 1 to exclude the null terminator
}

// -------------------------------------------------------------------------------------------------------
template <const char pcDelimiter[], size_t ullDelimiterSize, ASCII_HEADER eField>
bool HeaderDecoder::DecodeAsciiHeaderField(IntermediateHeader& stInterHeader_, const char** ppcLogBuf_) const
{
    constexpr bool bIsLeadingNumberField = eField == ASCII_HEADER::SEQUENCE || eField == ASCII_HEADER::IDLE_TIME || eField == ASCII_HEADER::WEEK ||
                                           eField == ASCII_HEADER::SECONDS || eField == ASCII_HEADER::RECEIVER_SW_VERSION;

    constexpr bool bIsLeadingAlphaField = eField == ASCII_HEADER::MESSAGE_NAME || eField == ASCII_HEADER::PORT || eField == ASCII_HEADER::TIME_STATUS;

    constexpr bool bIsLeadingHexField = eField == ASCII_HEADER::RECEIVER_STATUS || eField == ASCII_HEADER::MSG_DEF_CRC;

    static_assert(bIsLeadingNumberField || bIsLeadingAlphaField || bIsLeadingHexField);

    // We check if the first character in the field is in the valid format
    if ((bIsLeadingNumberField && !isdigit(**ppcLogBuf_)) || (bIsLeadingAlphaField && !isalpha(**ppcLogBuf_)) ||
        (bIsLeadingHexField && !isxdigit(**ppcLogBuf_)))
    {
        return false;
    }

    // Find next delimiter
    const char* pcNextDelimiter = strchr(*ppcLogBuf_, pcDelimiter[0]);
    if (pcNextDelimiter == nullptr) { return false; }

    // Check that full delimiter is correct
    if (std::strncmp(pcNextDelimiter, pcDelimiter, ullDelimiterSize) != 0) { return false; };

    // Process field value up until delimiter
    size_t ullTokenLength = pcNextDelimiter - *ppcLogBuf_;
    switch (eField)
    {
    case ASCII_HEADER::MESSAGE_NAME: {
        uint16_t usLogId = 0;
        uint32_t uiSiblingId = 0;
        uint32_t uiMsgFormat = 0;
        uint32_t uiResponse = 0;
        UnpackMsgId(pclMyMsgDb->MsgNameToMsgId(std::string(*ppcLogBuf_, ullTokenLength)), usLogId, uiSiblingId, uiMsgFormat, uiResponse);
        stInterHeader_.usMessageId = usLogId;
        stInterHeader_.ucMessageType = PackMsgType(uiSiblingId, uiMsgFormat, uiResponse);
        break;
    }
    case ASCII_HEADER::PORT:
        stInterHeader_.uiPortAddress = static_cast<uint32_t>(GetEnumValue(vMyPortAddressDefinitions, std::string(*ppcLogBuf_, ullTokenLength)));
        break;
    case ASCII_HEADER::SEQUENCE: stInterHeader_.usSequence = static_cast<uint16_t>(strtoul(*ppcLogBuf_, nullptr, 10)); break;
    case ASCII_HEADER::IDLE_TIME: stInterHeader_.ucIdleTime = static_cast<uint8_t>(std::lround(2.0F * strtof(*ppcLogBuf_, nullptr))); break;
    case ASCII_HEADER::TIME_STATUS:
        stInterHeader_.uiTimeStatus = GetEnumValue(vMyGpsTimeStatusDefinitions, std::string(*ppcLogBuf_, ullTokenLength));
        break;
    case ASCII_HEADER::WEEK: stInterHeader_.usWeek = static_cast<uint16_t>(strtoul(*ppcLogBuf_, nullptr, 10)); break;
    case ASCII_HEADER::SECONDS: stInterHeader_.dMilliseconds = 1000.0 * strtod(*ppcLogBuf_, nullptr); break;
    case ASCII_HEADER::RECEIVER_STATUS: stInterHeader_.uiReceiverStatus = strtoul(*ppcLogBuf_, nullptr, 16); break;
    case ASCII_HEADER::MSG_DEF_CRC: stInterHeader_.uiMessageDefinitionCrc = strtoul(*ppcLogBuf_, nullptr, 16); break;
    case ASCII_HEADER::RECEIVER_SW_VERSION: stInterHeader_.usReceiverSwVersion = static_cast<uint16_t>(strtoul(*ppcLogBuf_, nullptr, 10)); break;
    default: return false;
    }
    *ppcLogBuf_ = pcNextDelimiter + ullDelimiterSize; // Consume the token and the trailing delimiter
    return true;
}

// -------------------------------------------------------------------------------------------------------
template <const char pcDelimiter[], size_t ullDelimiterSize, ASCII_HEADER... eFields>
bool HeaderDecoder::DecodeAsciiHeaderFields(IntermediateHeader& stInterHeader_, const char** ppcLogBuf_) const
{
    return (DecodeAsciiHeaderField<pcDelimiter, ullDelimiterSize, eFields>(stInterHeader_, ppcLogBuf_) && ...);
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

constexpr char pcAsciiRegDelimiter[] = ",";
constexpr size_t ullAsciiRegDelimSize = sizeof(pcAsciiRegDelimiter) - 1;
constexpr char pcAsciiFinalDelimiter[] = ";";
constexpr size_t ullAsciiFinalDelimSize = sizeof(pcAsciiFinalDelimiter) - 1;
constexpr char pcAbbrevAsciiRegDelimiter[] = " ";
constexpr size_t ullAbbrevAsciiRegDelimSize = sizeof(pcAbbrevAsciiRegDelimiter) - 1;
constexpr char pcAbbrevAsciiFinalDelimiter[] = "\r\n";
constexpr size_t ullAbbrevAsciiFinalDelimSize = sizeof(pcAbbrevAsciiFinalDelimiter) - 1;
STATUS HeaderDecoder::Decode(const unsigned char* pucLogBuf_, IntermediateHeader& stInterHeader_, MetaDataStruct& stMetaData_) const
{
    if (pucLogBuf_ == nullptr) { return STATUS::NULL_PROVIDED; }

    if (pclMyMsgDb == nullptr) { return STATUS::NO_DATABASE; }

    const auto* pcTempBuf = reinterpret_cast<const char*>(pucLogBuf_);
    const auto* pstBinaryHeader = reinterpret_cast<const Oem4BinaryHeader*>(pucLogBuf_);

    stMetaData_.eFormat = [&] {
        switch (pstBinaryHeader->ucSync1)
        {
        case OEM4_ASCII_SYNC: return HEADER_FORMAT::ASCII;
        case OEM4_SHORT_ASCII_SYNC: return HEADER_FORMAT::SHORT_ASCII;
        case OEM4_ABBREV_ASCII_SYNC: return HEADER_FORMAT::ABB_ASCII;
        case NMEA_SYNC: return HEADER_FORMAT::NMEA;
        case '{': return HEADER_FORMAT::JSON;
        case OEM4_BINARY_SYNC1:
            switch (pstBinaryHeader->ucSync3)
            {
            case OEM4_BINARY_SYNC3: return HEADER_FORMAT::BINARY;
            case OEM4_SHORT_BINARY_SYNC3: return HEADER_FORMAT::SHORT_BINARY;
            default: return HEADER_FORMAT::UNKNOWN;
            }
        default: return HEADER_FORMAT::UNKNOWN;
        }
    }();

    switch (stMetaData_.eFormat)
    {
    case HEADER_FORMAT::ASCII:
        ++pcTempBuf; // Move the input buffer past the sync char '#'
        if (!DecodeAsciiHeaderFields<pcAsciiRegDelimiter, ullAsciiRegDelimSize, ASCII_HEADER::MESSAGE_NAME, ASCII_HEADER::PORT,
                                     ASCII_HEADER::SEQUENCE, ASCII_HEADER::IDLE_TIME, ASCII_HEADER::TIME_STATUS, ASCII_HEADER::WEEK,
                                     ASCII_HEADER::SECONDS, ASCII_HEADER::RECEIVER_STATUS, ASCII_HEADER::MSG_DEF_CRC>(stInterHeader_, &pcTempBuf))
        {
            return STATUS::FAILURE;
        }
        if (!DecodeAsciiHeaderField<pcAsciiFinalDelimiter, ullAsciiFinalDelimSize, ASCII_HEADER::RECEIVER_SW_VERSION>(stInterHeader_, &pcTempBuf))
        {
            return STATUS::FAILURE;
        }
        break;

    case HEADER_FORMAT::ABB_ASCII:
        ++pcTempBuf; // Move the input buffer past the sync char '<'
        // At this point, we do not know if the format is short or not, but both have a message
        // field
        if (!DecodeAsciiHeaderField<pcAbbrevAsciiRegDelimiter, ullAbbrevAsciiRegDelimSize, ASCII_HEADER::MESSAGE_NAME>(stInterHeader_, &pcTempBuf))
        {
            return STATUS::FAILURE;
        }
        if (DecodeAsciiHeaderField<pcAbbrevAsciiRegDelimiter, ullAbbrevAsciiRegDelimSize, ASCII_HEADER::PORT>(stInterHeader_, &pcTempBuf))
        {
            // Port field succeeded, so this is not short format
            if (!DecodeAsciiHeaderFields<pcAbbrevAsciiRegDelimiter, ullAbbrevAsciiRegDelimSize, ASCII_HEADER::SEQUENCE, ASCII_HEADER::IDLE_TIME,
                                         ASCII_HEADER::TIME_STATUS,
                                         ASCII_HEADER::WEEK, ASCII_HEADER::SECONDS, ASCII_HEADER::RECEIVER_STATUS, ASCII_HEADER::MSG_DEF_CRC>(
                    stInterHeader_, &pcTempBuf) ||
                !DecodeAsciiHeaderField<pcAbbrevAsciiFinalDelimiter, ullAbbrevAsciiFinalDelimSize, ASCII_HEADER::RECEIVER_SW_VERSION>(stInterHeader_,
                                                                                                                                  &pcTempBuf))
            {
                return STATUS::FAILURE;
            }
        }
        else
        {
            // Port field failed, so we (unsafely) assume this is short
            stMetaData_.eFormat = HEADER_FORMAT::SHORT_ABB_ASCII;
            if (!DecodeAsciiHeaderField<pcAbbrevAsciiRegDelimiter, ullAbbrevAsciiRegDelimSize, ASCII_HEADER::WEEK>(stInterHeader_, &pcTempBuf) ||
                !DecodeAsciiHeaderField<pcAbbrevAsciiFinalDelimiter, ullAbbrevAsciiFinalDelimSize, ASCII_HEADER::SECONDS>(stInterHeader_, &pcTempBuf))
            {
                return STATUS::FAILURE;
            }
        }
        break;

    case HEADER_FORMAT::SHORT_ASCII:
        ++pcTempBuf; // Move the input buffer past the sync char '%'
        if (!DecodeAsciiHeaderFields<pcAsciiRegDelimiter, ullAsciiRegDelimSize, ASCII_HEADER::MESSAGE_NAME, ASCII_HEADER::WEEK>(stInterHeader_,
                                                                                                                                     &pcTempBuf) ||
            !DecodeAsciiHeaderField<pcAsciiFinalDelimiter, ullAsciiFinalDelimSize, ASCII_HEADER::SECONDS>(stInterHeader_, &pcTempBuf))
        {
            return STATUS::FAILURE;
        }
        break;

    case HEADER_FORMAT::BINARY:
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

    case HEADER_FORMAT::SHORT_BINARY: {
        const auto* pstBinaryShortHeader = reinterpret_cast<const Oem4BinaryShortHeader*>(pucLogBuf_);
        stInterHeader_.usLength = pstBinaryShortHeader->ucLength;
        stInterHeader_.usMessageId = pstBinaryShortHeader->usMessageId;
        stInterHeader_.usWeek = pstBinaryShortHeader->usWeekNo;
        stInterHeader_.dMilliseconds = pstBinaryShortHeader->uiWeekMSec;
        pcTempBuf += sizeof(Oem4BinaryShortHeader);
        break;
    }
    case HEADER_FORMAT::JSON:
        try
        {
            DecodeJsonHeader(json::parse(pcTempBuf)["header"], stInterHeader_);
        }
        catch (std::exception& e)
        {
            SPDLOG_LOGGER_CRITICAL(pclMyLogger, e.what());
            return STATUS::FAILURE;
        }
        break;

    case HEADER_FORMAT::NMEA: return STATUS::UNSUPPORTED;

    default: return STATUS::UNKNOWN;
    }

    stMetaData_.eMeasurementSource =
        static_cast<MEASUREMENT_SOURCE>(stInterHeader_.ucMessageType & static_cast<uint32_t>(MESSAGE_TYPE_MASK::MEASSRC));
    stMetaData_.eTimeStatus = static_cast<TIME_STATUS>(stInterHeader_.uiTimeStatus);
    stMetaData_.bResponse =
        static_cast<uint32_t>(MESSAGE_TYPE_MASK::RESPONSE) == (stInterHeader_.ucMessageType & static_cast<uint32_t>(MESSAGE_TYPE_MASK::RESPONSE));
    stMetaData_.usWeek = static_cast<uint32_t>(stInterHeader_.usWeek);
    stMetaData_.dMilliseconds = static_cast<uint32_t>(stInterHeader_.dMilliseconds);
    stMetaData_.usMessageId = static_cast<uint32_t>(stInterHeader_.usMessageId);
    stMetaData_.uiMessageCrc = stInterHeader_.uiMessageDefinitionCrc;
    stMetaData_.uiHeaderLength = static_cast<uint32_t>(pcTempBuf - reinterpret_cast<const char*>(pucLogBuf_));
    stMetaData_.uiBinaryMsgLength = static_cast<uint32_t>(stInterHeader_.usLength);

    // Reconstruct a message name that won't have a suffix of any kind.
    stMetaData_.messageName = pclMyMsgDb->MsgIdToMsgName(CreateMsgId(stInterHeader_.usMessageId, static_cast<uint32_t>(MEASUREMENT_SOURCE::PRIMARY),
                                                                     static_cast<uint32_t>(MESSAGE_FORMAT::ABBREV), 0U));

    return STATUS::SUCCESS;
}
