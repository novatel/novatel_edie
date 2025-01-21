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
// ! \file json_db_reader.cpp
// ===============================================================================

#include "novatel_edie/decoders/common/json_db_reader.hpp"

#include <nlohmann/json.hpp>

#include "novatel_edie/decoders/common/common.hpp"

namespace novatel::edie {

using json = nlohmann::json;

// Forward declaration of from_json
void from_json(const json& j_, EnumDataType& f_);
void from_json(const json& j_, BaseDataType& f_);
void from_json(const json& j_, SimpleDataType& f_);
void from_json(const json& j_, BaseField& f_);
void from_json(const json& j_, EnumField& f_);
void from_json(const json& j_, ArrayField& fd_);
void from_json(const json& j_, FieldArrayField& fd_);
void from_json(const json& j_, MessageDefinition& md_);
void from_json(const json& j_, EnumDefinition& ed_);

// Forward declaration of parse_fields and parse_enumerators
uint32_t ParseFields(const json& j_, std::vector<BaseField::Ptr>& vFields_);
void ParseEnumerators(const json& j_, std::vector<EnumDataType>& vEnumerators_);

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

    if (j_.at("enumID").is_null()) { throw std::runtime_error("Invalid enum ID - cannot be NULL. JsonDB file is likely corrupted."); }

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
uint32_t ParseFields(const json& j_, std::vector<BaseField::Ptr>& vFields_)
{
    uint32_t uiFieldSize = 0;
    for (const auto& field : j_)
    {
        const auto sFieldType = field.at("type").get<std::string>();
        const auto stDataType = field.at("dataType").get<BaseDataType>();

        if (sFieldType == "SIMPLE")
        {
            auto pstField = std::make_shared<BaseField>();
            *pstField = field;
            vFields_.push_back(pstField);
            uiFieldSize += stDataType.length;
        }
        else if (sFieldType == "ENUM")
        {
            auto pstField = std::make_shared<EnumField>();
            *pstField = field;
            pstField->length = stDataType.length;
            vFields_.push_back(pstField);
            uiFieldSize += stDataType.length;
        }
        else if (sFieldType == "FIXED_LENGTH_ARRAY" || sFieldType == "VARIABLE_LENGTH_ARRAY" || sFieldType == "STRING")
        {
            auto pstField = std::make_shared<ArrayField>();
            *pstField = field;
            vFields_.push_back(pstField);
            uint32_t uiArrayLength = field.at("arrayLength").get<uint32_t>();
            uiFieldSize += (stDataType.length * uiArrayLength);
        }
        else if (sFieldType == "FIELD_ARRAY")
        {
            auto pstField = std::make_shared<FieldArrayField>();
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
MessageDatabase::Ptr JsonDbReader::LoadFile(const std::filesystem::path& filePath_)
{
    try
    {
        std::fstream jsonFile;
        jsonFile.open(filePath_, std::ios::in);
        json jDefinitions = json::parse(jsonFile);

        std::vector<MessageDefinition::ConstPtr> vMessageDefinitions;
        std::vector<EnumDefinition::ConstPtr> vEnumDefinitions;

        // The JSON object is converted to a MessageDefinition object here
        for (auto& msg : jDefinitions["messages"]) { vMessageDefinitions.push_back(std::make_shared<MessageDefinition>(msg)); }

        // The JSON object is converted to a EnumDefinition object here
        for (auto& enm : jDefinitions["enums"]) { vEnumDefinitions.push_back(std::make_shared<EnumDefinition>(enm)); }
        return std::make_shared<MessageDatabase>(vMessageDefinitions, vEnumDefinitions);
    }
    catch (std::exception& e)
    {
        throw JsonDbReaderFailure(__func__, __FILE__, __LINE__, filePath_.c_str(), e.what());
    }
}

//-----------------------------------------------------------------------
MessageDatabase::Ptr JsonDbReader::Parse(std::string_view strJsonData_)
{
    try
    {
        json jDefinitions = json::parse(strJsonData_);

        std::vector<MessageDefinition::ConstPtr> vMessageDefinitions;
        std::vector<EnumDefinition::ConstPtr> vEnumDefinitions;

        // The JSON object is converted to a MessageDefinition object here
        for (auto& msg : jDefinitions["messages"]) { vMessageDefinitions.push_back(std::make_shared<MessageDefinition>(msg)); }

        // The JSON object is converted to a EnumDefinition object here
        for (auto& enm : jDefinitions["enums"]) { vEnumDefinitions.push_back(std::make_shared<EnumDefinition>(enm)); }
        return std::make_shared<MessageDatabase>(vMessageDefinitions, vEnumDefinitions);
    }
    catch (std::exception& e)
    {
        throw JsonDbReaderFailure(__func__, __FILE__, __LINE__, "", e.what());
    }
}

//-----------------------------------------------------------------------
void JsonDbReader::AppendMessages(const MessageDatabase::Ptr& messageDb_, const std::filesystem::path& filePath_)
{
    try
    {
        std::fstream jsonFile;
        jsonFile.open(std::filesystem::path(filePath_));
        json jDefinitions = json::parse(jsonFile);

        std::vector<MessageDefinition::ConstPtr> vMessageDefinitions;
        for (const auto& msg : jDefinitions["messages"])
        {
            // Convert JSON object to an MessageDefinition object
            vMessageDefinitions.push_back(std::make_shared<MessageDefinition>(msg));
        }
        messageDb_->AppendMessages(vMessageDefinitions, false);

        std::vector<EnumDefinition::ConstPtr> vEnumDefinitions;
        for (const auto& enm : jDefinitions["enums"])
        {
            // Convert JSON object to an EnumDefinition object
            vEnumDefinitions.push_back(std::make_shared<EnumDefinition>(enm));
        }
        messageDb_->AppendEnumerations(vEnumDefinitions);
    }
    catch (std::exception& e)
    {
        throw JsonDbReaderFailure(__func__, __FILE__, __LINE__, filePath_, e.what());
    }
}

//-----------------------------------------------------------------------
void JsonDbReader::AppendEnumerations(const MessageDatabase::Ptr& messageDb_, const std::filesystem::path& filePath_)
{
    try
    {
        std::fstream jsonFile;
        jsonFile.open(std::filesystem::path(filePath_));
        json jDefinitions = json::parse(jsonFile);

        std::vector<EnumDefinition::ConstPtr> vEnumDefinitions;
        for (const auto& enm : jDefinitions["enums"])
        {
            // Convert JSON object to an EnumDefinition object
            vEnumDefinitions.push_back(std::make_shared<EnumDefinition>(enm));
        }
        messageDb_->AppendEnumerations(vEnumDefinitions);
    }
    catch (std::exception& e)
    {
        throw JsonDbReaderFailure(__func__, __FILE__, __LINE__, filePath_, e.what());
    }
}

} // namespace novatel::edie
