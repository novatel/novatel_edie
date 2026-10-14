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
// ! \file command_decoder.cpp
// ===============================================================================

#include "novatel_edie/decoders/oem/command_decoder.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <iomanip>
#include <limits>
#include <sstream>
#include <type_traits>
#include <variant>

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
// Check if a parameter is one of the values of an enum field.
bool IsEnumParam(const BaseField& clField_, const std::string& strParam_)
{
    const EnumDefinition::ConstPtr& pclEnumDef = dynamic_cast<const EnumField&>(clField_).enumDef;
    return pclEnumDef != nullptr && pclEnumDef->nameValue.count(strParam_) != 0;
}

// -------------------------------------------------------------------------------------------------------
// Get the database default of a field, or of each element of an array, as an abbreviated ASCII parameter.
// Returns false if it has none.
bool GetDefaultParam(const BaseField& clField_, std::string& strParam_)
{
    if (!clField_.defaultValue) { return false; }

    // Message names and characters are given as text, which a numeric default can't be
    if (clField_.conversionHash == CalculateBlockCrc32("m") || clField_.conversionHash == CalculateBlockCrc32("c")) { return false; }

    if (clField_.type == FIELD_TYPE::ENUM)
    {
        const EnumDefinition::ConstPtr& pclEnumDef = dynamic_cast<const EnumField&>(clField_).enumDef;
        if (pclEnumDef == nullptr) { return false; }

        const auto uiValue = std::visit([](const auto value_) { return static_cast<uint32_t>(value_); }, *clField_.defaultValue);
        const auto itEnumerator = pclEnumDef->valueName.find(uiValue);
        if (itEnumerator == pclEnumDef->valueName.end()) { return false; }

        strParam_ = itEnumerator->second;
        return true;
    }

    const bool bHex = clField_.conversionHash == CalculateBlockCrc32("x") || clField_.conversionHash == CalculateBlockCrc32("X") ||
                      clField_.conversionHash == CalculateBlockCrc32("XB");

    std::ostringstream oss;
    std::visit(
        [&](const auto value_) {
            using T = std::decay_t<decltype(value_)>;
            if constexpr (std::is_same_v<T, bool>) { oss << (value_ ? "TRUE" : "FALSE"); }
            else if constexpr (std::is_floating_point_v<T>) { oss << std::setprecision(std::numeric_limits<T>::max_digits10) << value_; }
            else
            {
                if (bHex) { oss << std::hex; }
                oss << +value_; // + so 8-bit values are written as numbers, not characters
            }
        },
        *clField_.defaultValue);

    strParam_ = oss.str();
    return true;
}

// -------------------------------------------------------------------------------------------------------
// Add the parameters of a field left off the end of a command. Returns false if the field has no default.
bool AddOmittedParams(const BaseField& clField_, std::vector<std::string>& vParams_)
{
    std::string strDefault;

    switch (clField_.type)
    {
    case FIELD_TYPE::SIMPLE: [[fallthrough]];
    case FIELD_TYPE::ENUM:
        if (!GetDefaultParam(clField_, strDefault)) { return false; }
        vParams_.push_back(strDefault);
        return true;
    case FIELD_TYPE::FIXED_LENGTH_ARRAY:
        // Arrays given as a single parameter, such as strings, have no default
        if (!clField_.isCsv || !GetDefaultParam(clField_, strDefault)) { return false; }
        vParams_.insert(vParams_.end(), dynamic_cast<const ArrayField&>(clField_).arrayLength, strDefault);
        return true;
    case FIELD_TYPE::VARIABLE_LENGTH_ARRAY:
        // An empty array given as a single parameter, such as hex bytes, can't be followed by more fields
        if (!clField_.isCsv) { return false; }
        [[fallthrough]];
    case FIELD_TYPE::FIELD_ARRAY: vParams_.emplace_back("0"); return true;
    default: return false;
    }
}

// -------------------------------------------------------------------------------------------------------
// Read the element count of a variable-length or field array, which can't be more than the array's length.
bool ParseArrayCount(const BaseField& clField_, const std::string& strParam_, uint32_t& uiCount_)
{
    const char* pcEnd = strParam_.data() + strParam_.size();
    const auto [pcParsed, eError] = std::from_chars(strParam_.data(), pcEnd, uiCount_);
    return eError == std::errc() && pcParsed == pcEnd && uiCount_ <= dynamic_cast<const ArrayField&>(clField_).arrayLength;
}

