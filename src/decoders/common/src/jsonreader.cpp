////////////////////////////////////////////////////////////////////////
//
// COPYRIGHT NovAtel Inc, 2022. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////
//                            DESCRIPTION
//
//! \file jsonreader.cpp
//! \brief Class to parse JSON UI DB files from NovAtel.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "jsonreader.hpp"

namespace novatel::edie {

//-----------------------------------------------------------------------
void from_json(const json& j, novatel::edie::EnumDataType& f)
{
    f.value = j.at("value");
    f.name = j.at("name");
    f.description = j.at("description").is_null() ? "" : j.at("description");
}

//-----------------------------------------------------------------------
void from_json(const json& j, novatel::edie::BaseDataType& f)
{
    auto itrDataTypeMapping = DataTypeEnumLookup.find(j.at("name"));
    f.name = itrDataTypeMapping != DataTypeEnumLookup.end() ? itrDataTypeMapping->second : DATA_TYPE::UNKNOWN;
    f.length = j.at("length");
    f.description = j.at("description").is_null() ? "" : j.at("description");
}

//-----------------------------------------------------------------------
void from_json(const json& j, novatel::edie::SimpleDataType& f)
{
    from_json(j, static_cast<BaseDataType&>(f));

    if (j.find("enum") != j.end())
    {
        for (const auto& e : j.at("enum")) { f.enums[e.at("value")] = e; }
    }
}

//-----------------------------------------------------------------------
void from_json(const json& j, novatel::edie::BaseField& f)
{
    f.name = j.at("name");
    f.description = j.at("description").is_null() ? "" : j.at("description");

    auto itrFieldTypeMapping = FieldTypeEnumLookup.find(j.at("type"));
    f.type = itrFieldTypeMapping != FieldTypeEnumLookup.end() ? itrFieldTypeMapping->second : FIELD_TYPE::UNKNOWN;

    if (j.find("conversionString") != j.end())
    {
        if (j.at("conversionString").is_null())
            f.conversion = "";
        else
            f.setConversion(j.at("conversionString"));
    }

    f.dataType = j.at("dataType");
}

//-----------------------------------------------------------------------
void from_json(const json& j, novatel::edie::EnumField& f)
{
    from_json(j, static_cast<BaseField&>(f));

    if (j.at("enumID").is_null()) throw std::runtime_error("Invalid enum ID - cannot be NULL.  JsonDB file is likely corrupted.");

    f.enumID = j.at("enumID");
}

//-----------------------------------------------------------------------
void from_json(const json& j, novatel::edie::ArrayField& fd)
{
    from_json(j, static_cast<BaseField&>(fd));

    fd.arrayLength = j.at("arrayLength");
    fd.dataType = j.at("dataType");
}

//-----------------------------------------------------------------------
void from_json(const json& j, novatel::edie::FieldArrayField& fd)
{
    from_json(j, static_cast<BaseField&>(fd));

    fd.arrayLength = j.at("arrayLength").is_null() ? 0 : static_cast<uint32_t>(j.at("arrayLength"));
    fd.fieldSize = fd.arrayLength * parse_fields(j.at("fields"), fd.fields);
}

//-----------------------------------------------------------------------
void from_json(const json& j, MessageDefinition& md)
{
    md._id = j.at("_id");
    md.logID = j.at("messageID"); // this was "logID"
    md.name = j.at("name");
    md.description = j.at("description").is_null() ? "" : j.at("description");
    md.latestMessageCrc = std::stoul(j.at("latestMsgDefCrc").get<std::string>());

    for (const auto& fields : j.at("fields").items())
    {
        uint32_t defCRC = std::stoul(fields.key());
        md.fields[defCRC];
        parse_fields(fields.value(), md.fields[defCRC]);
    }
}

//-----------------------------------------------------------------------
void from_json(const json& j, EnumDefinition& ed)
{
    ed._id = j.at("_id");
    ed.name = j.at("name");
    parse_enumerators(j.at("enumerators"), ed.enumerators);
}

//-----------------------------------------------------------------------
uint32_t parse_fields(const json& j, std::vector<novatel::edie::BaseField*>& vFields)
{
    uint32_t uiFieldSize = 0;
    for (const auto& field : j)
    {
        std::string sFieldType = field.at("type").get<std::string>();
        novatel::edie::BaseDataType stDataType = field.at("dataType").get<novatel::edie::BaseDataType>();

        if (sFieldType == "SIMPLE")
        {
            novatel::edie::BaseField* pstField = new novatel::edie::BaseField;
            *pstField = field;
            vFields.push_back(pstField);
            uiFieldSize += stDataType.length;
        }
        else if (sFieldType == "ENUM")
        {
            novatel::edie::EnumField* pstField = new novatel::edie::EnumField;
            *pstField = field;
            pstField->length = stDataType.length;
            vFields.push_back(pstField);
            uiFieldSize += stDataType.length;
        }
        else if (sFieldType == "FIXED_LENGTH_ARRAY" || sFieldType == "VARIABLE_LENGTH_ARRAY" || sFieldType == "STRING")
        {
            novatel::edie::ArrayField* pstField = new novatel::edie::ArrayField;
            *pstField = field;
            vFields.push_back(pstField);
            uint32_t uiArrayLength = field.at("arrayLength").get<uint32_t>();
            uiFieldSize += (stDataType.length * uiArrayLength);
        }
        else if (sFieldType == "FIELD_ARRAY")
        {
            novatel::edie::FieldArrayField* pstField = new novatel::edie::FieldArrayField;
            *pstField = field;
            vFields.push_back(pstField);
        }
        else
            throw std::runtime_error("Could not find field type");
    }
    return uiFieldSize;
}

//-----------------------------------------------------------------------
void parse_enumerators(const json& j, std::vector<novatel::edie::EnumDataType>& vEnumerators)
{
    for (const auto& enumerator : j) { vEnumerators.push_back(enumerator); }
}

} // namespace novatel::edie

