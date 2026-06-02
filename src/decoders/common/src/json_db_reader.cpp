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

#include <algorithm>
#include <cassert>
#include <future>
#include <stdexcept>
#include <string>

#include <simdjson.h>

#include "novatel_edie/decoders/common/common.hpp"

namespace novatel::edie {

namespace {

using simdjson::dom::array;
using simdjson::dom::element;
using simdjson::dom::object;

//-----------------------------------------------------------------------
// Small helpers providing required-member, optional-member and null-aware
// access semantics on top of the simdjson DOM API.
//-----------------------------------------------------------------------

//! Resolve a required object member. Throws if the key is absent.
element Member(element obj_, std::string_view key_)
{
    element out;
    if (obj_[key_].get(out) != simdjson::SUCCESS) { throw std::runtime_error("Missing required JSON field: " + std::string(key_)); }
    return out;
}

//! Read an element as a string view. Throws if it is not a string.
std::string_view AsStringView(element el_)
{
    std::string_view sv;
    if (el_.get(sv) != simdjson::SUCCESS) { throw std::runtime_error("Expected a JSON string value"); }
    return sv;
}

//! Read an element as an owned string. Throws if it is not a string.
std::string AsString(element el_) { return std::string(AsStringView(el_)); }

//! Read an element as a string, mapping a JSON null to an empty string.
std::string AsStringOrEmpty(element el_) { return el_.is_null() ? std::string() : AsString(el_); }

//! Read any JSON integer (tagged int64 or uint64) as int64. Throws otherwise.
int64_t AsInt(element el_)
{
    int64_t i;
    if (el_.get(i) == simdjson::SUCCESS) { return i; }
    uint64_t u;
    if (el_.get(u) == simdjson::SUCCESS) { return static_cast<int64_t>(u); }
    throw std::runtime_error("Expected a JSON integer value");
}

//! Read an optional string member, returning the default if the key is absent.
std::string StringOr(element obj_, std::string_view key_, std::string_view default_)
{
    element el;
    if (obj_[key_].get(el) != simdjson::SUCCESS) { return std::string(default_); }
    return AsString(el);
}

// Forward declarations (parse functions are mutually recursive via field arrays).
void ParseEnumDataType(element j_, EnumDataType& f_);
void ParseBaseDataType(element j_, BaseDataType& f_);
void ParseSimpleDataType(element j_, SimpleDataType& f_);
void ParseBaseField(element j_, BaseField& f_);
void ParseEnumField(element j_, EnumField& f_);
void ParseArrayField(element j_, ArrayField& fd_);
void ParseFieldArrayField(element j_, FieldArrayField& fd_);
void ParseMessageDefinition(element j_, MessageDefinition& md_);
void ParseEnumDefinition(element j_, EnumDefinition& ed_);
void ParseDbMetadata(element j_, DbMetadata& dbm_);
uint32_t ParseFields(element j_, std::vector<BaseField::Ptr>& vFields_);
void ParseEnumerators(element j_, std::vector<EnumDataType>& vEnumerators_);

//-----------------------------------------------------------------------
void ParseEnumDataType(element j_, EnumDataType& f_)
{
    f_.value = static_cast<uint32_t>(AsInt(Member(j_, "value")));
    f_.name = AsString(Member(j_, "name"));
    f_.description = AsStringOrEmpty(Member(j_, "description"));
}

//-----------------------------------------------------------------------
void ParseBaseDataType(element j_, BaseDataType& f_)
{
    const auto itrDataTypeMapping = DataTypeEnumLookup.find(std::string(AsStringView(Member(j_, "name"))));
    f_.name = itrDataTypeMapping != DataTypeEnumLookup.end() ? itrDataTypeMapping->second : DATA_TYPE::UNKNOWN;
    f_.length = static_cast<uint16_t>(AsInt(Member(j_, "length")));
    f_.description = AsStringOrEmpty(Member(j_, "description"));
}

//-----------------------------------------------------------------------
void ParseSimpleDataType(element j_, SimpleDataType& f_)
{
    ParseBaseDataType(j_, f_);

    element enumEl;
    if (j_["enum"].get(enumEl) == simdjson::SUCCESS)
    {
        array enumArr;
        if (enumEl.get(enumArr) == simdjson::SUCCESS)
        {
            for (element e : enumArr)
            {
                EnumDataType enumerator;
                ParseEnumDataType(e, enumerator);
                f_.enums[static_cast<int32_t>(enumerator.value)] = std::move(enumerator);
            }
        }
    }
}

//-----------------------------------------------------------------------
void ParseBaseField(element j_, BaseField& f_)
{
    f_.name = AsString(Member(j_, "name"));
    f_.description = AsStringOrEmpty(Member(j_, "description"));

    const auto itrFieldTypeMapping = FieldTypeEnumLookup.find(std::string(AsStringView(Member(j_, "type"))));
    f_.type = itrFieldTypeMapping != FieldTypeEnumLookup.end() ? itrFieldTypeMapping->second : FIELD_TYPE::UNKNOWN;

    element conversionString;
    if (j_["conversionString"].get(conversionString) == simdjson::SUCCESS)
    {
        if (conversionString.is_null()) { f_.conversion = ""; }
        else { f_.SetConversion(std::string(AsStringView(conversionString))); }
    }

    ParseSimpleDataType(Member(j_, "dataType"), f_.dataType);
}

//-----------------------------------------------------------------------
void ParseEnumField(element j_, EnumField& f_)
{
    ParseBaseField(j_, f_);

    element enumId = Member(j_, "enumID");
    if (enumId.is_null()) { throw std::runtime_error("Invalid enum ID - cannot be NULL. JsonDB file is likely corrupted."); }

    f_.enumId = AsString(enumId);
}

//-----------------------------------------------------------------------
void ParseArrayField(element j_, ArrayField& fd_)
{
    ParseBaseField(j_, fd_);

    fd_.arrayLength = static_cast<uint32_t>(AsInt(Member(j_, "arrayLength")));

    element arrayLengthFieldSize;
    fd_.arrayLengthFieldSize =
        j_["arrayLengthFieldSize"].get(arrayLengthFieldSize) == simdjson::SUCCESS ? static_cast<uint8_t>(AsInt(arrayLengthFieldSize)) : 4;

    element arrayLengthRef;
    if (j_["arrayLengthRef"].get(arrayLengthRef) == simdjson::SUCCESS) { fd_.arrayLengthRef = AsStringOrEmpty(arrayLengthRef); }
}

//-----------------------------------------------------------------------
void ParseFieldArrayField(element j_, FieldArrayField& fd_)
{
    ParseBaseField(j_, fd_);

    element arrayLength = Member(j_, "arrayLength");
    fd_.arrayLength = arrayLength.is_null() ? 0 : static_cast<uint32_t>(AsInt(arrayLength));

    element arrayLengthFieldSize;
    fd_.arrayLengthFieldSize =
        j_["arrayLengthFieldSize"].get(arrayLengthFieldSize) == simdjson::SUCCESS ? static_cast<uint8_t>(AsInt(arrayLengthFieldSize)) : 4;

    fd_.fieldSize = fd_.arrayLength * ParseFields(Member(j_, "fields"), fd_.fields);

    element arrayLengthRef;
    if (j_["arrayLengthRef"].get(arrayLengthRef) == simdjson::SUCCESS) { fd_.arrayLengthRef = AsStringOrEmpty(arrayLengthRef); }
}

//-----------------------------------------------------------------------
void ParseMessageDefinition(element j_, MessageDefinition& md_)
{
    md_._id = AsString(Member(j_, "_id"));
    md_.logID = static_cast<uint32_t>(AsInt(Member(j_, "messageID"))); // this was "logID"
    md_.name = AsString(Member(j_, "name"));
    md_.description = AsStringOrEmpty(Member(j_, "description"));
    md_.latestMessageCrc = std::stoul(AsString(Member(j_, "latestMsgDefCrc")));

    element messageStyle;
    md_.messageStyle =
        j_["messageStyle"].get(messageStyle) == simdjson::SUCCESS && !messageStyle.is_null() ? AsString(messageStyle) : "OEM4_MESSAGE_STYLE";

    object fields;
    if (Member(j_, "fields").get(fields) != simdjson::SUCCESS) { throw std::runtime_error("Expected 'fields' to be a JSON object"); }
    for (auto field : fields)
    {
        uint32_t defCrc = std::stoul(std::string(field.key));
        ParseFields(field.value, md_.fields[defCrc]);
    }
}

//-----------------------------------------------------------------------
void ParseEnumDefinition(element j_, EnumDefinition& ed_)
{
    ed_._id = AsString(Member(j_, "_id"));
    ed_.name = AsString(Member(j_, "name"));

    // Parse enumerators into the vector
    ParseEnumerators(Member(j_, "enumerators"), ed_.enumerators);

    // Populate the lookup maps
    uint32_t maxVal = 0;
    for (const auto& enumerator : ed_.enumerators)
    {
        maxVal = std::max(maxVal, enumerator.value);
        ed_.nameValue[enumerator.name] = enumerator.value;
        ed_.valueName[enumerator.value] = enumerator.name;
        ed_.descriptionValue[enumerator.description] = enumerator.value;
    }
    ed_.unknownValue = maxVal + 1;
    assert(ed_.unknownValue > maxVal &&
           "Overflow encountered when determining placeholder value. Enumerator values are expected to  be within [0, 2^31).");
}

//-----------------------------------------------------------------------
void ParseDbMetadata(element j_, DbMetadata& dbm_)
{
    dbm_.subset = StringOr(j_, "subset", "");
    dbm_.version = StringOr(j_, "version", "0.0.0");
    dbm_.messageFamily = StringOr(j_, "messageFamily", "");
}

//-----------------------------------------------------------------------
uint32_t ParseFields(element j_, std::vector<BaseField::Ptr>& vFields_)
{
    uint32_t uiFieldSize = 0;

    array fields;
    if (j_.get(fields) != simdjson::SUCCESS) { throw std::runtime_error("Expected 'fields' to be a JSON array"); }
    vFields_.reserve(fields.size());

    for (element field : fields)
    {
        const auto sFieldType = AsStringView(Member(field, "type"));

        BaseDataType stDataType;
        ParseBaseDataType(Member(field, "dataType"), stDataType);

        if (sFieldType == "SIMPLE")
        {
            auto pstField = std::make_shared<BaseField>();
            ParseBaseField(field, *pstField);
            vFields_.emplace_back(std::move(pstField));
            uiFieldSize += stDataType.length;
        }
        else if (sFieldType == "ENUM")
        {
            auto pstField = std::make_shared<EnumField>();
            ParseEnumField(field, *pstField);
            pstField->length = stDataType.length;
            vFields_.emplace_back(std::move(pstField));
            uiFieldSize += stDataType.length;
        }
        else if (sFieldType == "FIXED_LENGTH_ARRAY" || sFieldType == "VARIABLE_LENGTH_ARRAY" || sFieldType == "STRING")
        {
            auto pstField = std::make_shared<ArrayField>();
            ParseArrayField(field, *pstField);
            uint32_t uiArrayLength = static_cast<uint32_t>(AsInt(Member(field, "arrayLength")));
            vFields_.emplace_back(std::move(pstField));
            uiFieldSize += stDataType.length * uiArrayLength;
        }
        else if (sFieldType == "FIELD_ARRAY")
        {
            auto pstField = std::make_shared<FieldArrayField>();
            ParseFieldArrayField(field, *pstField);
            vFields_.emplace_back(std::move(pstField));
        }
        else { throw std::runtime_error("Could not find field type"); }
    }
    return uiFieldSize;
}

//-----------------------------------------------------------------------
void ParseEnumerators(element j_, std::vector<EnumDataType>& vEnumerators_)
{
    array enumerators;
    if (j_.get(enumerators) != simdjson::SUCCESS) { throw std::runtime_error("Expected 'enumerators' to be a JSON array"); }
    vEnumerators_.reserve(enumerators.size());
    for (element enumerator : enumerators)
    {
        EnumDataType enumDataType;
        ParseEnumDataType(enumerator, enumDataType);
        vEnumerators_.emplace_back(std::move(enumDataType));
    }
}

//-----------------------------------------------------------------------
std::vector<MessageDefinition::ConstPtr> ProcessMessageDefinitions(element jRoot_)
{
    array data;
    if (Member(jRoot_, "messages").get(data) != simdjson::SUCCESS) { throw std::runtime_error("Expected 'messages' to be a JSON array"); }

    std::vector<MessageDefinition::ConstPtr> res;
    res.reserve(data.size());

    for (element it : data)
    {
        auto md = std::make_shared<MessageDefinition>();
        ParseMessageDefinition(it, *md);
        res.emplace_back(std::move(md));
    }

    return res;
}

//-----------------------------------------------------------------------
std::vector<EnumDefinition::ConstPtr> ProcessEnumDefinitions(element jRoot_)
{
    array data;
    if (Member(jRoot_, "enums").get(data) != simdjson::SUCCESS) { throw std::runtime_error("Expected 'enums' to be a JSON array"); }

    std::vector<EnumDefinition::ConstPtr> res;
    res.reserve(data.size());

    for (element it : data)
    {
        auto ed = std::make_shared<EnumDefinition>();
        ParseEnumDefinition(it, *ed);
        res.emplace_back(std::move(ed));
    }

    return res;
}

//-----------------------------------------------------------------------
MessageDatabase::Ptr ParseJsonDbImpl(simdjson::padded_string source, std::string_view errorContext)
{
    try
    {
        simdjson::dom::parser parser;
        element root;
        if (parser.parse(source).get(root) != simdjson::SUCCESS) { throw std::runtime_error("Failed to parse JSON database"); }

        // The parsed DOM is immutable; the message and enum subtrees can be read concurrently.
        auto messageFuture = std::async(std::launch::async, ProcessMessageDefinitions, root);
        auto enumFuture = std::async(std::launch::async, ProcessEnumDefinitions, root);

        DbMetadata::Ptr dbMeta;
        element meta;
        if (root["meta"].get(meta) == simdjson::SUCCESS)
        {
            dbMeta = std::make_shared<DbMetadata>();
            ParseDbMetadata(meta, *dbMeta);
        }

        return std::make_shared<MessageDatabase>(messageFuture.get(), enumFuture.get(), dbMeta);
    }
    catch (const std::exception& e)
    {
        throw JsonDbReaderFailure(__func__, __FILE__, __LINE__, errorContext, e.what());
    }
}
} // namespace

//-----------------------------------------------------------------------
MessageDatabase::Ptr LoadJsonDbFile(const std::filesystem::path& filePath_)
{
    simdjson::padded_string source;
    const auto error = simdjson::padded_string::load(filePath_.string()).get(source);
    if (error) { throw JsonDbReaderFailure(__func__, __FILE__, __LINE__, filePath_, simdjson::error_message(error)); }
    return ParseJsonDbImpl(std::move(source), filePath_.string());
}

//-----------------------------------------------------------------------
MessageDatabase::Ptr ParseJsonDb(std::string_view strJsonData_)
{
    return ParseJsonDbImpl(simdjson::padded_string(strJsonData_.data(), strJsonData_.size()), strJsonData_);
}

} // namespace novatel::edie
