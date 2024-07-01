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
// ! \file json_reader.cpp
// ===============================================================================

#include "novatel_edie/common/json_reader.hpp"

#include "novatel_edie/common/common.hpp"

namespace novatel::edie {

//-----------------------------------------------------------------------
void from_json(const json& j_, EnumDataType& f_)
{
    f_.value = j_.at("value");
    f_.name = j_.at("name");
    f_.description = j_.at("description").is_null() ? "" : j_.at("description");
}

//-----------------------------------------------------------------------
void from_json(const json& j_, BaseDataType& f_)
{
    auto itrDataTypeMapping = DataTypeEnumLookup.find(j_.at("name"));
    f_.name = itrDataTypeMapping != DataTypeEnumLookup.end() ? itrDataTypeMapping->second : DATA_TYPE::UNKNOWN;
    f_.length = j_.at("length");
    f_.description = j_.at("description").is_null() ? "" : j_.at("description");
}

//-----------------------------------------------------------------------
void from_json(const json& j_, SimpleDataType& f_)
{
    from_json(j_, static_cast<BaseDataType&>(f_));

    if (j_.find("enum") != j_.end())
    {
        for (const auto& e : j_.at("enum")) { f_.enums[e.at("value")] = e; }
    }
}

//-----------------------------------------------------------------------
void from_json(const json& j_, BaseField& f_)
{
    f_.name = j_.at("name");
    f_.description = j_.at("description").is_null() ? "" : j_.at("description");

    const auto itrFieldTypeMapping = FieldTypeEnumLookup.find(j_.at("type"));
    f_.type = itrFieldTypeMapping != FieldTypeEnumLookup.end() ? itrFieldTypeMapping->second : FIELD_TYPE::UNKNOWN;

    if (j_.find("conversionString") != j_.end())
    {
        if (j_.at("conversionString").is_null()) { f_.conversion = ""; }
        else { f_.SetConversion(j_.at("conversionString")); }
    }

    f_.dataType = j_.at("dataType");
}

//-----------------------------------------------------------------------
void from_json(const json& j_, EnumField& f_)
{
    from_json(j_, static_cast<BaseField&>(f_));

    if (j_.at("enumID").is_null()) { throw std::runtime_error("Invalid enum ID - cannot be NULL.  JsonDB file is likely corrupted."); }

    f_.enumId = j_.at("enumID");
}

//-----------------------------------------------------------------------
void from_json(const json& j_, ArrayField& fd_)
{
    from_json(j_, static_cast<BaseField&>(fd_));

    fd_.arrayLength = j_.at("arrayLength");
    fd_.dataType = j_.at("dataType");
}

//-----------------------------------------------------------------------
void from_json(const json& j_, FieldArrayField& fd_)
{
    from_json(j_, static_cast<BaseField&>(fd_));

    fd_.arrayLength = j_.at("arrayLength").is_null() ? 0 : static_cast<uint32_t>(j_.at("arrayLength"));
    fd_.fieldSize = fd_.arrayLength * ParseFields(j_.at("fields"), fd_.fields);
}

//-----------------------------------------------------------------------
void from_json(const json& j_, MessageDefinition& md_)
{
    md_._id = j_.at("_id");
    md_.logID = j_.at("messageID"); // this was "logID"
    md_.name = j_.at("name");
    md_.description = j_.at("description").is_null() ? "" : j_.at("description");
    md_.latestMessageCrc = std::stoul(j_.at("latestMsgDefCrc").get<std::string>());

    for (const auto& fields : j_.at("fields").items())
    {
        uint32_t defCrc = std::stoul(fields.key());
        md_.fields[defCrc];
        ParseFields(fields.value(), md_.fields[defCrc]);
    }
}

//-----------------------------------------------------------------------
void from_json(const json& j_, EnumDefinition& ed_)
{
    ed_._id = j_.at("_id");
    ed_.name = j_.at("name");
    ParseEnumerators(j_.at("enumerators"), ed_.enumerators);
}

//-----------------------------------------------------------------------
uint32_t ParseFields(const json& j_, std::vector<BaseField*>& vFields_)
{
    uint32_t uiFieldSize = 0;
    for (const auto& field : j_)
    {
        const auto sFieldType = field.at("type").get<std::string>();
        const auto stDataType = field.at("dataType").get<BaseDataType>();

        if (sFieldType == "SIMPLE")
        {
            auto pstField = new BaseField;
            *pstField = field;
            vFields_.push_back(pstField);
            uiFieldSize += stDataType.length;
        }
        else if (sFieldType == "ENUM")
        {
            auto pstField = new EnumField;
            *pstField = field;
            pstField->length = stDataType.length;
            vFields_.push_back(pstField);
            uiFieldSize += stDataType.length;
        }
        else if (sFieldType == "FIXED_LENGTH_ARRAY" || sFieldType == "VARIABLE_LENGTH_ARRAY" || sFieldType == "STRING")
        {
            auto pstField = new ArrayField;
            *pstField = field;
            vFields_.push_back(pstField);
            uint32_t uiArrayLength = field.at("arrayLength").get<uint32_t>();
            uiFieldSize += (stDataType.length * uiArrayLength);
        }
        else if (sFieldType == "FIELD_ARRAY")
        {
            auto pstField = new FieldArrayField;
            *pstField = field;
            vFields_.push_back(pstField);
        }
        else { throw std::runtime_error("Could not find field type"); }
    }
    return uiFieldSize;
}

//-----------------------------------------------------------------------
void ParseEnumerators(const json& j_, std::vector<EnumDataType>& vEnumerators_)
{
    for (const auto& enumerator : j_) { vEnumerators_.push_back(enumerator); }
}

//-----------------------------------------------------------------------
template <typename T> void JsonReader::LoadFile(T filePath_)
{
    try
    {
        std::fstream jsonFile;
        jsonFile.open(std::filesystem::path(filePath_), std::ios::in);
        json jDefinitions = json::parse(jsonFile);

        vMessageDefinitions.clear();
        for (const auto& msg : jDefinitions["messages"])
        {
            vMessageDefinitions.push_back(msg); // The JSON object is converted to a MessageDefinition object here
        }

        vEnumDefinitions.clear();
        for (const auto& enm : jDefinitions["enums"])
        {
            vEnumDefinitions.push_back(enm); // The JSON object is converted to a EnumDefinition object here
        }

        GenerateMappings();
    }
    catch (std::exception& e)
    {
        throw JsonReaderFailure(__func__, __FILE__, __LINE__, filePath_, e.what());
    }
}

//-----------------------------------------------------------------------
template <> void JsonReader::LoadFile<std::string>(std::string filePath_)
{
    try
    {
        std::fstream jsonFile;
        jsonFile.open(filePath_, std::ios::in);
        json jDefinitions = json::parse(jsonFile);

        vMessageDefinitions.clear();

        for (auto& msg : jDefinitions["messages"])
        {
            vMessageDefinitions.push_back(msg); // The JSON object is converted to a MessageDefinition object here
        }

        vEnumDefinitions.clear();
        for (auto& enm : jDefinitions["enums"])
        {
            vEnumDefinitions.push_back(enm); // The JSON object is converted to a EnumDefinition object here
        }

        GenerateMappings();
    }
    catch (std::exception& e)
    {
        throw JsonReaderFailure(__func__, __FILE__, __LINE__, filePath_.c_str(), e.what());
    }
}

//-----------------------------------------------------------------------
template <typename T> void JsonReader::AppendMessages(T filePath_)
{
    try
    {
        std::fstream jsonFile;
        jsonFile.open(std::filesystem::path(filePath_));
        json jDefinitions = json::parse(jsonFile);

        for (const auto& msg : jDefinitions["messages"])
        {
            // Convert JSON object to an MessageDefinition object
            const MessageDefinition msgDef(msg);
            RemoveMessage(msgDef.logID, false);
            vMessageDefinitions.push_back(msg);
        }

        for (const auto& enm : jDefinitions["enums"])
        {
            // Convert JSON object to an EnumDefinition object
            const EnumDefinition enmDef(enm);
            RemoveEnumeration(enmDef.name, false);
            vEnumDefinitions.push_back(enm);
        }

        GenerateMappings();
    }
    catch (std::exception& e)
    {
        throw JsonReaderFailure(__func__, __FILE__, __LINE__, filePath_, e.what());
    }
}

//-----------------------------------------------------------------------
template <typename T> void JsonReader::AppendEnumerations(T filePath_)
{
    try
    {
        std::fstream jsonFile;
        jsonFile.open(std::filesystem::path(filePath_));
        json jDefinitions = json::parse(jsonFile);

        for (const auto& enm : jDefinitions["enums"])
        {
            vEnumDefinitions.push_back(enm); // The JSON object is converted to a EnumDefinition object here
        }

        GenerateMappings();
    }
    catch (std::exception& e)
    {
        throw JsonReaderFailure(__func__, __FILE__, __LINE__, filePath_, e.what());
    }
}

// explicit template instantiations
template void JsonReader::LoadFile<std::u32string>(std::u32string);
template void JsonReader::LoadFile<char*>(char*);
template void JsonReader::LoadFile<const char*>(const char*);

template void JsonReader::AppendMessages<std::string>(std::string);
template void JsonReader::AppendMessages<std::u32string>(std::u32string);
template void JsonReader::AppendMessages<char*>(char*);

template void JsonReader::AppendEnumerations<std::string>(std::string);
template void JsonReader::AppendEnumerations<std::u32string>(std::u32string);
template void JsonReader::AppendEnumerations<char*>(char*);

//-----------------------------------------------------------------------
void JsonReader::RemoveMessage(const uint32_t iMsgId_, const bool bGenerateMappings_)
{
    auto iTer = GetMessageIt(iMsgId_);

    if (iTer != vMessageDefinitions.end())
    {
        RemoveMessageMapping(*iTer);
        vMessageDefinitions.erase(iTer);
    }

    if (bGenerateMappings_) { GenerateMappings(); }
}

//-----------------------------------------------------------------------
void JsonReader::RemoveEnumeration(const std::string& strEnumeration_, const bool bGenerateMappings_)
{
    const auto iTer = GetEnumIt(strEnumeration_);

    if (iTer != vEnumDefinitions.end())
    {
        RemoveEnumerationMapping(*iTer);
        vEnumDefinitions.erase(iTer);
    }

    if (bGenerateMappings_) { GenerateMappings(); }
}

//-----------------------------------------------------------------------
void JsonReader::ParseJson(const std::string& strJsonData_)
{
    json jDefinitions = json::parse(strJsonData_);

    vMessageDefinitions.clear();
    for (const auto& msg : jDefinitions["logs"])
    {
        vMessageDefinitions.push_back(msg); // The JSON object is converted to a MessageDefinition object here
    }

    vEnumDefinitions.clear();
    for (const auto& enm : jDefinitions["enums"])
    {
        vEnumDefinitions.push_back(enm); // The JSON object is converted to a EnumDefinition object here
    }

    GenerateMappings();
}

//-----------------------------------------------------------------------
uint32_t JsonReader::MsgNameToMsgId(std::string sMsgName_) const
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
    const MessageDefinition* pclMessageDef = GetMsgDef(sMsgName_);
    if (pclMessageDef)
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