//-----------------------------------------------------------------------
template <typename T> void JsonReader::LoadFile(T filePath)
{
    try
    {
        std::fstream json_file;
        json_file.open(std::filesystem::path(filePath), std::ios::in);
        json jDefinitions = json::parse(json_file);

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
        throw novatel::edie::JsonReaderFailure(__func__, __FILE__, __LINE__, filePath, e.what());
    }
}

//-----------------------------------------------------------------------
template <> void JsonReader::LoadFile<std::string>(std::string filePath)
{
    try
    {
        std::fstream json_file;
        json_file.open(filePath, std::ios::in);
        json jDefinitions = json::parse(json_file);

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
        throw novatel::edie::JsonReaderFailure(__func__, __FILE__, __LINE__, filePath.c_str(), e.what());
    }
}

//-----------------------------------------------------------------------
template <typename T> void JsonReader::AppendMessages(T filePath_)
{
    try
    {
        std::fstream json_file;
        json_file.open(std::filesystem::path(filePath_));
        json jDefinitions = json::parse(json_file);

        for (const auto& msg : jDefinitions["messages"])
        {
            const novatel::edie::MessageDefinition msgdef = msg;

            RemoveMessage(msgdef.logID, false);

            vMessageDefinitions.push_back(msg); // The JSON object is converted to a MessageDefinition object here
        }

        for (const auto& enm : jDefinitions["enums"])
        {
            const novatel::edie::EnumDefinition enmdef = enm;

            RemoveEnumeration(enmdef.name, false);

            vEnumDefinitions.push_back(enm); // The JSON object is converted to a EnumDefinition object here
        }

        GenerateMappings();
    }
    catch (std::exception& e)
    {
        throw novatel::edie::JsonReaderFailure(__func__, __FILE__, __LINE__, filePath_, e.what());
    }
}

//-----------------------------------------------------------------------
template <typename T> void JsonReader::AppendEnumerations(T filePath_)
{
    try
    {
        std::fstream json_file;
        json_file.open(std::filesystem::path(filePath_));
        json jDefinitions = json::parse(json_file);

        for (const auto& enm : jDefinitions["enums"])
        {
            vEnumDefinitions.push_back(enm); // The JSON object is converted to a EnumDefinition object here
        }

        GenerateMappings();
    }
    catch (std::exception& e)
    {
        throw novatel::edie::JsonReaderFailure(__func__, __FILE__, __LINE__, filePath_, e.what());
    }
}

// explicit template instantiations
template void JsonReader::LoadFile<std::u32string>(std::u32string filePath);
template void JsonReader::LoadFile<char*>(char* filePath);
template void JsonReader::LoadFile<const char*>(const char* filePath);

template void JsonReader::AppendMessages<std::string>(std::string filePath);
template void JsonReader::AppendMessages<std::u32string>(std::u32string filePath);
template void JsonReader::AppendMessages<char*>(char* filePath);

template void JsonReader::AppendEnumerations<std::string>(std::string filePath);
template void JsonReader::AppendEnumerations<std::u32string>(std::u32string filePath);
template void JsonReader::AppendEnumerations<char*>(char* filePath);

//-----------------------------------------------------------------------
void JsonReader::RemoveMessage(uint32_t iMsgId_, bool bGenerateMappings_)
{
    std::vector<novatel::edie::MessageDefinition>::iterator iTer;

    iTer = GetMessageIt(iMsgId_);

    if (iTer != vMessageDefinitions.end())
    {
        RemoveMessageMapping(*iTer);
        vMessageDefinitions.erase(iTer);
    }

    if (bGenerateMappings_) GenerateMappings();
}

