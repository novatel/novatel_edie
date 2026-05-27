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
using AlignFunction = std::function<size_t(const size_t, const uintptr_t, const uintptr_t)>;

// Forward declaration of from_json
void from_json(const json& j_, EnumDataType& f_);
void from_json(const json& j_, SimpleDataType& f_);
void from_json(const json& j_, BaseField& f_);
void from_json(const json& j_, EnumField& f_);
void from_json(const json& j_, ArrayField& fd_);
void from_json(const json& j_, FieldArrayField& fd_);
void from_json(const json& j_, EnumDefinition& ed_);
void from_json(const json& j_, DbMetadata& dbm_);

// Forward declaration of parse_fields and parse_enumerators
uint32_t ParseFields(const json& j_, FieldInfo& vFields_, const AlignFunction& alignFn_ = MessageDatabase::NoAlign);
void ParseEnumerators(const json& j_, std::vector<EnumDataType>& vEnumerators_);

//-----------------------------------------------------------------------
void from_json(const json& j_, EnumDataType& f_)
{
    f_.value = j_.at("value");
    f_.name = j_.at("name");
    f_.description = j_.at("description").is_null() ? "" : j_.at("description");
}

//-----------------------------------------------------------------------
void from_json(const json& j_, SimpleDataType& f_)
{
    auto itrDataTypeMapping = DataTypeEnumLookup.find(j_.at("name"));
    f_.name = itrDataTypeMapping != DataTypeEnumLookup.end() ? itrDataTypeMapping->second : DATA_TYPE::UNKNOWN;
    f_.length = j_.at("length");
    f_.description = j_.at("description").is_null() ? "" : j_.at("description");
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
    fd_.arrayLengthFieldSize = j_.value("arrayLengthFieldSize", 4);
    fd_.dataType = j_.at("dataType");
    if (j_.find("arrayLengthRef") != j_.end()) { fd_.arrayLengthRef = j_.at("arrayLengthRef").is_null() ? "" : j_.at("arrayLengthRef"); }
}

//-----------------------------------------------------------------------
void from_json(const json& j_, FieldArrayField& fd_)
{
    from_json(j_, static_cast<BaseField&>(fd_));

    fd_.arrayLength = j_.at("arrayLength").is_null() ? 0 : static_cast<uint32_t>(j_.at("arrayLength"));
    fd_.arrayLengthFieldSize = j_.value("arrayLengthFieldSize", 4);
    fd_.fieldSize = fd_.arrayLength * ParseFields(j_.at("fields"), fd_.fieldInfo);
    if (j_.find("arrayLengthRef") != j_.end()) { fd_.arrayLengthRef = j_.at("arrayLengthRef").is_null() ? "" : j_.at("arrayLengthRef"); }
}

void from_json(const json& j_, EnumDefinition& ed_)
{
    std::vector<EnumDataType> enumerators;
    ParseEnumerators(j_.at("enumerators"), enumerators);
    ed_ = EnumDefinition(j_.at("_id"), j_.at("name"), std::move(enumerators));
}

//-----------------------------------------------------------------------
void from_json(const json& j_, DbMetadata& dbm_)
{
    dbm_.subset = j_.value("subset", "");
    dbm_.version = j_.value("version", "0.0.0");
    dbm_.messageFamily = j_.value("messageFamily", "");
}