    return pclMessageDef ? CreateMsgId(pclMessageDef->logID, uiSiblingId, uiMsgFormat, uiResponse) : 0;
}

// -------------------------------------------------------------------------------------------------------
std::string JsonReader::MsgIdToMsgName(const uint32_t uiMessageId_) const
{
    uint16_t usLogId = 0;
    uint32_t uiSiblingId = 0;
    uint32_t uiMessageFormat = 0;
    uint32_t uiResponse = 0;

    UnpackMsgId(uiMessageId_, usLogId, uiSiblingId, uiMessageFormat, uiResponse);

    const MessageDefinition* pstMessageDefinition = GetMsgDef(usLogId);
    std::string strMessageName = pstMessageDefinition ? pstMessageDefinition->name : GetEnumString(vEnumDefinitions.data(), usLogId);

    std::string strMessageFormatSuffix = uiResponse                                                         ? "R"
                                         : uiMessageFormat == static_cast<uint32_t>(MESSAGE_FORMAT::BINARY) ? "B"
                                         : uiMessageFormat == static_cast<uint32_t>(MESSAGE_FORMAT::ASCII)
                                             ? "A"
                                             : ""; // default to abbreviated ASCII format

    if (uiSiblingId) { strMessageFormatSuffix.append("_").append(std::to_string(uiSiblingId)); }

    return strMessageName.append(strMessageFormatSuffix);
}

//-----------------------------------------------------------------------
const MessageDefinition* JsonReader::GetMsgDef(const std::string& strMsgName_) const
{
    const auto it = mMessageName.find(strMsgName_);
    return it != mMessageName.end() ? it->second : nullptr;
}

//-----------------------------------------------------------------------
// TODO: need to look into the map and find the right crc and return the msg def for that CRC
const MessageDefinition* JsonReader::GetMsgDef(const int32_t iMsgId_) const
{
    const auto it = mMessageId.find(iMsgId_);
    return it != mMessageId.end() ? it->second : nullptr;
}

// -------------------------------------------------------------------------------------------------------
const std::vector<BaseField*>* MessageDefinition::GetMsgDefFromCrc(const std::shared_ptr<spdlog::logger>& pclLogger_, uint32_t& uiMsgDefCrc_) const
{
    // If we can't find the correct CRC just default to the latest.
    if (fields.find(uiMsgDefCrc_) == fields.end())
    {
        pclLogger_->info("Log DB is missing the log definition {} - {}.  Defaulting to newest version of the log definition.", name, uiMsgDefCrc_);
        uiMsgDefCrc_ = latestMessageCrc;
    }

    return &fields.at(uiMsgDefCrc_);
}

} // namespace novatel::edie
