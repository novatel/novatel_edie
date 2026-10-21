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
// ! \file commander.cpp
// ===============================================================================

#include "novatel_edie/decoders/oem/commander.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <iomanip>
#include <limits>
#include <sstream>

using namespace novatel::edie;
using namespace novatel::edie::oem;

namespace {

// -------------------------------------------------------------------------------------------------------
// Uppercase a command name or parameter, other than a quoted string.
std::string UppercaseAbbrevAsciiParam(std::string strParam_)
{
    if (strParam_.empty() || strParam_.front() == '"') { return strParam_; }

    std::transform(strParam_.begin(), strParam_.end(), strParam_.begin(), [](const unsigned char c_) { return static_cast<char>(std::toupper(c_)); });
    return strParam_;
}

// -------------------------------------------------------------------------------------------------------
// Split an abbreviated ASCII command into its parameters. Parameters are separated by spaces, except within quotes.
std::vector<std::string> SplitAbbrevAsciiParams(const std::string& strParams_)
{
    std::vector<std::string> vParams;
    bool bInParam = false;
    bool bInQuotes = false;

    for (const char c : strParams_)
    {
        if (c == ' ' && !bInQuotes)
        {
            bInParam = false;
            continue;
        }

        if (!bInParam) { vParams.emplace_back(); }
        bInParam = true;

        vParams.back() += c;
        if (c == '"') { bInQuotes = !bInQuotes; }
    }

    return vParams;
}

// -------------------------------------------------------------------------------------------------------
// Join parameters back into an abbreviated ASCII command.
std::string JoinAbbrevAsciiParams(const std::vector<std::string>& vParams_)
{
    std::string strParams;

    for (const std::string& strParam : vParams_)
    {
        if (!strParams.empty()) { strParams += ' '; }
        strParams += strParam;
    }

    return strParams;
}

// -------------------------------------------------------------------------------------------------------
// The number of parameters a field takes in an abbreviated ASCII command, or 0 if it can vary.
size_t FieldParamCount(const BaseField& clField_)
{
    switch (clField_.type)
    {
    case FIELD_TYPE::SIMPLE: [[fallthrough]];
    case FIELD_TYPE::ENUM: [[fallthrough]];
    case FIELD_TYPE::STRING: return 1;
    case FIELD_TYPE::FIXED_LENGTH_ARRAY:
        if (clField_.isCsv) { return dynamic_cast<const ArrayField&>(clField_).arrayLength; }
        return 1;
    default: return 0;
    }
}

// -------------------------------------------------------------------------------------------------------
// Check if every field of a command takes a fixed number of parameters.
bool HasFixedParams(const std::vector<BaseField::ConstPtr>& vFields_)
{
    return std::all_of(vFields_.begin(), vFields_.end(), [](const BaseField::ConstPtr& pclField_) { return FieldParamCount(*pclField_) != 0; });
}

// -------------------------------------------------------------------------------------------------------
// Check if a parameter is one of the values of an enum field.
bool IsEnumParam(const BaseField& clField_, const std::string& strParam_)
{
    const EnumDefinition::ConstPtr& pclEnumDef = dynamic_cast<const EnumField&>(clField_).enumDef;
    return pclEnumDef != nullptr && pclEnumDef->nameValue.count(strParam_) != 0;
}

// -------------------------------------------------------------------------------------------------------
// Check if a field is a port, which is an enum with a THISPORT value.
bool IsPortField(const BaseField& clField_) { return clField_.type == FIELD_TYPE::ENUM && IsEnumParam(clField_, "THISPORT"); }

// -------------------------------------------------------------------------------------------------------
// Get the database default of a field as an abbreviated ASCII parameter. Returns false if it has none.
bool GetDefaultParam(const BaseField& clField_, std::string& strParam_)
{
    if (!clField_.hasDefaultValue) { return false; }

    if (clField_.conversionHash == CalculateBlockCrc32("m")) { return false; }

    const double dDefault = clField_.defaultValueData;

    if (clField_.type == FIELD_TYPE::ENUM)
    {
        const EnumDefinition::ConstPtr& pclEnumDef = dynamic_cast<const EnumField&>(clField_).enumDef;
        if (pclEnumDef == nullptr) { return false; }

        const auto itEnumerator = pclEnumDef->valueName.find(static_cast<uint32_t>(dDefault));
        if (itEnumerator == pclEnumDef->valueName.end()) { return false; }

        strParam_ = itEnumerator->second;
        return true;
    }

    if (clField_.type != FIELD_TYPE::SIMPLE) { return false; }

    std::ostringstream oss;
    if (clField_.dataType.name == DATA_TYPE::BOOL) { oss << (dDefault == 0.0 ? "FALSE" : "TRUE"); }
    else if (clField_.conversionHash == CalculateBlockCrc32("x") || clField_.conversionHash == CalculateBlockCrc32("X") ||
             clField_.conversionHash == CalculateBlockCrc32("XB"))
    {
        oss << std::hex << static_cast<uint64_t>(dDefault);
    }
    else { oss << std::setprecision(std::numeric_limits<double>::max_digits10) << dDefault; }

    strParam_ = oss.str();
    return true;
}

// -------------------------------------------------------------------------------------------------------
// Add the database default for a leading omitted port.
void AddLeadingPortDefault(const std::vector<BaseField::ConstPtr>& vFields_, std::vector<std::string>& vParams_)
{
    if (vFields_.empty() || vParams_.empty() || !IsPortField(*vFields_.front())) { return; }
    if (IsEnumParam(*vFields_.front(), UppercaseAbbrevAsciiParam(vParams_.front()))) { return; }

    std::string strDefault;
    if (GetDefaultParam(*vFields_.front(), strDefault)) { vParams_.insert(vParams_.begin(), strDefault); }
}

// -------------------------------------------------------------------------------------------------------
// Normalize and check the supplied values of a field.
STATUS PrepareValues(const BaseField& clField_, std::vector<std::string>& vParams_, const size_t uiParam_, const size_t uiCount_)
{
    for (size_t i = uiParam_; i < uiParam_ + uiCount_; ++i)
    {
        vParams_[i] = UppercaseAbbrevAsciiParam(vParams_[i]);

        if (clField_.type == FIELD_TYPE::ENUM && !IsEnumParam(clField_, vParams_[i])) { return STATUS::MALFORMED_INPUT; }
    }

    return STATUS::SUCCESS;
}

// -------------------------------------------------------------------------------------------------------
// Handle a field that was omitted from the end of the command, which is only allowed with database defaults.
STATUS PrepareOmittedField(const BaseField& clField_, std::vector<std::string>& vParams_, const DEFAULT_PARAMETER_POLICY eDefaultParameterPolicy_)
{
    std::string strDefault;
    if (eDefaultParameterPolicy_ != DEFAULT_PARAMETER_POLICY::DATABASE_DEFAULTS || FieldParamCount(clField_) != 1 ||
        !GetDefaultParam(clField_, strDefault))
    {
        return STATUS::MALFORMED_INPUT;
    }

    vParams_.push_back(strDefault);
    return STATUS::SUCCESS;
}

// -------------------------------------------------------------------------------------------------------
// Normalize, check and, if the policy allows, complete the parameters of fields that each take a fixed number of parameters.
STATUS PrepareFields(const std::vector<BaseField::ConstPtr>& vFields_, std::vector<std::string>& vParams_,
                     const DEFAULT_PARAMETER_POLICY eDefaultParameterPolicy_)
{
    size_t uiParam = 0;

    for (const auto& pclField : vFields_)
    {
        const size_t uiParamCount = FieldParamCount(*pclField);
        STATUS eStatus = STATUS::SUCCESS;

        if (uiParam == vParams_.size()) { eStatus = PrepareOmittedField(*pclField, vParams_, eDefaultParameterPolicy_); }
        else if (uiParamCount > vParams_.size() - uiParam) { eStatus = STATUS::MALFORMED_INPUT; }
        else { eStatus = PrepareValues(*pclField, vParams_, uiParam, uiParamCount); }

        if (eStatus != STATUS::SUCCESS) { return eStatus; }
        uiParam += uiParamCount;
    }

    return uiParam == vParams_.size() ? STATUS::SUCCESS : STATUS::MALFORMED_INPUT;
}

// -------------------------------------------------------------------------------------------------------
// Prepare abbreviated ASCII parameters according to the selected omitted-parameter policy.
STATUS PrepareAbbrevAsciiParams(const FieldInfo& clFieldInfo_, std::string& strParams_, const DEFAULT_PARAMETER_POLICY eDefaultParameterPolicy_)
{
    const std::vector<BaseField::ConstPtr>& vFields = clFieldInfo_.messageOrderedFields;
    if (vFields.empty()) { return STATUS::SUCCESS; }

    std::vector<std::string> vParams = SplitAbbrevAsciiParams(strParams_.substr(0, strParams_.find_first_of("\r\n")));

    // Variable-length fields are not supported
    if (!HasFixedParams(vFields))
    {
        if (eDefaultParameterPolicy_ == DEFAULT_PARAMETER_POLICY::DATABASE_DEFAULTS) { return STATUS::UNSUPPORTED; }

        std::transform(vParams.begin(), vParams.end(), vParams.begin(), UppercaseAbbrevAsciiParam);
        strParams_ = JoinAbbrevAsciiParams(vParams);
        return STATUS::SUCCESS;
    }

    if (eDefaultParameterPolicy_ == DEFAULT_PARAMETER_POLICY::DATABASE_DEFAULTS) { AddLeadingPortDefault(vFields, vParams); }

    const STATUS eStatus = PrepareFields(vFields, vParams, eDefaultParameterPolicy_);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    strParams_ = JoinAbbrevAsciiParams(vParams);
    return STATUS::SUCCESS;
}

// -------------------------------------------------------------------------------------------------------
// Check if a value is the named enumerator of an enum in the database.
bool IsEnumerator(const MessageDatabase& clMessageDb_, const char* pcEnumName_, const char* pcEnumerator_, const uint32_t uiValue_)
{
    const EnumDefinition::ConstPtr pclEnumDef = clMessageDb_.GetEnumDefName(pcEnumName_);
    if (pclEnumDef == nullptr) { return false; }

    const auto itEnumerator = pclEnumDef->nameValue.find(pcEnumerator_);
    return itEnumerator != pclEnumDef->nameValue.end() && itEnumerator->second == uiValue_;
}

// -------------------------------------------------------------------------------------------------------
// Populate metadata for a command and the payload produced by a LOG ... ONCE command.
void PopulateCommandMetadata(const MessageDatabase& clMessageDb_, const MessageDefinition& clMessageDef_, const CompositeField& clMessage_,
                             CommandMetadata& stMetadata_)
{
    stMetadata_ = {};
    stMetadata_.commandId = static_cast<uint16_t>(clMessageDef_.logID);
    if (clMessageDef_.name != "LOG") { return; }

    // Only ONCE gives a single log that follows the response. The other triggers (ONCHANGED, ONTIME, ...)
    // start a stream of logs that go on after the command is answered, so there is no payload to wait for.
    const auto uiTrigger = static_cast<uint32_t>(clMessage_.GetFieldValueByName<int32_t>("trigger"));
    if (!IsEnumerator(clMessageDb_, "LogTrigger", "ONCE", uiTrigger)) { return; }

    [[maybe_unused]] uint32_t uiSiblingId = 0;
    [[maybe_unused]] uint32_t uiMessageFormat = 0;
    [[maybe_unused]] uint32_t uiResponse = 0;
    UnpackMsgId(clMessage_.GetFieldValueByName<uint32_t>("message_id"), stMetadata_.payloadId, uiSiblingId, uiMessageFormat, uiResponse);

    // A payload on THISPORT comes back on whichever port the command was sent on, so it has no fixed port
    const auto uiPort = static_cast<uint32_t>(clMessage_.GetFieldValueByName<int32_t>("log_port_address"));
    if (!IsEnumerator(clMessageDb_, "PortAddress", "THISPORT", uiPort)) { stMetadata_.payloadPort = uiPort; }
}

// -------------------------------------------------------------------------------------------------------
// Encode a prepared abbreviated ASCII command using the supplied EDIE components.
STATUS EncodeCommand(const MessageDatabase& clJsonDb_, const MessageDecoder& clMessageDecoder_, Encoder& clEncoder_,
                     const char* pcAbbrevAsciiCommand_, const uint32_t uiAbbrevAsciiCommandLength_, char* pcEncodeBuffer_,
                     uint32_t& uiEncodeBufferSize_, const ENCODE_FORMAT eEncodeFormat_, CommandMetadata& stMetadata_,
                     const DEFAULT_PARAMETER_POLICY eDefaultParameterPolicy_)
{
    constexpr uint32_t thisPort = 0xC0;

    stMetadata_ = {};
    CommandMetadata stCommandMetadata;

    if ((pcAbbrevAsciiCommand_ == nullptr) || (pcEncodeBuffer_ == nullptr)) { return STATUS::NULL_PROVIDED; }

    if (eEncodeFormat_ != ENCODE_FORMAT::ASCII && eEncodeFormat_ != ENCODE_FORMAT::BINARY) { return STATUS::UNSUPPORTED; }

    const std::string strAbbrevAsciiCommand(pcAbbrevAsciiCommand_, uiAbbrevAsciiCommandLength_);
    const size_t ullPos = strAbbrevAsciiCommand.find_first_of(' ');
    const std::string strCmdName = UppercaseAbbrevAsciiParam(strAbbrevAsciiCommand.substr(0, ullPos));
    std::string strCmdParams;
    if (ullPos != std::string::npos) { strCmdParams = strAbbrevAsciiCommand.substr(ullPos + 1); }

    MessageDefinition::ConstPtr pclMessageDef = clJsonDb_.GetMsgDef(strCmdName);
    if (!pclMessageDef) { return STATUS::NO_DEFINITION; }

    const FieldInfo& clFieldInfo = *pclMessageDef->fieldInfo.begin()->second;

    const STATUS eParamsStatus = PrepareAbbrevAsciiParams(clFieldInfo, strCmdParams, eDefaultParameterPolicy_);
    if (eParamsStatus != STATUS::SUCCESS) { return eParamsStatus; }
    if (strCmdParams.size() >= MAX_ASCII_MESSAGE_LENGTH) { return STATUS::BUFFER_FULL; }

    unsigned char acCmdParams[MAX_ASCII_MESSAGE_LENGTH];
    unsigned char* pucCmdParams = acCmdParams;
    std::copy(strCmdParams.begin(), strCmdParams.end(), acCmdParams);
    acCmdParams[strCmdParams.size()] = '\0';

    MessageDataStruct stMessageData;
    MetaDataStruct stMetaData;
    IntermediateHeader stIntermediateHeader;
    CompositeField stIntermediateMessage;

    // Prime the metadata with information we already know
    stMetaData.eFormat = DECODE_FORMAT::ABB_ASCII;
    stMetaData.usMessageId = static_cast<uint16_t>(pclMessageDef->logID);
    stMetaData.uiMessageCrc = static_cast<uint32_t>(pclMessageDef->fieldInfo.begin()->first);

    const STATUS eDecoderStatus = clMessageDecoder_.Decode(pucCmdParams, stIntermediateMessage, stMetaData);
    if (eDecoderStatus != STATUS::SUCCESS) { return eDecoderStatus; }

    PopulateCommandMetadata(clJsonDb_, *pclMessageDef, stIntermediateMessage, stCommandMetadata);

    // Prime the intermediate header with information we already know
    stIntermediateHeader.uiPortAddress = thisPort;
    stIntermediateHeader.usMessageId = stMetaData.usMessageId;
    stIntermediateHeader.uiMessageDefinitionCrc = stMetaData.uiMessageCrc;

    auto* pucEncodeBuffer = reinterpret_cast<unsigned char*>(pcEncodeBuffer_);
    const STATUS eEncoderStatus =
        clEncoder_.Encode(&pucEncodeBuffer, uiEncodeBufferSize_, stIntermediateHeader, stIntermediateMessage, stMessageData, eEncodeFormat_);

    if (eEncoderStatus != STATUS::SUCCESS) { return eEncoderStatus; }

    stMetadata_ = stCommandMetadata;

    // Null-terminate the command, if possible. Otherwise, the command will be the size of the buffer.
    if (stMessageData.uiMessageLength < uiEncodeBufferSize_) { stMessageData.pucMessage[stMessageData.uiMessageLength] = '\0'; }
    uiEncodeBufferSize_ = stMessageData.uiMessageLength;

    return STATUS::SUCCESS;
}

} // namespace

