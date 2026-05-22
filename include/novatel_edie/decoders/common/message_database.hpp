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
// ! \file message_database.hpp
// ===============================================================================

#ifndef MESSAGE_DATABASE_HPP
#define MESSAGE_DATABASE_HPP

#include <algorithm>
#include <cassert>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "novatel_edie/common/crc.hpp"
#include "novatel_edie/common/logger.hpp"

namespace novatel::edie {

//-----------------------------------------------------------------------
//! \enum DATA_TYPE
//! \brief Data type name string represented as an enum.
//-----------------------------------------------------------------------
enum class DATA_TYPE
{
    BOOL,
    CHAR,
    UCHAR,
    SHORT,
    USHORT,
    INT,
    UINT,
    LONG,
    ULONG,
    LONGLONG,
    ULONGLONG,
    FLOAT,
    DOUBLE,
    HEXBYTE,
    SATELLITEID,
    UNKNOWN
};

//!< returns the size associated with a datatype
constexpr size_t DataTypeSize(const DATA_TYPE eType_)
{
    switch (eType_)
    {
    case DATA_TYPE::BOOL: return sizeof(int32_t);
    case DATA_TYPE::HEXBYTE: return sizeof(uint8_t);
    case DATA_TYPE::CHAR: return sizeof(int8_t);
    case DATA_TYPE::UCHAR: return sizeof(uint8_t);
    case DATA_TYPE::SHORT: return sizeof(int16_t);
    case DATA_TYPE::USHORT: return sizeof(uint16_t);
    case DATA_TYPE::INT: return sizeof(int32_t);
    case DATA_TYPE::UINT: return sizeof(uint32_t);
    case DATA_TYPE::LONG: return sizeof(int32_t);
    case DATA_TYPE::ULONG: return sizeof(uint32_t);
    case DATA_TYPE::LONGLONG: return sizeof(int64_t);
    case DATA_TYPE::ULONGLONG: return sizeof(uint64_t);
    case DATA_TYPE::FLOAT: return sizeof(float);
    case DATA_TYPE::DOUBLE: return sizeof(double);
    case DATA_TYPE::SATELLITEID: return sizeof(uint32_t);
    default: return 0;
    }
}

constexpr std::string_view DataTypeToString(const DATA_TYPE eDataType_)
{
    switch (eDataType_)
    {
    case DATA_TYPE::BOOL: return "BOOL";
    case DATA_TYPE::CHAR: return "CHAR";
    case DATA_TYPE::UCHAR: return "UCHAR";
    case DATA_TYPE::SHORT: return "SHORT";
    case DATA_TYPE::USHORT: return "USHORT";
    case DATA_TYPE::INT: return "INT";
    case DATA_TYPE::UINT: return "UINT";
    case DATA_TYPE::LONG: return "LONG";
    case DATA_TYPE::ULONG: return "ULONG";
    case DATA_TYPE::LONGLONG: return "LONGLONG";
    case DATA_TYPE::ULONGLONG: return "ULONGLONG";
    case DATA_TYPE::FLOAT: return "FLOAT";
    case DATA_TYPE::DOUBLE: return "DOUBLE";
    case DATA_TYPE::HEXBYTE: return "HEXBYTE";
    case DATA_TYPE::SATELLITEID: return "SATELLITEID";
    case DATA_TYPE::UNKNOWN: return "UNKNOWN";
    default: return "UNKNOWN";
    }
}

// TODO: this table is misleading, as one DATA_TYPE may correspond to many different conversion strings
//!< returns conversion string associated with a datatype
inline std::string DataTypeConversion(const DATA_TYPE eType_)
{
    switch (eType_)
    {
    case DATA_TYPE::BOOL: return "%d";
    case DATA_TYPE::CHAR: return "%c";
    case DATA_TYPE::UCHAR: return "%uc";
    case DATA_TYPE::SHORT: return "%hd";
    case DATA_TYPE::USHORT: return "%hu";
    case DATA_TYPE::INT: return "%d";
    case DATA_TYPE::UINT: return "%u";
    case DATA_TYPE::LONG: return "%ld";
    case DATA_TYPE::ULONG: return "%lu";
    case DATA_TYPE::LONGLONG: return "%lld";
    case DATA_TYPE::ULONGLONG: return "%llu";
    case DATA_TYPE::FLOAT: return "%f";
    case DATA_TYPE::DOUBLE: return "%lf";
    case DATA_TYPE::HEXBYTE: return "%Z"; // these are not valid default conversion strings
    case DATA_TYPE::SATELLITEID: return "%id";
    default: return "%";
    }
}

//!< Mapping from String to data type enums.
static const std::unordered_map<std::string, DATA_TYPE> DataTypeEnumLookup = {
    {"BOOL", DATA_TYPE::BOOL},      {"HEXBYTE", DATA_TYPE::HEXBYTE},   {"CHAR", DATA_TYPE::CHAR},
    {"UCHAR", DATA_TYPE::UCHAR},    {"SHORT", DATA_TYPE::SHORT},       {"USHORT", DATA_TYPE::USHORT},
    {"INT", DATA_TYPE::INT},        {"UINT", DATA_TYPE::UINT},         {"LONG", DATA_TYPE::LONG},
    {"ULONG", DATA_TYPE::ULONG},    {"LONGLONG", DATA_TYPE::LONGLONG}, {"ULONGLONG", DATA_TYPE::ULONGLONG},
    {"FLOAT", DATA_TYPE::FLOAT},    {"DOUBLE", DATA_TYPE::DOUBLE},     {"SATELLITEID", DATA_TYPE::SATELLITEID},
    {"UNKNOWN", DATA_TYPE::UNKNOWN}};

//-----------------------------------------------------------------------
//! \enum FIELD_TYPE
//! \brief Field type string represented as an enum.
//-----------------------------------------------------------------------
enum class FIELD_TYPE
{
    SIMPLE,                //!< Simple type.
    ENUM,                  //!< Enum type.
    BITFIELD,              //!< BitField type.
    FIXED_LENGTH_ARRAY,    //!< Fixed-sized array.
    VARIABLE_LENGTH_ARRAY, //!< Variable-length array.
    STRING,                //!< String type.
    FIELD_ARRAY,           //!< Array of other fields.
    RESPONSE_ID,           //!< Fabricated response ID field.
    RESPONSE_STR,          //!< Fabricated response string field.
    RXCONFIG_HEADER,       //!< Fabricated RXCONFIG header field.
    RXCONFIG_BODY,         //!< Fabricated RXCONFIG body field.
    UNKNOWN                //!< Unknown.
};

//!< Mapping from String to field type enums.
static const std::unordered_map<std::string, FIELD_TYPE> FieldTypeEnumLookup = {{"SIMPLE", FIELD_TYPE::SIMPLE},
                                                                                {"ENUM", FIELD_TYPE::ENUM},
                                                                                {"BITFIELD", FIELD_TYPE::BITFIELD},
                                                                                {"FIXED_LENGTH_ARRAY", FIELD_TYPE::FIXED_LENGTH_ARRAY},
                                                                                {"VARIABLE_LENGTH_ARRAY", FIELD_TYPE::VARIABLE_LENGTH_ARRAY},
                                                                                {"STRING", FIELD_TYPE::STRING},
                                                                                {"FIELD_ARRAY", FIELD_TYPE::FIELD_ARRAY},
                                                                                {"UNKNOWN", FIELD_TYPE::UNKNOWN}};

constexpr std::string_view FieldTypeToString(const FIELD_TYPE eFieldType_)
{
    switch (eFieldType_)
    {
    case FIELD_TYPE::SIMPLE: return "SIMPLE";
    case FIELD_TYPE::ENUM: return "ENUM";
    case FIELD_TYPE::BITFIELD: return "BITFIELD";
    case FIELD_TYPE::FIXED_LENGTH_ARRAY: return "FIXED_LENGTH_ARRAY";
    case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: return "VARIABLE_LENGTH_ARRAY";
    case FIELD_TYPE::STRING: return "STRING";
    case FIELD_TYPE::FIELD_ARRAY: return "FIELD_ARRAY";
    case FIELD_TYPE::RESPONSE_ID: return "RESPONSE_ID";
    case FIELD_TYPE::RESPONSE_STR: return "RESPONSE_STR";
    case FIELD_TYPE::RXCONFIG_HEADER: return "RXCONFIG_HEADER";
    case FIELD_TYPE::RXCONFIG_BODY: return "RXCONFIG_BODY";
    case FIELD_TYPE::UNKNOWN: return "UNKNOWN";
    default: return "UNKNOWN";
    }
}

//-----------------------------------------------------------------------
//! \struct EnumDataType
//! \brief Enum Data Type representing contents of UI DB.
//-----------------------------------------------------------------------
struct EnumDataType
{
    uint32_t value{0};
    std::string name;
    std::string description;

