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
// ! \file message_database.cpp
// ===============================================================================

#include "novatel_edie/decoders/common/message_database.hpp"

#include <unordered_set>

#include "novatel_edie/decoders/common/common.hpp"

namespace novatel::edie {

//-----------------------------------------------------------------------
void MessageDatabase::RemoveMessage(const uint32_t iMsgId_)
{
    auto iTer = GetMessageIt(iMsgId_);

    if (iTer != vMessageDefinitions.end())
    {
        RemoveMessageMapping(**iTer);
        vMessageDefinitions.erase(iTer);
    }
}

//-----------------------------------------------------------------------
void MessageDatabase::RemoveEnumeration(std::string_view strEnumeration_)
{
    const auto iTer = GetEnumIt(strEnumeration_);

    if (iTer != vEnumDefinitions.end())
    {
        RemoveEnumerationMapping(**iTer);
        vEnumDefinitions.erase(iTer);
    }
}

//-----------------------------------------------------------------------
uint32_t MessageDatabase::MsgNameToMsgId(std::string sMsgName_) const
{
    uint32_t uiSiblingId = NULL_SIBLING_ID;
    uint32_t uiMsgFormat;
    uint32_t uiResponse;

    // Ingest the sibling information, i.e. the _1 from LOGNAMEA_1
    if (sMsgName_.find_last_of('_') != std::string::npos && sMsgName_.find_last_of('_') == sMsgName_.size() - 2)
    {
        uiSiblingId = static_cast<uint32_t>(ToDigit(sMsgName_.back()));
        sMsgName_.resize(sMsgName_.size() - 2);
    }

    // If this is an abbrev msg (no format information), we will be able to find the MsgDef
    MessageDefinition::ConstPtr pclMessageDef = GetMsgDef(sMsgName_);
    if (pclMessageDef != nullptr)
    {
        uiResponse = static_cast<uint32_t>(false);
        uiMsgFormat = static_cast<uint32_t>(MESSAGE_FORMAT::ABBREV);

        return CreateMsgId(pclMessageDef->logID, uiSiblingId, uiMsgFormat, uiResponse);
    }

    switch (sMsgName_.back())
    {
    case 'R': // ASCII Response
        uiResponse = static_cast<uint32_t>(true);
        uiMsgFormat = static_cast<uint32_t>(MESSAGE_FORMAT::ASCII);
        sMsgName_.pop_back();
        break;
    case 'A': // ASCII
        uiResponse = static_cast<uint32_t>(false);
        uiMsgFormat = static_cast<uint32_t>(MESSAGE_FORMAT::ASCII);
        sMsgName_.pop_back();
        break;
    case 'B': // Binary
        uiResponse = static_cast<uint32_t>(false);
        uiMsgFormat = static_cast<uint32_t>(MESSAGE_FORMAT::BINARY);
        sMsgName_.pop_back();
        break;
    default: // Abbreviated ASCII
        uiResponse = static_cast<uint32_t>(false);
        uiMsgFormat = static_cast<uint32_t>(MESSAGE_FORMAT::ABBREV);
        break;
    }

    pclMessageDef = GetMsgDef(sMsgName_);

    return pclMessageDef != nullptr ? CreateMsgId(pclMessageDef->logID, uiSiblingId, uiMsgFormat, uiResponse) : 0;
}

// -------------------------------------------------------------------------------------------------------
std::string MessageDatabase::MsgIdToMsgName(const uint32_t uiMessageId_) const
{
    uint16_t usLogId = 0;
    uint32_t uiSiblingId = NULL_SIBLING_ID;
    uint32_t uiMessageFormat = 0;
    uint32_t uiResponse = 0;

    UnpackMsgId(uiMessageId_, usLogId, uiSiblingId, uiMessageFormat, uiResponse);

    MessageDefinition::ConstPtr pstMessageDefinition = GetMsgDef(usLogId);
    std::string strMessageName = pstMessageDefinition != nullptr ? pstMessageDefinition->name : "UNKNOWN";

    std::string strMessageFormatSuffix;
    if (uiResponse != 0U) { strMessageFormatSuffix = 'R'; }
    else if (uiMessageFormat == static_cast<uint32_t>(MESSAGE_FORMAT::BINARY)) { strMessageFormatSuffix = 'B'; }
    else if (uiMessageFormat == static_cast<uint32_t>(MESSAGE_FORMAT::ASCII)) { strMessageFormatSuffix = 'A'; }

    if (uiSiblingId != 0U) { strMessageFormatSuffix.append("_").append(std::to_string(uiSiblingId)); }

    return strMessageName.append(strMessageFormatSuffix);
}

//-----------------------------------------------------------------------
MessageDefinition::ConstPtr MessageDatabase::GetMsgDef(std::string_view strMsgName_) const
{
    const auto it = mMessageName.find(strMsgName_.data());
    return it != mMessageName.end() ? it->second : nullptr;
}

//-----------------------------------------------------------------------
// TODO: need to look into the map and find the right crc and return the msg def for that CRC
MessageDefinition::ConstPtr MessageDatabase::GetMsgDef(const int32_t iMsgId_) const
{
    const auto it = mMessageId.find(iMsgId_);
    return it != mMessageId.end() ? it->second : nullptr;
}

//-----------------------------------------------------------------------
void LogMissingMsgDef(spdlog::logger& pclLogger_, const int32_t iMsgId_)
{
    thread_local std::unordered_set<int32_t> loggedMissingMsgDefs;
    const bool bFirstReport = loggedMissingMsgDefs.insert(iMsgId_).second;
    if (bFirstReport) { SPDLOG_LOGGER_INFO(&pclLogger_, "No log definition for ID {}", iMsgId_); }
    else { SPDLOG_LOGGER_DEBUG(&pclLogger_, "No log definition for ID {}", iMsgId_); }
}

//-----------------------------------------------------------------------
MessageDefinition::ConstPtr MessageDatabase::GetResponseDefinition() const
{
    if (pResponseDefinition != nullptr) { return pResponseDefinition; }

    auto responseDefinition = std::make_shared<MessageDefinition>();
    responseDefinition->name = "response";

    auto responsesEnum = GetEnumDefName("Responses");

    SimpleDataType responseIdDataType;
    responseIdDataType.description = "Response as numerical id";
    responseIdDataType.length = 4;
    responseIdDataType.name = DATA_TYPE::UINT;

    auto responseIdField = std::make_shared<EnumField>();
    responseIdField->name = "response_id";
    responseIdField->type = FIELD_TYPE::RESPONSE_ID;
    responseIdField->dataType = responseIdDataType;
    responseIdField->index = 0;
    responseIdField->length = 4;
    if (responsesEnum != nullptr) { responseIdField->enumId = responsesEnum->_id; }
    responseIdField->enumDef = responsesEnum;

    SimpleDataType responseStrDataType;
    responseStrDataType.description = "Response as a string";
    responseStrDataType.length = 1;
    responseStrDataType.name = DATA_TYPE::CHAR;

    auto responseStrField = std::make_shared<BaseField>();
    responseStrField->name = "response_str";
    responseStrField->type = FIELD_TYPE::RESPONSE_STR;
    responseStrField->dataType = responseStrDataType;
    responseStrField->index = 0;

    auto& responseFieldInfo = responseDefinition->fieldInfo[0]; // Responses do not use definition CRCs.
    responseFieldInfo.fixedFieldBytes = sizeof(uint32_t);
    responseFieldInfo.varFieldCount = 1;
    responseFieldInfo.messageOrderedFields = {responseIdField, responseStrField};
    responseFieldInfo.fieldNameToDef[responseIdField->name] = responseIdField;
    responseFieldInfo.fieldNameToDef[responseStrField->name] = responseStrField;

    pResponseDefinition = responseDefinition;
    return pResponseDefinition;
}

// -------------------------------------------------------------------------------------------------------
const FieldInfo& MessageDefinition::GetMsgDefFromCrc(uint32_t uiMsgDefCrc_) const
{
    auto it = fieldInfo.find(uiMsgDefCrc_);
    return (it != fieldInfo.end()) ? it->second : fieldInfo.at(latestMessageCrc);
}

// -------------------------------------------------------------------------------------------------------
void MessageDatabase::RegisterAlignmentFunction(std::string messageFamily_, std::function<size_t(const size_t, const uintptr_t, const uintptr_t)> fn)
{
    GetAlignmentFunctions()[messageFamily_] = std::move(fn);
}

// -------------------------------------------------------------------------------------------------------
std::unordered_map<std::string, std::function<size_t(const size_t, const uintptr_t, const uintptr_t)>>& MessageDatabase::GetAlignmentFunctions()
{
    static std::unordered_map<std::string, std::function<size_t(const size_t, const uintptr_t, const uintptr_t)>> alignmentFunctions;
    return alignmentFunctions;
}

} // namespace novatel::edie
