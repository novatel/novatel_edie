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

#include "novatel_edie/decoders/common/common.hpp"

namespace novatel::edie {

//-----------------------------------------------------------------------
void MessageDatabase::RemoveMessage(const uint32_t iMsgId_, const bool bGenerateMappings_)
{
    auto iTer = GetMessageIt(iMsgId_);

    if (iTer != vMessageDefinitions.end())
    {
        RemoveMessageMapping(**iTer);
        vMessageDefinitions.erase(iTer);
    }

    if (bGenerateMappings_) { GenerateMappings(); }
}

//-----------------------------------------------------------------------
void MessageDatabase::RemoveEnumeration(std::string_view strEnumeration_, const bool bGenerateMappings_)
{
    const auto iTer = GetEnumIt(strEnumeration_);

    if (iTer != vEnumDefinitions.end())
    {
        RemoveEnumerationMapping(**iTer);
        vEnumDefinitions.erase(iTer);
    }

    if (bGenerateMappings_) { GenerateMappings(); }
}

//-----------------------------------------------------------------------
uint32_t MessageDatabase::MsgNameToMsgId(std::string sMsgName_) const
{
    uint32_t uiSiblingId = 0;
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
    uint32_t uiSiblingId = 0;
    uint32_t uiMessageFormat = 0;
    uint32_t uiResponse = 0;

    UnpackMsgId(uiMessageId_, usLogId, uiSiblingId, uiMessageFormat, uiResponse);

    MessageDefinition::ConstPtr pstMessageDefinition = GetMsgDef(usLogId);
    std::string strMessageName = pstMessageDefinition != nullptr ? pstMessageDefinition->name : GetEnumString(vEnumDefinitions[0], usLogId);

    std::string strMessageFormatSuffix;
    if (uiResponse != 0U) { strMessageFormatSuffix = "R"; }
    else if (uiMessageFormat == static_cast<uint32_t>(MESSAGE_FORMAT::BINARY)) { strMessageFormatSuffix = "B"; }
    else if (uiMessageFormat == static_cast<uint32_t>(MESSAGE_FORMAT::ASCII)) { strMessageFormatSuffix = "A"; }
    else { strMessageFormatSuffix = ""; } // default to abbreviated ASCII format

    if (uiSiblingId != 0U) { strMessageFormatSuffix.append("_").append(std::to_string(uiSiblingId)); }

    return strMessageName.append(strMessageFormatSuffix);
}

//-----------------------------------------------------------------------
MessageDefinition::ConstPtr MessageDatabase::GetMsgDef(const std::string& strMsgName_) const
{
    const auto it = mMessageName.find(strMsgName_);
    return it != mMessageName.end() ? it->second : nullptr;
}

//-----------------------------------------------------------------------
// TODO: need to look into the map and find the right crc and return the msg def for that CRC
MessageDefinition::ConstPtr MessageDatabase::GetMsgDef(const int32_t iMsgId_) const
{
    const auto it = mMessageId.find(iMsgId_);
    return it != mMessageId.end() ? it->second : nullptr;
}

// -------------------------------------------------------------------------------------------------------
const std::vector<BaseField::Ptr>& MessageDefinition::GetMsgDefFromCrc([[maybe_unused]] spdlog::logger& pclLogger_, uint32_t uiMsgDefCrc_) const
{
    // If we can't find the correct CRC just default to the latest.
    if (fields.find(uiMsgDefCrc_) == fields.end())
    {
        // TODO: this branch always gets triggered for decompression. Disabled logging for now.
        // pclLogger_.info("Log DB is missing the log definition {} - {}.  Defaulting to newest version of the log definition.", name, uiMsgDefCrc_);
        uiMsgDefCrc_ = latestMessageCrc;
    }

    return fields.at(uiMsgDefCrc_);
}

} // namespace novatel::edie