    [[nodiscard]] bool operator==(const EnumDataType& other) const
    {
        return value == other.value && name == other.name && description == other.description;
    }
    [[nodiscard]] bool operator!=(const EnumDataType& other) const { return !(*this == other); }
};

//-----------------------------------------------------------------------
//! \struct EnumDefinition
//! \brief Enum Definition representing contents of UI DB.
//-----------------------------------------------------------------------
struct EnumDefinition
{
    std::string _id;
    std::string name;
    std::vector<EnumDataType> enumerators;
    std::unordered_map<std::string_view, uint32_t> nameValue;        // cached
    std::unordered_map<std::string_view, uint32_t> descriptionValue; // cached
    std::unordered_map<uint32_t, std::string_view> valueName;        // cached
    uint32_t unknownValue{0};                                        // cached; one greater than the largest enumerator value

    using Ptr = std::shared_ptr<EnumDefinition>;
    using ConstPtr = std::shared_ptr<const EnumDefinition>;

    EnumDefinition() = default;
    ~EnumDefinition() = default;

    EnumDefinition(std::string id_, std::string name_, std::vector<EnumDataType> enumerators_)
        : _id(std::move(id_)), name(std::move(name_)), enumerators(std::move(enumerators_))
    {
        RebuildCaches();
    }

