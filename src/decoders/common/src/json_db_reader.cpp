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

#include <future>

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
    if (j_.find("arrayLengthRef") != j_.end()) { fd_.arrayLengthRef = j_.at("arrayLengthRef").is_null() ? "" : j_.at("arrayLengthRef"); }
}

//-----------------------------------------------------------------------
void from_json(const json& j_, MessageDefinition& md_)
{
    md_._id = j_.at("_id");
    md_.logID = j_.at("messageID"); // this was "logID"
    md_.name = j_.at("name");
    md_.description = j_.at("description").is_null() ? "" : j_.at("description");
    md_.latestMessageCrc = std::stoul(j_.at("latestMsgDefCrc").get<std::string>());
    md_.messageStyle = j_.at("messageStyle").is_null() ? "" : j_.at("messageStyle");

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

    // Parse enumerators into the vector
    ParseEnumerators(j_.at("enumerators"), ed_.enumerators);

    // Populate the lookup maps
    for (const auto& enumerator : ed_.enumerators)
    {
        ed_.nameValue[enumerator.name] = enumerator.value;
        ed_.valueName[enumerator.value] = enumerator.name;
        ed_.descriptionValue[enumerator.description] = enumerator.value;
    }
}

//-----------------------------------------------------------------------
uint32_t ParseFields(const json& j_, std::vector<BaseField::Ptr>& vFields_)
{
    uint32_t uiFieldSize = 0;
    vFields_.reserve(j_.size());

    for (const auto& field : j_)
    {
        const auto sFieldType = field.at("type").get<std::string_view>();
        const auto stDataType = field.at("dataType").get<BaseDataType>();

        if (sFieldType == "SIMPLE")
        {
            vFields_.emplace_back(std::make_shared<BaseField>(field));
            uiFieldSize += stDataType.length;
        }
        else if (sFieldType == "ENUM")
        {
            auto pstField = std::make_shared<EnumField>(field);
            pstField->length = stDataType.length;
            vFields_.emplace_back(pstField);
            uiFieldSize += stDataType.length;
        }
        else if (sFieldType == "FIXED_LENGTH_ARRAY" || sFieldType == "VARIABLE_LENGTH_ARRAY" || sFieldType == "STRING")
        {
            vFields_.emplace_back(std::make_shared<ArrayField>(field));
            uint32_t uiArrayLength = field.at("arrayLength").get<uint32_t>();
            uiFieldSize += stDataType.length * uiArrayLength;
        }
        else if (sFieldType == "FIELD_ARRAY") { vFields_.emplace_back(std::make_shared<FieldArrayField>(field)); }
        else { throw std::runtime_error("Could not find field type"); }
    }
    return uiFieldSize;
}

//-----------------------------------------------------------------------
void ParseEnumerators(const json& j_, std::vector<EnumDataType>& vEnumerators_)
{
    vEnumerators_.reserve(j_.size());
    for (const auto& enumerator : j_) { vEnumerators_.emplace_back(enumerator); }
}

//-----------------------------------------------------------------------
std::vector<MessageDefinition::ConstPtr> ProcessMessageDefinitions(const json& jArray)
{
    const auto& data = jArray["messages"];
    std::vector<MessageDefinition::ConstPtr> res;
    res.reserve(data.size());

    for (const auto& it : data) { res.emplace_back(std::make_shared<MessageDefinition>(it)); }

    return res;
}

//-----------------------------------------------------------------------
std::vector<EnumDefinition::ConstPtr> ProcessEnumDefinitions(const json& jArray)
{
    const auto& data = jArray["enums"];
    std::vector<EnumDefinition::ConstPtr> res;
    res.reserve(data.size());

    for (const auto& it : data) { res.emplace_back(std::make_shared<EnumDefinition>(it)); }

    return res;
}

//-----------------------------------------------------------------------
MessageDatabase::Ptr LoadJsonDbFile(const std::filesystem::path& filePath_)
{
    try
    {
        std::ifstream jsonFile(filePath_);
        auto json = json::parse(jsonFile);

        auto messageFuture = std::async(std::launch::async, ProcessMessageDefinitions, std::cref(json));
        auto enumFuture = std::async(std::launch::async, ProcessEnumDefinitions, std::cref(json));

        return std::make_shared<MessageDatabase>(messageFuture.get(), enumFuture.get());
    }
    catch (const std::exception& e)
    {
        throw JsonDbReaderFailure(__func__, __FILE__, __LINE__, filePath_, e.what());
    }
}

//-----------------------------------------------------------------------
MessageDatabase::Ptr ParseJsonDb(std::string_view strJsonData_)
{
    try
    {
        auto json = json::parse(strJsonData_);

        auto messageFuture = std::async(std::launch::async, ProcessMessageDefinitions, std::cref(json));
        auto enumFuture = std::async(std::launch::async, ProcessEnumDefinitions, std::cref(json));

        return std::make_shared<MessageDatabase>(messageFuture.get(), enumFuture.get());
    }
    catch (const std::exception& e)
    {
        throw JsonDbReaderFailure(__func__, __FILE__, __LINE__, strJsonData_, e.what());
    }
}

} // namespace novatel::edie