// -------------------------------------------------------------------------------------------------------
Commander::Commander(MessageDatabase::Ptr pclMessageDb_) : Commander(pclMessageDb_, DEFAULT_PARAMETER_POLICY::REQUIRE_ALL_PARAMETERS) {}

// -------------------------------------------------------------------------------------------------------
Commander::Commander(MessageDatabase::Ptr pclMessageDb_, const DEFAULT_PARAMETER_POLICY eDefaultParameterPolicy_)
    : clMyMessageDecoder(pclMessageDb_), clMyEncoder(pclMessageDb_), eMyDefaultParameterPolicy(eDefaultParameterPolicy_)
{
    pclMyLogger->debug("Commander initializing...");
    if (pclMessageDb_ != nullptr) { LoadJsonDb(pclMessageDb_); }
    pclMyLogger->debug("Commander initialized");
}

// -------------------------------------------------------------------------------------------------------
void Commander::LoadJsonDb(MessageDatabase::Ptr pclMessageDb_)
{
    pclMyMsgDb = pclMessageDb_;
    InitEnumDefinitions();
}

// -------------------------------------------------------------------------------------------------------
void Commander::InitEnumDefinitions()
{
    vMyCommandDefinitions = pclMyMsgDb->GetEnumDefName("Commands");
    vMyPortAddressDefinitions = pclMyMsgDb->GetEnumDefName("PortAddress");
    vMyGpsTimeStatusDefinitions = pclMyMsgDb->GetEnumDefName("GPSTimeStatus");
}