    //----------------------------------------------------------------------------
    //! \brief Deep copy. The lookup maps are rebuilt against the new
    //! `enumerators` vector — a default copy would alias the source's
    //! enumerator strings via the `string_view` keys, dangling once the source
    //! is destroyed.
    //----------------------------------------------------------------------------
    EnumDefinition(const EnumDefinition& other) : _id(other._id), name(other.name), enumerators(other.enumerators) { RebuildCaches(); }

    EnumDefinition& operator=(const EnumDefinition& other)
    {
        if (this == &other) { return *this; }
        _id = other._id;
        name = other.name;
        enumerators = other.enumerators;
        RebuildCaches();
        return *this;
    }

    EnumDefinition(EnumDefinition&&) = default;
    EnumDefinition& operator=(EnumDefinition&&) = default;

    void RebuildCaches()
    {
        nameValue.clear();
        valueName.clear();
        descriptionValue.clear();
        uint32_t maxVal = 0;
        for (const auto& enumerator : enumerators)
        {
            maxVal = std::max(maxVal, enumerator.value);
            nameValue[enumerator.name] = enumerator.value;
            valueName[enumerator.value] = enumerator.name;
            descriptionValue[enumerator.description] = enumerator.value;
        }
        unknownValue = maxVal + 1;
        assert(unknownValue > maxVal &&
               "Overflow encountered when determining placeholder value. Enumerator values are expected to be within [0, 2^31).");
    }
};

//-----------------------------------------------------------------------
//! \struct SimpleDataType
//! \brief Struct containing elements of simple data type fields in the
//! UI DB.
//-----------------------------------------------------------------------
struct SimpleDataType
{
    DATA_TYPE name{DATA_TYPE::UNKNOWN};
    uint16_t length{0};
    std::string description;

    [[nodiscard]] bool operator==(const SimpleDataType& other) const
    {
        return name == other.name && length == other.length && description == other.description;
    }
    [[nodiscard]] bool operator!=(const SimpleDataType& other) const { return !(*this == other); }
};

//-----------------------------------------------------------------------
//! \struct BaseField
//! \brief Struct containing elements of basic fields in the UI DB.
//-----------------------------------------------------------------------
struct BaseField
{
    std::string name;
    FIELD_TYPE type{FIELD_TYPE::UNKNOWN};
    std::string description;
    std::string conversion;
    uint32_t conversionHash{0ULL};    // cached
    std::optional<int32_t> width;     // cached
    std::optional<int32_t> precision; // cached
    bool isString{false};             // cached; has a FIELD_TYPE of STRING or conversion string of either %s or %S
    bool isCsv{false};                // cached; Ascii encoding of this field is comma-separated,
                                      // true for arrays which are not strings and do not use the %Z or %P conversion strings
    SimpleDataType dataType;
    size_t index{0}; // if fixed, this is the byte offset of the field in MessageBody.fixedFields
                     // if variable, this is the index of the field in MessageBody.variableFields

    BaseField() = default;

    BaseField(std::string name_, FIELD_TYPE type_, std::string conversion_, DATA_TYPE eDataTypeName_) : name(std::move(name_)), type(type_)
    {
        dataType.name = eDataTypeName_;
        dataType.length = static_cast<uint16_t>(DataTypeSize(eDataTypeName_));
        if (!conversion_.empty()) { SetConversion(std::move(conversion_)); }
    }

    virtual ~BaseField() = default;

    //----------------------------------------------------------------------------
    //! \brief Compares the non-cached fields. Returns true when `other` has the
    //! same runtime type and every non-cached field compares equal.
    //
    //! Cached fields are intentionally excluded from the comparison. For
    //! example, `EnumField::enumDef` is a cache derived from `enumId`, and we
    //! want two `EnumField` instances that name the same enum to compare equal
    //! regardless of which database happens to have resolved their cache
    //! pointer.
    //----------------------------------------------------------------------------
    [[nodiscard]] bool operator==(const BaseField& other) const { return typeid(*this) == typeid(other) && equalsImpl(other); }

    [[nodiscard]] bool operator!=(const BaseField& other) const { return !(*this == other); }

    //----------------------------------------------------------------------------
    //! \brief Produce a deep copy of this field definition.
    //
    //! Subclasses must override to preserve their runtime type. The base
    //! implementation copy-constructs a plain `BaseField` and is correct only
    //! for `FIELD_TYPE`s that don't have a subclass (`SIMPLE`, `STRING`,
    //! `RESPONSE_ID`, `RESPONSE_STR`, `BITFIELD`, `RXCONFIG_*`, `UNKNOWN`).
    //
    //! `FieldArrayField::clone()` recursively deep-copies its nested `fields`
    //! vector so the resulting tree shares no `shared_ptr<BaseField>` storage
    //! with the source.
    //
    //! `EnumField::enumDef` and other `*Definition::ConstPtr` members are
    //! shallow-copied: those entities are treated as immutable values, so
    //! sharing the pointer is safe.
    //----------------------------------------------------------------------------
    [[nodiscard]] virtual std::shared_ptr<BaseField> clone() const { return std::make_shared<BaseField>(*this); }