//-----------------------------------------------------------------------
uint32_t ParseFields(const json& j_, FieldInfo& vFields_, const AlignFunction& alignFn_)
{
    uint32_t uiFieldSize = 0;
    vFields_.messageOrderedFields = std::vector<BaseField::ConstPtr>();
    vFields_.messageOrderedFields.reserve(j_.size());

    auto alignFixed = [&](size_t typeLength) {
        const auto ptr = static_cast<uintptr_t>(vFields_.fixedFieldBytes);
        vFields_.fixedFieldBytes += alignFn_(typeLength, uintptr_t{0}, ptr);
    };

    for (const auto& field : j_)
    {
        const auto sFieldType = field.at("type").get<std::string_view>();
        const auto stDataType = field.at("dataType").get<SimpleDataType>();

        if (sFieldType == "SIMPLE")
        {
            auto pstField = std::make_shared<BaseField>(field);
            alignFixed(stDataType.length);
            pstField->index = vFields_.fixedFieldBytes;
            vFields_.messageOrderedFields.push_back(pstField);
            uiFieldSize += stDataType.length;
            vFields_.fixedFieldBytes += stDataType.length;
        }
        else if (sFieldType == "ENUM")
        {
            auto pstField = std::make_shared<EnumField>(field);
            alignFixed(stDataType.length);
            pstField->index = vFields_.fixedFieldBytes;
            pstField->length = stDataType.length;
            vFields_.messageOrderedFields.push_back(pstField);
            uiFieldSize += stDataType.length;
            vFields_.fixedFieldBytes += stDataType.length;
        }
        else if (sFieldType == "FIXED_LENGTH_ARRAY" || sFieldType == "VARIABLE_LENGTH_ARRAY" || sFieldType == "STRING")
        {
            auto pstField = std::make_shared<ArrayField>(field);
            uint32_t uiArrayLength = field.at("arrayLength").get<uint32_t>();
            uiFieldSize += stDataType.length * uiArrayLength;
            if (sFieldType == "FIXED_LENGTH_ARRAY")
            {
                alignFixed(stDataType.length);
                pstField->index = vFields_.fixedFieldBytes;
                vFields_.fixedFieldBytes += stDataType.length * uiArrayLength;
            }
            else { pstField->index = vFields_.varFieldCount; }

            vFields_.messageOrderedFields.push_back(pstField);
            if (sFieldType != "FIXED_LENGTH_ARRAY") { vFields_.varFieldCount++; }
        }
        else if (sFieldType == "FIELD_ARRAY")
        {
            auto pstField = std::make_shared<FieldArrayField>(field);
            pstField->fieldInfo = {};
            pstField->fieldSize = pstField->arrayLength * ParseFields(field.at("fields"), pstField->fieldInfo, alignFn_);
            vFields_.messageOrderedFields.push_back(pstField);
            pstField->index = vFields_.varFieldCount;
            vFields_.varFieldCount++;
        }
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
std::vector<MessageDefinition::ConstPtr> ProcessMessageDefinitions(const json& jArray, const AlignFunction& alignFn_)
{
    const auto& data = jArray["messages"];
    std::vector<MessageDefinition::ConstPtr> res;
    res.reserve(data.size());

    for (const auto& it : data)
    {
        auto md = std::make_shared<MessageDefinition>();
        md->_id = it.at("_id");
        md->logID = it.at("messageID");
        md->name = it.at("name");
        md->description = it.at("description").is_null() ? "" : it.at("description");
        md->latestMessageCrc = std::stoul(it.at("latestMsgDefCrc").get<std::string>());
        md->messageStyle = it.contains("messageStyle") && !it.at("messageStyle").is_null() ? it.at("messageStyle") : "OEM4_MESSAGE_STYLE";

        for (const auto& fields : it.at("fields").items())
        {
            uint32_t defCrc = std::stoul(fields.key());
            ParseFields(fields.value(), md->fieldInfo[defCrc], alignFn_);
        }
        res.emplace_back(std::move(md));
    }

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
namespace {
template <typename T> MessageDatabase::Ptr ParseJsonDbImpl(T&& source, std::string_view errorContext)
{
    try
    {
        auto json = json::parse(std::forward<T>(source));

        DbMetadata::Ptr dbMeta;
        if (json.contains("meta")) { dbMeta = std::make_shared<DbMetadata>(json.at("meta").template get<DbMetadata>()); }

        AlignFunction alignFn = MessageDatabase::NoAlign;
        if (dbMeta && !dbMeta->messageFamily.empty())
        {
            const auto it = MessageDatabase::GetAlignmentFunctions().find(dbMeta->messageFamily);
            if (it != MessageDatabase::GetAlignmentFunctions().end()) { alignFn = it->second; }
        }

        auto messageFuture = std::async(std::launch::async, ProcessMessageDefinitions, std::cref(json), std::cref(alignFn));
        auto enumFuture = std::async(std::launch::async, ProcessEnumDefinitions, std::cref(json));

        return std::make_shared<MessageDatabase>(messageFuture.get(), enumFuture.get(), dbMeta);
    }
    catch (const std::exception& e)
    {
        throw JsonDbReaderFailure(__func__, __FILE__, __LINE__, errorContext, e.what());
    }
}
} // namespace

//-----------------------------------------------------------------------
MessageDatabase::Ptr LoadJsonDbFile(const std::filesystem::path& filePath_) { return ParseJsonDbImpl(std::ifstream(filePath_), filePath_.string()); }

//-----------------------------------------------------------------------
MessageDatabase::Ptr ParseJsonDb(std::string_view strJsonData_) { return ParseJsonDbImpl(strJsonData_, strJsonData_); }

} // namespace novatel::edie