// -------------------------------------------------------------------------------------------------------
STATUS Commander::Encode(const char* pcAbbrevAsciiCommand_, const uint32_t uiAbbrevAsciiCommandLength_, char* pcEncodeBuffer_,
                         uint32_t& uiEncodeBufferSize_, const ENCODE_FORMAT eEncodeFormat_)
{
    CommandMetadata stMetadata;
    return Encode(pcAbbrevAsciiCommand_, uiAbbrevAsciiCommandLength_, pcEncodeBuffer_, uiEncodeBufferSize_, eEncodeFormat_, stMetadata);
}

// -------------------------------------------------------------------------------------------------------
STATUS Commander::Encode(const char* pcAbbrevAsciiCommand_, const uint32_t uiAbbrevAsciiCommandLength_, char* pcEncodeBuffer_,
                         uint32_t& uiEncodeBufferSize_, const ENCODE_FORMAT eEncodeFormat_, CommandMetadata& stMetadata)
{
    stMetadata = {};

    if ((pcAbbrevAsciiCommand_ == nullptr) || (pcEncodeBuffer_ == nullptr)) { return STATUS::NULL_PROVIDED; }
    if (eEncodeFormat_ != ENCODE_FORMAT::ASCII && eEncodeFormat_ != ENCODE_FORMAT::BINARY) { return STATUS::UNSUPPORTED; }
    if (pclMyMsgDb == nullptr) { return STATUS::NO_DATABASE; }

    return EncodeCommand(*pclMyMsgDb, clMyMessageDecoder, clMyEncoder, pcAbbrevAsciiCommand_, uiAbbrevAsciiCommandLength_, pcEncodeBuffer_,
                         uiEncodeBufferSize_, eEncodeFormat_, stMetadata, eMyDefaultParameterPolicy);
}