    void SetConversion(std::string&& sConversion_)
    {
        const char* sConvertString = sConversion_.c_str();

        if (*sConvertString != '%') { throw std::runtime_error("Encountered an unexpected character in conversion string"); }
        ++sConvertString;

        std::optional<int32_t> newWidth;
        if (std::isdigit(*sConvertString))
        {
            newWidth = std::stoi(sConvertString);
            sConvertString += std::to_string(*newWidth).length();
        }

        if (*sConvertString == '.') { ++sConvertString; }

        std::optional<int32_t> newPrecision;
        if (std::isdigit(*sConvertString))
        {
            newPrecision = std::stoi(sConvertString);
            sConvertString += std::to_string(*newPrecision).length();
        }

        if (!std::isalpha(*sConvertString)) { throw std::runtime_error("Conversion string must contain a character"); }

        uint32_t newHash = 0;
        while (std::isalpha(*sConvertString)) { CalculateCharacterCrc32(newHash, *sConvertString++); }

        if (*sConvertString != '\0') { throw std::runtime_error("Encountered an unexpected character in conversion string"); }

        conversion = std::move(sConversion_);
        width = newWidth;
        precision = newPrecision;
        conversionHash = newHash;
        ComputeStringFlags();
    }

    void ComputeStringFlags()
    {
        isString = type == FIELD_TYPE::STRING || conversionHash == CalculateBlockCrc32("s") || conversionHash == CalculateBlockCrc32("S");
        isCsv = !isString && conversionHash != CalculateBlockCrc32("Z") && conversionHash != CalculateBlockCrc32("P");
    }

    using Ptr = std::shared_ptr<BaseField>;
    using ConstPtr = std::shared_ptr<const BaseField>;

  protected:
    //----------------------------------------------------------------------------
    //! \brief Virtual hook used by `operator==` after a runtime-type match.
    //! Subclasses override to compare their own members in addition to the
    //! base class fields.
    //----------------------------------------------------------------------------
    [[nodiscard]] virtual bool equalsImpl(const BaseField& other) const
    {
        return name == other.name && type == other.type && description == other.description && conversion == other.conversion &&
               dataType == other.dataType;
    }
};

//-----------------------------------------------------------------------
//! \struct EnumField
//! \brief Struct containing elements of enum fields in the UI DB.
//-----------------------------------------------------------------------
struct EnumField : BaseField
{
    std::string enumId;
    EnumDefinition::ConstPtr enumDef{nullptr}; // cached

    EnumField() = default;

    EnumField(std::string name_, FIELD_TYPE type_, std::string conversion_, DATA_TYPE eDataTypeName_, std::string enumId_)
        : BaseField(std::move(name_), type_, std::move(conversion_), eDataTypeName_), enumId(std::move(enumId_))
    {
    }

    [[nodiscard]] std::shared_ptr<BaseField> clone() const override { return std::make_shared<EnumField>(*this); }

    using Ptr = std::shared_ptr<EnumField>;
    using ConstPtr = std::shared_ptr<const EnumField>;

  protected:
    [[nodiscard]] bool equalsImpl(const BaseField& other) const override
    {
        if (!BaseField::equalsImpl(other)) { return false; }
        const auto& o = static_cast<const EnumField&>(other);
        return enumId == o.enumId;
    }
};

//-----------------------------------------------------------------------
//! \struct ArrayField
//! \brief Struct containing elements of array fields in the UI DB.
//-----------------------------------------------------------------------
struct ArrayField : BaseField
{
    uint32_t arrayLength{0};
    std::string arrayLengthRef;
    uint8_t arrayLengthFieldSize{0}; // in bytes, only for variable-length and field arrays

    ArrayField() = default;

    ArrayField(std::string name_, FIELD_TYPE type_, std::string conversion_, DATA_TYPE eDataTypeName_, uint32_t arrayLength_)
        : BaseField(std::move(name_), type_, std::move(conversion_), eDataTypeName_), arrayLength(arrayLength_)
    {
    }

    [[nodiscard]] std::shared_ptr<BaseField> clone() const override { return std::make_shared<ArrayField>(*this); }

    using Ptr = std::shared_ptr<ArrayField>;
    using ConstPtr = std::shared_ptr<const ArrayField>;
    
  protected:
    [[nodiscard]] bool equalsImpl(const BaseField& other) const override
    {
        if (!BaseField::equalsImpl(other)) { return false; }
        const auto& o = static_cast<const ArrayField&>(other);
        return arrayLength == o.arrayLength && arrayLengthRef == o.arrayLengthRef && arrayLengthFieldSize == o.arrayLengthFieldSize;
    }
};

struct FieldInfo
{
    size_t fixedFieldBytes{0};
    size_t varFieldCount{0};
    std::vector<BaseField::ConstPtr> messageOrderedFields;                 // vector of field definitions in the order they are encoded in the message
    std::unordered_map<std::string, BaseField::ConstPtr> fieldNameToDef{}; // map of field name to field definition
};

//-----------------------------------------------------------------------
//! \struct FieldArrayField
//! \brief Struct containing elements of field array fields in the UI DB.
//-----------------------------------------------------------------------
struct FieldArrayField : ArrayField
{
    uint32_t fieldSize{0}; // cached
    FieldInfo fieldInfo{};