//-----------------------------------------------------------------------
void JsonReader::RemoveEnumeration(std::string strEnumeration_, bool bGenerateMappings_)
{
    std::vector<novatel::edie::EnumDefinition>::iterator iTer;
    iTer = GetEnumIt(strEnumeration_);

    if (iTer != vEnumDefinitions.end())
    {
        RemoveEnumerationMapping(*iTer);
        vEnumDefinitions.erase(iTer);
    }

    if (bGenerateMappings_) GenerateMappings();
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
    uint32_t uiSiblingID = 0;
    uint32_t uiMsgFormat;
    uint32_t uiResponse;

    // Ingest the sibling information, i.e. the _1 from LOGNAMEA_1
    if (sMsgName_.find_last_of('_') != std::string::npos && sMsgName_.find_last_of('_') == sMsgName_.size() - 2)
    {
        uiSiblingID = static_cast<uint32_t>(ToDigit(sMsgName_.back()));
        sMsgName_.resize(sMsgName_.size() - 2);
    }

    // If this is an abbrev msg (no format information), we will be able to find the MsgDef
    const novatel::edie::MessageDefinition* pclMessageDef = GetMsgDef(sMsgName_);
    if (pclMessageDef)
    {
        uiResponse = static_cast<uint32_t>(false);
        uiMsgFormat = static_cast<uint32_t>(novatel::edie::MESSAGEFORMAT::ABBREV);

        return CreateMsgID(pclMessageDef->logID, uiSiblingID, uiMsgFormat, uiResponse);
    }

    switch (sMsgName_.back())
    {
    case 'R': // ASCII Response
        uiResponse = static_cast<uint32_t>(true);
        uiMsgFormat = static_cast<uint32_t>(novatel::edie::MESSAGEFORMAT::ASCII);
        sMsgName_.pop_back();
        break;
    case 'A': // ASCII
        uiResponse = static_cast<uint32_t>(false);
        uiMsgFormat = static_cast<uint32_t>(novatel::edie::MESSAGEFORMAT::ASCII);
        sMsgName_.pop_back();
        break;
    case 'B': // Binary
        uiResponse = static_cast<uint32_t>(false);
        uiMsgFormat = static_cast<uint32_t>(novatel::edie::MESSAGEFORMAT::BINARY);
        sMsgName_.pop_back();
        break;
    default: // Abbreviated ASCII
        uiResponse = static_cast<uint32_t>(false);
        uiMsgFormat = static_cast<uint32_t>(novatel::edie::MESSAGEFORMAT::ABBREV);
        break;
    }

    pclMessageDef = GetMsgDef(sMsgName_);

    return pclMessageDef ? CreateMsgID(pclMessageDef->logID, uiSiblingID, uiMsgFormat, uiResponse) : 0;
}

// -------------------------------------------------------------------------------------------------------
std::string JsonReader::MsgIdToMsgName(const uint32_t uiMessageID_) const
{
    uint16_t usLogID = 0;
    uint32_t uiSiblingID = 0;
    uint32_t uiMessageFormat = 0;
    uint32_t uiResponse = 0;

    UnpackMsgID(uiMessageID_, usLogID, uiSiblingID, uiMessageFormat, uiResponse);

    const novatel::edie::MessageDefinition* pstMessageDefinition = GetMsgDef(usLogID);
    std::string strMessageName = pstMessageDefinition ? pstMessageDefinition->name : GetEnumString(vEnumDefinitions.data(), usLogID);

    std::string strMessageFormatSuffix = uiResponse                                                                       ? "R"
                                         : uiMessageFormat == static_cast<uint32_t>(novatel::edie::MESSAGEFORMAT::BINARY) ? "B"
                                         : uiMessageFormat == static_cast<uint32_t>(novatel::edie::MESSAGEFORMAT::ASCII)
                                             ? "A"
                                             : ""; // default to abbreviated ASCII format

    if (uiSiblingID) strMessageFormatSuffix.append("_").append(std::to_string(uiSiblingID));

    return strMessageName.append(strMessageFormatSuffix);
}

//-----------------------------------------------------------------------
const novatel::edie::MessageDefinition* JsonReader::GetMsgDef(const std::string& strMsgName_) const
{
    const auto it = mMessageName.find(strMsgName_);
    return it != mMessageName.end() ? it->second : nullptr;
}

//-----------------------------------------------------------------------
// TODO need to look into the map and find the right crc and return the msg def for that CRC
const novatel::edie::MessageDefinition* JsonReader::GetMsgDef(int32_t iMsgID) const
{
    const auto it = mMessageID.find(iMsgID);
    return it != mMessageID.end() ? it->second : nullptr;
}

// -------------------------------------------------------------------------------------------------------
std::vector<novatel::edie::BaseField*> const* novatel::edie::MessageDefinition::GetMsgDefFromCRC(std::shared_ptr<spdlog::logger> pclLogger_,
                                                                                                 uint32_t& uiMsgDefCRC_) const
{
    // If we can't find the correct CRC just default to the latest.
    if (fields.count(uiMsgDefCRC_) == 0)
    {
        pclLogger_->info("Log DB is missing the log definition {} - {}.  Defaulting to newest version of the "
                         "log definition.",
                         name, uiMsgDefCRC_);
        uiMsgDefCRC_ = latestMessageCrc;
    }
    return &fields.at(uiMsgDefCRC_);
}