// -------------------------------------------------------------------------------------------------------
STATUS Commander::Encode(const MessageDatabase& clJsonDb_, const MessageDecoder& clMessageDecoder_, Encoder& clEncoder_,
                         const char* pcAbbrevAsciiCommand_, const uint32_t uiAbbrevAsciiCommandLength_, char* pcEncodeBuffer_,
                         uint32_t& uiEncodeBufferSize_, const ENCODE_FORMAT eEncodeFormat_)
{
    CommandMetadata stMetadata;
    return Encode(clJsonDb_, clMessageDecoder_, clEncoder_, pcAbbrevAsciiCommand_, uiAbbrevAsciiCommandLength_, pcEncodeBuffer_, uiEncodeBufferSize_,
                  eEncodeFormat_, stMetadata);
}

// -------------------------------------------------------------------------------------------------------
STATUS Commander::Encode(const MessageDatabase& clJsonDb_, const MessageDecoder& clMessageDecoder_, Encoder& clEncoder_,
                         const char* pcAbbrevAsciiCommand_, const uint32_t uiAbbrevAsciiCommandLength_, char* pcEncodeBuffer_,
                         uint32_t& uiEncodeBufferSize_, const ENCODE_FORMAT eEncodeFormat_, CommandMetadata& stMetadata)
{
    return Encode(clJsonDb_, clMessageDecoder_, clEncoder_, pcAbbrevAsciiCommand_, uiAbbrevAsciiCommandLength_, pcEncodeBuffer_, uiEncodeBufferSize_,
                  eEncodeFormat_, stMetadata, DEFAULT_PARAMETER_POLICY::REQUIRE_ALL_PARAMETERS);
}