    FieldArrayField() = default;

    FieldArrayField(std::string name_, FIELD_TYPE type_, std::string conversion_, DATA_TYPE eDataTypeName_, uint32_t arrayLength_,
                    std::vector<std::shared_ptr<BaseField>> fields_)
        : ArrayField(std::move(name_), type_, std::move(conversion_), eDataTypeName_, arrayLength_), fields(std::move(fields_))
    {
        uint32_t childTotal = 0;
        for (const auto& f : *fieldInfo.messageOrderedFields)
        {
            if (!f) { continue; }
            switch (f->type)
            {
            case FIELD_TYPE::SIMPLE:
            case FIELD_TYPE::ENUM: childTotal += f->dataType.length; break;
            case FIELD_TYPE::FIXED_LENGTH_ARRAY:
            case FIELD_TYPE::VARIABLE_LENGTH_ARRAY:
            case FIELD_TYPE::STRING:
                if (const auto* a = dynamic_cast<const ArrayField*>(f.get())) { childTotal += f->dataType.length * a->arrayLength; }
                break;
            default: break;
            }
        }
        fieldSize = arrayLength * childTotal;
    }

    [[nodiscard]] std::shared_ptr<BaseField> clone() const override
    {
        auto copy = std::make_shared<FieldArrayField>(*this);
        for (auto& sub : *copy->fieldInfo.messageOrderedFields) { sub = sub->clone(); }
        return copy;
    }

    using Ptr = std::shared_ptr<FieldArrayField>;
    using ConstPtr = std::shared_ptr<const FieldArrayField>;

  protected:
    [[nodiscard]] bool equalsImpl(const BaseField& other) const override
    {
        if (!ArrayField::equalsImpl(other)) { return false; }
        const auto& o = static_cast<const FieldArrayField&>(other);
        if (fields.size() != o.fields.size()) { return false; }
        for (size_t i = 0; i < fields.size(); ++i)
        {
            const bool lhs_null = !fields[i];
            const bool rhs_null = !o.fields[i];
            if (lhs_null != rhs_null) { return false; }
            if (!lhs_null && *fields[i] != *o.fields[i]) { return false; }
        }
        return true;
    }
};

//-----------------------------------------------------------------------
//! \struct DbMetadata
//! \brief Struct containing metadata about the message database.
//-----------------------------------------------------------------------
struct DbMetadata
{
    std::string subset;
    std::string version;
    std::string messageFamily;

    using Ptr = std::shared_ptr<DbMetadata>;
    using ConstPtr = std::shared_ptr<const DbMetadata>;
};

//-----------------------------------------------------------------------
//! \struct MessageDefinition
//! \brief Struct containing elements of message definitions in the UI
//! DB.
//-----------------------------------------------------------------------
struct MessageDefinition
{
    std::string _id;
    uint32_t logID{0};
    std::string name;
    std::string description;
    std::string messageStyle;
    std::unordered_map<uint32_t, FieldInfo> fieldInfo; // map of crc keys to field info
    uint32_t latestMessageCrc{0};

    const FieldInfo& GetMsgDefFromCrc(uint32_t uiMsgDefCrc_) const;

    //----------------------------------------------------------------------------
    //! \brief Compares the non-cached fields. Two `MessageDefinition`s compare
    //! equal if every scalar field matches and every `BaseField::Ptr` in
    //! `fields` dereferences to an equal field (which likewise compares only
    //! non-cached fields). Pointer identity is not required, so a deep-copied
    //! definition compares equal to its source.
    //----------------------------------------------------------------------------
    [[nodiscard]] bool operator==(const MessageDefinition& other) const
    {
        if (_id != other._id || logID != other.logID || name != other.name || description != other.description ||
            latestMessageCrc != other.latestMessageCrc || fields.size() != other.fields.size())
        {
            return false;
        }
        for (const auto& [crc, fieldVec] : fields)
        {
            auto it = other.fields.find(crc);
            if (it == other.fields.end()) { return false; }
            const auto& otherVec = it->second;
            if (fieldVec.size() != otherVec.size()) { return false; }
            for (size_t i = 0; i < fieldVec.size(); ++i)
            {
                const bool lhs_null = !fieldVec[i];
                const bool rhs_null = !otherVec[i];
                if (lhs_null != rhs_null) { return false; }
                if (!lhs_null && *fieldVec[i] != *otherVec[i]) { return false; }
            }
        }
        return true;
    }

    [[nodiscard]] bool operator!=(const MessageDefinition& other) const { return !(*this == other); }

    MessageDefinition() = default;
    ~MessageDefinition() = default;

    MessageDefinition(std::string id_, uint32_t logID_, std::string name_, std::string description_, uint32_t latestMessageCrc_,
                      std::unordered_map<uint32_t, std::vector<BaseField::Ptr>> fields_)
        : _id(std::move(id_)), logID(logID_), name(std::move(name_)), description(std::move(description_)), fields(std::move(fields_)),
          latestMessageCrc(latestMessageCrc_)
    {
    }