// -------------------------------------------------------------------------------------------------------
// Check the parameters of a list of fields, starting at uiParam_, and fill in the ones that were left out.
// On return uiParam_ is the parameter after the last field.
STATUS CompleteParams(const std::vector<BaseField::ConstPtr>& vFields_, std::vector<std::string>& vParams_, size_t& uiParam_)
{
    for (const auto& pclField : vFields_)
    {
        const BaseField& clField = *pclField;

        // Fields left off the end of the command take their defaults
        if (uiParam_ == vParams_.size())
        {
            if (!AddOmittedParams(clField, vParams_)) { return STATUS::MALFORMED_INPUT; }
            uiParam_ = vParams_.size();
            continue;
        }

        switch (clField.type)
        {
        case FIELD_TYPE::ENUM:
            // An enum with a default that the parameter isn't a value of was left out, the parameter is for a later field
            if (!IsEnumParam(clField, vParams_[uiParam_]))
            {
                std::string strDefault;
                if (!GetDefaultParam(clField, strDefault)) { return STATUS::MALFORMED_INPUT; }
                vParams_.insert(vParams_.begin() + static_cast<std::ptrdiff_t>(uiParam_), strDefault);
            }
            ++uiParam_;
            break;
        case FIELD_TYPE::SIMPLE: [[fallthrough]];
        case FIELD_TYPE::STRING: ++uiParam_; break;
        case FIELD_TYPE::FIXED_LENGTH_ARRAY: uiParam_ += clField.isCsv ? dynamic_cast<const ArrayField&>(clField).arrayLength : 1; break;
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: {
            uint32_t uiCount = 0;
            if (!ParseArrayCount(clField, vParams_[uiParam_++], uiCount)) { return STATUS::MALFORMED_INPUT; }
            // An array given as a single parameter, such as hex bytes, has no parameter when it's empty
            uiParam_ += clField.isCsv ? uiCount : std::min(uiCount, 1U);
            break;
        }
        case FIELD_TYPE::FIELD_ARRAY: {
            uint32_t uiCount = 0;
            if (!ParseArrayCount(clField, vParams_[uiParam_++], uiCount)) { return STATUS::MALFORMED_INPUT; }

            const std::vector<BaseField::ConstPtr>& vSubFields = dynamic_cast<const FieldArrayField&>(clField).fieldInfo->messageOrderedFields;
            for (uint32_t i = 0; i < uiCount; ++i)
            {
                const STATUS eStatus = CompleteParams(vSubFields, vParams_, uiParam_);
                if (eStatus != STATUS::SUCCESS) { return eStatus; }
            }
            break;
        }
        default: return STATUS::UNSUPPORTED;
        }

        // An array was given fewer values than it has elements
        if (uiParam_ > vParams_.size()) { return STATUS::MALFORMED_INPUT; }
    }

    return STATUS::SUCCESS;
}

} // namespace

// -------------------------------------------------------------------------------------------------------
CommandDecoder::CommandDecoder(MessageDatabase::Ptr pclMessageDb_) : clMyMessageDecoder(pclMessageDb_)
{
    pclMyLogger->debug("CommandDecoder initializing...");
    if (pclMessageDb_ != nullptr) { LoadJsonDb(pclMessageDb_); }
    pclMyLogger->debug("CommandDecoder initialized");
}

// -------------------------------------------------------------------------------------------------------
void CommandDecoder::LoadJsonDb(MessageDatabase::Ptr pclMessageDb_)
{
    pclMyMsgDb = pclMessageDb_;
    clMyMessageDecoder.LoadJsonDb(pclMessageDb_);
}

// -------------------------------------------------------------------------------------------------------
STATUS CommandDecoder::Decode(const char* pcCommand_, const uint32_t uiCommandLength_, IntermediateHeader& stHeader_,
                              CompositeField& stMessage_) const
{
    constexpr uint32_t thisPort = 0xC0;

    if (pcCommand_ == nullptr) { return STATUS::NULL_PROVIDED; }

    if (pclMyMsgDb == nullptr) { return STATUS::NO_DATABASE; }

    const std::string strCommand(pcCommand_, uiCommandLength_);
    std::vector<std::string> vParams = SplitAbbrevAsciiParams(strCommand.substr(0, strCommand.find_first_of("\r\n")));
    if (vParams.empty()) { return STATUS::MALFORMED_INPUT; }

    std::transform(vParams.begin(), vParams.end(), vParams.begin(), UppercaseAbbrevAsciiParam);
    const std::string strCmdName = vParams.front();
    vParams.erase(vParams.begin());

    MessageDefinition::ConstPtr pclMessageDef = pclMyMsgDb->GetMsgDef(strCmdName);
    if (!pclMessageDef) { return STATUS::NO_DEFINITION; }

    const auto itFieldInfo = pclMessageDef->fieldInfo.find(pclMessageDef->latestMessageCrc);
    if (itFieldInfo == pclMessageDef->fieldInfo.end()) { return STATUS::NO_DEFINITION; }

    size_t uiParam = 0;
    const STATUS eParamsStatus = CompleteParams(itFieldInfo->second->messageOrderedFields, vParams, uiParam);
    if (eParamsStatus != STATUS::SUCCESS) { return eParamsStatus; }
    if (uiParam != vParams.size()) { return STATUS::MALFORMED_INPUT; }

    const std::string strParams = JoinAbbrevAsciiParams(vParams);
    if (strParams.size() >= MAX_ASCII_MESSAGE_LENGTH) { return STATUS::BUFFER_FULL; }

    // Prime the metadata with information we already know
    MetaDataStruct stMetaData;
    stMetaData.eFormat = DECODE_FORMAT::ABB_ASCII;
    stMetaData.usMessageId = static_cast<uint16_t>(pclMessageDef->logID);
    stMetaData.uiMessageCrc = pclMessageDef->latestMessageCrc;

    stMessage_ = CompositeField();
    const STATUS eDecoderStatus = clMyMessageDecoder.Decode(reinterpret_cast<const unsigned char*>(strParams.c_str()), stMessage_, stMetaData);
    if (eDecoderStatus != STATUS::SUCCESS) { return eDecoderStatus; }

    stHeader_ = IntermediateHeader();
    stHeader_.uiPortAddress = thisPort;
    stHeader_.usMessageId = stMetaData.usMessageId;
    stHeader_.uiMessageDefinitionCrc = stMetaData.uiMessageCrc;

    return STATUS::SUCCESS;
}