// -------------------------------------------------------------------------------------------------------
STATUS Commander::Encode(const MessageDatabase& clJsonDb_, const MessageDecoder& clMessageDecoder_, Encoder& clEncoder_,
                         const char* pcAbbrevAsciiCommand_, const uint32_t uiAbbrevAsciiCommandLength_, char* pcEncodeBuffer_,
                         uint32_t& uiEncodeBufferSize_, const ENCODE_FORMAT eEncodeFormat_, const DEFAULT_PARAMETER_POLICY eDefaultParameterPolicy_)
{
    CommandMetadata stMetadata;
    return Encode(clJsonDb_, clMessageDecoder_, clEncoder_, pcAbbrevAsciiCommand_, uiAbbrevAsciiCommandLength_, pcEncodeBuffer_, uiEncodeBufferSize_,
                  eEncodeFormat_, stMetadata, eDefaultParameterPolicy_);
}

// -------------------------------------------------------------------------------------------------------
STATUS Commander::Encode(const MessageDatabase& clJsonDb_, const MessageDecoder& clMessageDecoder_, Encoder& clEncoder_,
                         const char* pcAbbrevAsciiCommand_, const uint32_t uiAbbrevAsciiCommandLength_, char* pcEncodeBuffer_,
                         uint32_t& uiEncodeBufferSize_, const ENCODE_FORMAT eEncodeFormat_, CommandMetadata& stMetadata_,
                         const DEFAULT_PARAMETER_POLICY eDefaultParameterPolicy_)
{
    return EncodeCommand(clJsonDb_, clMessageDecoder_, clEncoder_, pcAbbrevAsciiCommand_, uiAbbrevAsciiCommandLength_, pcEncodeBuffer_,
                         uiEncodeBufferSize_, eEncodeFormat_, stMetadata_, eDefaultParameterPolicy_);
}