    //----------------------------------------------------------------------------
    //! \brief Deep copy. The default copy would alias every `BaseField::Ptr`
    //! in `fields`; this walks each variant and replaces it with
    //! `field->clone()` so the new definition shares no field storage with
    //! the source.
    //----------------------------------------------------------------------------
    MessageDefinition(const MessageDefinition& other)
        : _id(other._id), logID(other.logID), name(other.name), description(other.description), latestMessageCrc(other.latestMessageCrc)
    {
        for (const auto& [crc, fieldVec] : other.fields)
        {
            auto& copyVec = fields[crc];
            copyVec.reserve(fieldVec.size());
            for (const auto& f : fieldVec) { copyVec.push_back(f ? f->clone() : nullptr); }
        }
    }

    MessageDefinition& operator=(const MessageDefinition& other)
    {
        if (this == &other) { return *this; }
        _id = other._id;
        logID = other.logID;
        name = other.name;
        description = other.description;
        latestMessageCrc = other.latestMessageCrc;
        fields.clear();
        for (const auto& [crc, fieldVec] : other.fields)
        {
            auto& copyVec = fields[crc];
            copyVec.reserve(fieldVec.size());
            for (const auto& f : fieldVec) { copyVec.push_back(f ? f->clone() : nullptr); }
        }
        return *this;
    }

    MessageDefinition(MessageDefinition&&) = default;
    MessageDefinition& operator=(MessageDefinition&&) = default;

    using Ptr = std::shared_ptr<MessageDefinition>;
    using ConstPtr = std::shared_ptr<const MessageDefinition>;
};

//============================================================================
//! \class MessageDatabase
//! \brief Holds the definitions of NovAtel messages and enums.
//============================================================================
class MessageDatabase
{
  protected:
    DbMetadata::Ptr pDbMetadata;
    std::vector<MessageDefinition::ConstPtr> vMessageDefinitions;
    std::vector<EnumDefinition::ConstPtr> vEnumDefinitions;
    std::unordered_map<std::string_view, MessageDefinition::ConstPtr> mMessageName;
    std::unordered_map<int32_t, MessageDefinition::ConstPtr> mMessageId;
    std::unordered_map<std::string_view, EnumDefinition::ConstPtr> mEnumName;
    std::unordered_map<std::string_view, EnumDefinition::ConstPtr> mEnumId;

  public:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the MessageDatabase class.
    //----------------------------------------------------------------------------
    MessageDatabase() = default;

    //----------------------------------------------------------------------------
    //! \brief A constructor for the MessageDatabase class.
    //
    //! \param[in] vMessageDefinitions_ A vector of message definitions
    //! \param[in] vEnumDefinitions_ A vector of enum definitions
    //----------------------------------------------------------------------------
    MessageDatabase(std::vector<MessageDefinition::ConstPtr> vMessageDefinitions_, std::vector<EnumDefinition::ConstPtr> vEnumDefinitions_)
        : vMessageDefinitions(std::move(vMessageDefinitions_)), vEnumDefinitions(std::move(vEnumDefinitions_))
    {
        GenerateEnumMappings();
        GenerateMessageMappings();
    }

    //----------------------------------------------------------------------------
    //! \brief A constructor for the MessageDatabase class.
    //
    //! \param[in] vMessageDefinitions_ A vector of message definitions
    //! \param[in] vEnumDefinitions_ A vector of enum definitions
    //! \param[in] pDbMetadata_ Database metadata
    //----------------------------------------------------------------------------
    MessageDatabase(std::vector<MessageDefinition::ConstPtr> vMessageDefinitions_, std::vector<EnumDefinition::ConstPtr> vEnumDefinitions_,
                    DbMetadata::Ptr pDbMetadata_)
        : pDbMetadata(std::move(pDbMetadata_)), vMessageDefinitions(std::move(vMessageDefinitions_)), vEnumDefinitions(std::move(vEnumDefinitions_))
    {
        GenerateEnumMappings();
        GenerateMessageMappings();
    }

    //----------------------------------------------------------------------------
    //! \brief Destructor for the MessageDatabase class.
    //----------------------------------------------------------------------------
    virtual ~MessageDatabase() = default;

    //----------------------------------------------------------------------------
    //! \brief Merge the message and enum definitions from another MessageDatabase into this one.
    //
    //! \param[in] other_ The other MessageDatabase object to merge.
    //----------------------------------------------------------------------------
    void Merge(const MessageDatabase& other_)
    {
        std::thread enumThread([this, &other_]() { AppendEnumerations(other_.vEnumDefinitions); });
        std::thread messageThread([this, &other_]() { AppendMessages(other_.vMessageDefinitions); });

        enumThread.join();
        messageThread.join();
    }

    //----------------------------------------------------------------------------
    //! \brief Append a list of message definitions to the database.
    //
    //! \param[in] vMessageDefinitions_ A vector of message definitions
    //----------------------------------------------------------------------------
    void AppendMessages(const std::vector<MessageDefinition::ConstPtr>& vMessageDefinitions_)
    {
        for (const auto& msgDef : vMessageDefinitions_)
        {
            RemoveMessage(msgDef->logID);
            vMessageDefinitions.push_back(msgDef);
            mMessageName[msgDef->name] = msgDef;
            mMessageId[msgDef->logID] = msgDef;

            for (const auto& item : msgDef->fieldInfo)
            {
                if (!item.second.messageOrderedFields.empty()) { MapMessageEnumFields(item.second.messageOrderedFields); }
            }
        }
    }

    //----------------------------------------------------------------------------
    //! \brief Append a list of enum definitions to the database.
    //
    //! \param[in] vEnumDefinitions_ A vector of enum definitions
    //----------------------------------------------------------------------------
    void AppendEnumerations(const std::vector<EnumDefinition::ConstPtr>& vEnumDefinitions_)
    {
        for (const auto& enmDef : vEnumDefinitions_)
        {
            RemoveEnumeration(enmDef->name);
            vEnumDefinitions.push_back(enmDef);
            mEnumName[enmDef->name] = enmDef;
            mEnumId[enmDef->_id] = enmDef;
        }
    }

    //----------------------------------------------------------------------------
    //! \brief Append a message Json DB from the provided filepath.
    //
    //! \param[in] iMsgId_ The message ID
    //----------------------------------------------------------------------------
    void RemoveMessage(uint32_t iMsgId_);

    //----------------------------------------------------------------------------
    //! \brief Append an enumeration Json DB from the provided filepath.
    //
    //! \param[in] strEnumeration_ The enumeration name
    //----------------------------------------------------------------------------
    void RemoveEnumeration(std::string_view strEnumeration_);

    //----------------------------------------------------------------------------
    //! \brief Get a UI DB message definition for the provided message name.
    //
    //! \param[in] strMsgName_ A string containing the message name.
    //----------------------------------------------------------------------------
    [[nodiscard]] MessageDefinition::ConstPtr GetMsgDef(std::string_view strMsgName_) const;

    //----------------------------------------------------------------------------
    //! \brief Get a UI DB message definition for the provided message ID.
    //
    //! \param[in] iMsgId_ The message ID.
    //----------------------------------------------------------------------------
    [[nodiscard]] MessageDefinition::ConstPtr GetMsgDef(int32_t iMsgId_) const;

    //----------------------------------------------------------------------------
    //! \brief Convert a message name string to a message ID number.
    //
    //! \param[in] sMsgName_ The message name string
    //----------------------------------------------------------------------------
    [[nodiscard]] uint32_t MsgNameToMsgId(std::string sMsgName_) const;

    //----------------------------------------------------------------------------
    //! \brief Convert a message ID number to a message name string.
    //
    //! \param[in] uiMessageId_ The message ID number
    //----------------------------------------------------------------------------
    [[nodiscard]] std::string MsgIdToMsgName(uint32_t uiMessageId_) const;

    //----------------------------------------------------------------------------
    //! \brief Get a UI DB enum definition for the provided enum ID.
    //
    //! \param[in] sEnumId_ The enum ID.
    //----------------------------------------------------------------------------
    [[nodiscard]] EnumDefinition::ConstPtr GetEnumDefId(const std::string& sEnumId_) const
    {
        const auto it = mEnumId.find(sEnumId_);
        return it != mEnumId.end() ? it->second : nullptr;
    }

    //----------------------------------------------------------------------------
    //! \brief Get a UI DB enum definition for the provided enum name.
    //
    //! \param[in] sEnumName_ The enum name.
    //----------------------------------------------------------------------------
    [[nodiscard]] EnumDefinition::ConstPtr GetEnumDefName(const std::string& sEnumName_) const
    {
        auto it = mEnumName.find(sEnumName_);
        return it != mEnumName.end() ? it->second : nullptr;
    }

    //----------------------------------------------------------------------------
    //! \brief Returns all defined enums.
    //----------------------------------------------------------------------------
    [[nodiscard]] const std::vector<EnumDefinition::ConstPtr>& EnumDefinitions() const { return vEnumDefinitions; }

    //----------------------------------------------------------------------------
    //! \brief Returns all defined message types.
    //----------------------------------------------------------------------------
    [[nodiscard]] const std::vector<MessageDefinition::ConstPtr>& MessageDefinitions() const { return vMessageDefinitions; }

    //----------------------------------------------------------------------------
    //! \brief Returns DB metadata.
    //----------------------------------------------------------------------------
    [[nodiscard]] DbMetadata::ConstPtr GetDbMetadata() const { return pDbMetadata; }

    //----------------------------------------------------------------------------
    //! \brief Sets the message family on the DB metadata, creating it if absent.
    //----------------------------------------------------------------------------
    void SetMessageFamily(const std::string& messageFamily_)
    {
        if (!pDbMetadata) { pDbMetadata = std::make_shared<DbMetadata>(); }
        pDbMetadata->messageFamily = messageFamily_;
    }

    //----------------------------------------------------------------------------
    //! \brief Registers an alignment function for a given message family.
    //!
    //! \param[in] messageFamily_ The message family for which to register the alignment function
    //! \param[in] fn The alignment function to register, which takes the field length,
    //!     a pointer to the start of the message body, and a pointer to the current position,
    //!     and returns the number of bytes to move forward to align the field.
    //----------------------------------------------------------------------------
    static void RegisterAlignmentFunction(std::string messageFamily_, std::function<size_t(const size_t, const uintptr_t, const uintptr_t)> fn)
    {
        GetAlignmentFunctions()[messageFamily_] = std::move(fn);
    }

    //----------------------------------------------------------------------------
    //! \brief Retrieves the alignment functions map.
    //! \return A reference to the map of message family names to their corresponding alignment functions.
    //----------------------------------------------------------------------------
    static std::unordered_map<std::string, std::function<size_t(const size_t, const uintptr_t, const uintptr_t)>>& GetAlignmentFunctions()
    {
        static std::unordered_map<std::string, std::function<size_t(const size_t, const uintptr_t, const uintptr_t)>> alignmentFunctions;
        return alignmentFunctions;
    }

    //----------------------------------------------------------------------------
    //! \brief A no-op alignment function that always returns 0.
    //----------------------------------------------------------------------------
    static size_t NoAlign(uint8_t, const uintptr_t, const uintptr_t) noexcept { return 0; }

  protected:
    virtual void GenerateEnumMappings()
    {
        for (auto& enm : vEnumDefinitions)
        {
            mEnumName[enm->name] = enm;
            mEnumId[enm->_id] = enm;
        }
    }

    virtual void GenerateMessageMappings()
    {
        for (auto& msg : vMessageDefinitions)
        {
            mMessageName[msg->name] = msg;
            mMessageId[msg->logID] = msg;

            for (const auto& item : msg->fieldInfo)
            {
                if (!item.second.messageOrderedFields.empty()) { MapMessageEnumFields(item.second.messageOrderedFields); }
            }
        }
    }

  private:
    void MapMessageEnumFields(const std::vector<BaseField::ConstPtr>& vMsgDefFields_)
    {
        for (const auto& field : vMsgDefFields_)
        {
            if (field->type == FIELD_TYPE::ENUM)
            {
                auto enumField = std::dynamic_pointer_cast<const EnumField>(field);
                if (!enumField) { continue; }

                // Definitions are stored as ConstPtr but enumDef is populated after loading DBs.
                std::const_pointer_cast<EnumField>(enumField)->enumDef = GetEnumDefId(enumField->enumId);
            }
            else if (field->type == FIELD_TYPE::FIELD_ARRAY)
            {
                auto fieldArrayField = std::dynamic_pointer_cast<const FieldArrayField>(field);
                if (!fieldArrayField || fieldArrayField->fieldInfo.messageOrderedFields.empty()) { continue; }
                MapMessageEnumFields(fieldArrayField->fieldInfo.messageOrderedFields);
            }
        }
    }

    void RemoveMessageMapping(const MessageDefinition& msg_)
    {
        // Check string against name map
        const auto itName = mMessageName.find(msg_.name);
        if (itName != mMessageName.end()) { mMessageName.erase(itName); }

        const auto itId = mMessageId.find(msg_.logID);
        if (itId != mMessageId.end()) { mMessageId.erase(itId); }
    }

    void RemoveEnumerationMapping(const EnumDefinition& enm_)
    {
        // Check string against name map
        const auto itName = mEnumName.find(enm_.name);
        if (itName != mEnumName.end()) { mEnumName.erase(itName); }

        const auto itId = mEnumId.find(enm_._id);
        if (itId != mEnumId.end()) { mEnumId.erase(itId); }
    }

    std::vector<MessageDefinition::ConstPtr>::iterator GetMessageIt(uint32_t iMsgId_)
    {
        return std::find_if(vMessageDefinitions.begin(), vMessageDefinitions.end(), //
                            [iMsgId_](const auto& elem_) { return elem_->logID == iMsgId_; });
    }

    std::vector<MessageDefinition::ConstPtr>::iterator GetMessageIt(std::string_view strMessage_)
    {
        return std::find_if(vMessageDefinitions.begin(), vMessageDefinitions.end(), //
                            [strMessage_](const auto& elem_) { return elem_->name == strMessage_; });
    }

    std::vector<EnumDefinition::ConstPtr>::iterator GetEnumIt(std::string_view strEnumeration_)
    {
        return std::find_if(vEnumDefinitions.begin(), vEnumDefinitions.end(), //
                            [strEnumeration_](const auto& elem_) { return elem_->name == strEnumeration_; });
    }

  public:
    using Ptr = std::shared_ptr<MessageDatabase>;
    using ConstPtr = std::shared_ptr<const MessageDatabase>;
};

//----------------------------------------------------------------------------
//! \brief Report a missing message definition. Logs at info the first time a
//! given ID is reported on the calling thread, and at debug thereafter.
//
//! \param[in] pclLogger_ The logger to emit the notice through.
//! \param[in] iMsgId_ The message ID with no matching definition.
//----------------------------------------------------------------------------
void LogMissingMsgDef(spdlog::logger& pclLogger_, int32_t iMsgId_);

} // namespace novatel::edie

#endif
