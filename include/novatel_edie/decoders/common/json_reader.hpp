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
// ! \file json_reader.hpp
// ===============================================================================

#ifndef JSON_READER_HPP
#define JSON_READER_HPP

#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "novatel_edie/common/crc32.hpp"
#include "novatel_edie/common/logger.hpp"

namespace novatel::edie {

using nlohmann::json;

//============================================================================
//! \class JsonReaderFailure
//! \brief Exception to be thrown when JsonReader fails to parse a JSON.
//============================================================================
class JsonReaderFailure : public std::exception
{
  private:
    const char* func;
    const char* file;
    int32_t line;
    std::filesystem::path clFilePath;
    const char* failure;
    char acWhatString[256]{};

  public:
    JsonReaderFailure(const char* func_, const char* file_, const int32_t line_, std::filesystem::path jsonFile_, const char* failure_)
        : func(func_), file(file_), line(line_), clFilePath(std::move(jsonFile_)), failure(failure_)
    {
    }

    [[nodiscard]] const char* what() const noexcept override
    {
        sprintf(const_cast<char*>(acWhatString), "In file \"%s\" : %s() (Line %d)\n\t\"%s: %s.\"", file, func, line,
                clFilePath.generic_string().c_str(), failure);
        return acWhatString;
    }
};

//-----------------------------------------------------------------------
//! \enum DataType
//! \brief Data type name string represented as an enum.
//-----------------------------------------------------------------------
enum class DataType
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
constexpr size_t DataTypeSize(const DataType eType_)
{
    return eType_ == DataType::BOOL          ? sizeof(int32_t)
           : eType_ == DataType::HEXBYTE     ? sizeof(uint8_t)
           : eType_ == DataType::CHAR        ? sizeof(int8_t)
           : eType_ == DataType::UCHAR       ? sizeof(uint8_t)
           : eType_ == DataType::SHORT       ? sizeof(int16_t)
           : eType_ == DataType::USHORT      ? sizeof(uint16_t)
           : eType_ == DataType::INT         ? sizeof(int32_t)
           : eType_ == DataType::UINT        ? sizeof(uint32_t)
           : eType_ == DataType::LONG        ? sizeof(int32_t)
           : eType_ == DataType::ULONG       ? sizeof(uint32_t)
           : eType_ == DataType::LONGLONG    ? sizeof(int64_t)
           : eType_ == DataType::ULONGLONG   ? sizeof(uint64_t)
           : eType_ == DataType::FLOAT       ? sizeof(float)
           : eType_ == DataType::DOUBLE      ? sizeof(double)
           : eType_ == DataType::SATELLITEID ? sizeof(uint32_t)
                                             : 0;
}

// TODO: this table is misleading, as one DataType may correspond to many different conversion strings
//!< returns conversion string associated with a datatype
inline std::string DataTypeConversion(const DataType eType_)
{
    return eType_ == DataType::BOOL          ? "%d"
           : eType_ == DataType::CHAR        ? "%c"
           : eType_ == DataType::UCHAR       ? "%uc"
           : eType_ == DataType::SHORT       ? "%hd"
           : eType_ == DataType::USHORT      ? "%hu"
           : eType_ == DataType::INT         ? "%d"
           : eType_ == DataType::UINT        ? "%u"
           : eType_ == DataType::LONG        ? "%ld"
           : eType_ == DataType::ULONG       ? "%lu"
           : eType_ == DataType::LONGLONG    ? "%lld"
           : eType_ == DataType::ULONGLONG   ? "%llu"
           : eType_ == DataType::FLOAT       ? "%f"
           : eType_ == DataType::DOUBLE      ? "%lf"
           : eType_ == DataType::HEXBYTE     ? "%Z" // these are not valid default conversion strings
           : eType_ == DataType::SATELLITEID ? "%id"
                                             : "";
}

//!< Mapping from String to data type enums.
static const std::unordered_map<std::string, DataType> DataTypeEnumLookup = {
    {"BOOL", DataType::BOOL},      {"HEXBYTE", DataType::HEXBYTE},   {"CHAR", DataType::CHAR},
    {"UCHAR", DataType::UCHAR},    {"SHORT", DataType::SHORT},       {"USHORT", DataType::USHORT},
    {"INT", DataType::INT},        {"UINT", DataType::UINT},         {"LONG", DataType::LONG},
    {"ULONG", DataType::ULONG},    {"LONGLONG", DataType::LONGLONG}, {"ULONGLONG", DataType::ULONGLONG},
    {"FLOAT", DataType::FLOAT},    {"DOUBLE", DataType::DOUBLE},     {"SATELLITEID", DataType::SATELLITEID},
    {"UNKNOWN", DataType::UNKNOWN}};

//-----------------------------------------------------------------------
//! \enum FieldType
//! \brief Field type string represented as an enum.
//-----------------------------------------------------------------------
enum class FieldType
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
static const std::unordered_map<std::string, FieldType> FieldTypeEnumLookup = {{"SIMPLE", FieldType::SIMPLE},
                                                                               {"ENUM", FieldType::ENUM},
                                                                               {"BITFIELD", FieldType::BITFIELD},
                                                                               {"FIXED_LENGTH_ARRAY", FieldType::FIXED_LENGTH_ARRAY},
                                                                               {"VARIABLE_LENGTH_ARRAY", FieldType::VARIABLE_LENGTH_ARRAY},
                                                                               {"STRING", FieldType::STRING},
                                                                               {"FIELD_ARRAY", FieldType::FIELD_ARRAY},
                                                                               {"UNKNOWN", FieldType::UNKNOWN}};

//-----------------------------------------------------------------------
//! \struct EnumDataType
//! \brief Enum Data Type representing contents of UI DB.
//-----------------------------------------------------------------------
struct EnumDataType
{
    uint32_t value{0};
    std::string name{};
    std::string description{};

    constexpr EnumDataType() = default;
};

//-----------------------------------------------------------------------
//! \struct EnumDefinition
//! \brief Enum Definition representing contents of UI DB.
//-----------------------------------------------------------------------
struct EnumDefinition
{
    std::string _id{};
    std::string name{};
    std::vector<EnumDataType> enumerators{};

    constexpr EnumDefinition() = default;
};

//-----------------------------------------------------------------------
//! \struct BaseDataType
//! \brief Struct containing basic elements of data type fields in the
//! UI DB.
//-----------------------------------------------------------------------
struct BaseDataType
{
    DataType name{DataType::UNKNOWN};
    uint16_t length{0};
    std::string description{};

    constexpr BaseDataType() = default;
};

//-----------------------------------------------------------------------
//! \struct SimpleDataType
//! \brief Struct containing elements of simple data type fields in the
//! UI DB.
//-----------------------------------------------------------------------
struct SimpleDataType : BaseDataType
{
    std::unordered_map<int32_t, EnumDataType> enums;
};

//-----------------------------------------------------------------------
//! \struct BaseField
//! \brief Struct containing elements of basic fields in the UI DB.
//-----------------------------------------------------------------------
struct BaseField
{
    std::string name;
    FieldType type{FieldType::UNKNOWN};
    std::string description;
    std::string conversion;
    std::string sConversionStripped;
    uint32_t conversionHash{0ULL};
    int32_t conversionBeforePoint{0};
    int32_t conversionAfterPoint{0};
    SimpleDataType dataType;

    BaseField() = default;

    BaseField(std::string name_, const FieldType type_, const std::string& sConversion_, const size_t length_, const DataType eDataTypeName_)
        : name(std::move(name_)), type(type_)
    {
        SetConversion(sConversion_);
        dataType.length = static_cast<uint16_t>(length_);
        dataType.name = eDataTypeName_;
    }

    virtual ~BaseField() = default;

    virtual BaseField* Clone() { return new BaseField(*this); }

    void SetConversion(const std::string& sConversion_)
    {
        conversion = sConversion_;
        ParseConversion(sConversionStripped, conversionBeforePoint, conversionAfterPoint);
        conversionHash = CalculateBlockCrc32(sConversionStripped.c_str());
    }

    void ParseConversion(std::string& strStrippedConversionString_, int32_t& iBeforePoint_, int32_t& iAfterPoint_) const
    {
        const char* sConvertString = conversion.c_str();
        int32_t* iSelectedPoint = &iBeforePoint_;

        while (*sConvertString != 0)
        {
            // "0x" could be a prefix on a hex field (0x%...) or a valid conversion string (%0x).
            // Prevent these two cases from occurring at the same time.
            if ((0 == memcmp(sConvertString, "0x", 2)) && (0 != strcmp(sConvertString, "0x"))) { sConvertString += 2; }

            // If the value "10" or greater is found from the conversion string, two bytes would
            // need to be consumed from the string to move past that value. Otherwise, only one byte
            // is necessary to consume.
            if (*sConvertString >= '0' && *sConvertString <= '9')
            {
                if (sscanf(sConvertString, "%d.", iSelectedPoint) != 1) { throw std::runtime_error("Failed to parse integer value"); }
                sConvertString = sConvertString + (*iSelectedPoint > 9 ? 2 : 1); // Skip the numerals
            }

            if ((std::isalpha(*sConvertString) != 0) || *sConvertString == '%')
            {
                strStrippedConversionString_.push_back(sConvertString[0]);
                sConvertString++;
            }

            if (*sConvertString == '.')
            {
                iSelectedPoint = &iAfterPoint_;
                sConvertString++;
            }
        }
    }

    [[nodiscard]] bool IsString() const
    {
        return type == FieldType::STRING || conversionHash == CalculateBlockCrc32("%s") || conversionHash == CalculateBlockCrc32("%S");
    }

    [[nodiscard]] bool IsCsv() const
    {
        return !IsString() && conversionHash != CalculateBlockCrc32("%Z") && conversionHash != CalculateBlockCrc32("%P");
    }
};

//-----------------------------------------------------------------------
//! \struct EnumField
//! \brief Struct containing elements of enum fields in the UI DB.
//-----------------------------------------------------------------------
struct EnumField : BaseField
{
    std::string enumId;
    EnumDefinition* enumDef{nullptr};
    uint32_t length{0};

    EnumField() = default;

    ~EnumField() override = default;

    EnumField* Clone() override { return new EnumField(*this); }
};

//-----------------------------------------------------------------------
//! \struct ArrayField
//! \brief Struct containing elements of array fields in the UI DB.
//-----------------------------------------------------------------------
struct ArrayField : BaseField
{
    uint32_t arrayLength{0};

    ArrayField() = default;

    ~ArrayField() override = default;

    ArrayField* Clone() override { return new ArrayField(*this); }
};

//-----------------------------------------------------------------------
//! \struct FieldArrayField
//! \brief Struct containing elements of field array fields in the UI DB.
//-----------------------------------------------------------------------
struct FieldArrayField : BaseField
{
    uint32_t arrayLength{0}, fieldSize{0};
    std::vector<BaseField*> fields;

    FieldArrayField() = default;

    ~FieldArrayField() override
    {
        for (const auto& field : fields) { delete field; }
    }

    FieldArrayField(const FieldArrayField& that_) : BaseField(that_)
    {
        for (const auto& field : that_.fields) { fields.emplace_back(field->Clone()); }

        arrayLength = that_.arrayLength;
        fieldSize = that_.fieldSize;
    }

    FieldArrayField& operator=(const FieldArrayField that_)
    {
        if (this != &that_)
        {
            BaseField::operator=(that_);

            fields.clear();
            for (const auto& field : that_.fields) { fields.emplace_back(field->Clone()); }

            arrayLength = that_.arrayLength;
            fieldSize = that_.fieldSize;
        }

        return *this;
    }

    FieldArrayField* Clone() override { return new FieldArrayField(*this); }
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
    std::unordered_map<uint32_t, std::vector<BaseField*>> fields; // map of crc keys to field definitions
    uint32_t latestMessageCrc{0};

    MessageDefinition() = default;
    MessageDefinition(const MessageDefinition& that_)
    {
        for (const auto& fieldDefinition : that_.fields)
        {
            uint32_t key = fieldDefinition.first;
            // Ensure a 0-length vector exists for this key in the case the message has no fields.
            fields[key] = std::vector<BaseField*>();
            for (const auto& field : fieldDefinition.second) { fields[key].emplace_back(field->Clone()); }
        }

        _id = that_._id;
        logID = that_.logID;
        name = that_.name;
        description = that_.description;
        latestMessageCrc = that_.latestMessageCrc;
    }

    ~MessageDefinition()
    {
        for (auto& item : fields)
        {
            for (auto* f : item.second) { delete f; }
        }
    }

    MessageDefinition& operator=(MessageDefinition that_)
    {
        if (this != &that_)
        {
            fields.clear();
            for (const auto& fieldDefinition : that_.fields)
            {
                uint32_t key = fieldDefinition.first;
                // Ensure a 0-length vector exists for this key in the case the message has no fields.
                fields[key] = std::vector<BaseField*>();
                for (const auto& field : fieldDefinition.second) { fields[key].emplace_back(field->Clone()); }
            }

            _id = that_._id;
            logID = that_.logID;
            name = that_.name;
            description = that_.description;
            latestMessageCrc = that_.latestMessageCrc;
        }

        return *this;
    }

    const std::vector<BaseField*>* GetMsgDefFromCrc(const std::shared_ptr<spdlog::logger>& pclLogger_, uint32_t& uiMsgDefCrc_) const;
};

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
uint32_t ParseFields(const json& j_, std::vector<BaseField*>& vFields_);
void ParseEnumerators(const json& j_, std::vector<EnumDataType>& vEnumerators_);

//============================================================================
//! \class JsonReader
//! \brief Responsible for translating the Json representation of the
//! NovAtel UI DB.
//============================================================================
class JsonReader
{
    std::vector<MessageDefinition> vMessageDefinitions;
    std::vector<EnumDefinition> vEnumDefinitions;
    std::unordered_map<std::string, MessageDefinition*> mMessageName;
    std::unordered_map<int32_t, MessageDefinition*> mMessageId;
    std::unordered_map<std::string, EnumDefinition*> mEnumName;
    std::unordered_map<std::string, EnumDefinition*> mEnumId;

  public:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the JsonReader class.
    //----------------------------------------------------------------------------
    JsonReader() = default;

    //----------------------------------------------------------------------------
    //! \brief A copy constructor for the JsonReader class.
    //
    //! \param[in] that_ The JsonReader object to copy.
    //----------------------------------------------------------------------------
    JsonReader(const JsonReader& that_)
    {
        // TODO: Verify it's calling the copy constructor for the messages
        vEnumDefinitions = that_.vEnumDefinitions;
        vMessageDefinitions = that_.vMessageDefinitions;
        GenerateMappings();
    }

    //----------------------------------------------------------------------------
    //! \brief Overloaded assignment operator for the JsonReader class.
    //
    //! \param[in] that_ The JsonReader object to assign.
    //----------------------------------------------------------------------------
    JsonReader& operator=(const JsonReader& that_)
    {
        if (this != &that_)
        {
            vEnumDefinitions = that_.vEnumDefinitions;
            vMessageDefinitions = that_.vMessageDefinitions;
            GenerateMappings();
        }

        return *this;
    }

    //----------------------------------------------------------------------------
    //! \brief Destructor for the JsonReader class.
    //----------------------------------------------------------------------------
    ~JsonReader() = default;

    //----------------------------------------------------------------------------
    //! \brief Load a Json DB from the provided filepath.
    //
    //! \param[in] filePath_ The filepath to the Json file.
    //----------------------------------------------------------------------------
    template <typename T> void LoadFile(T filePath_);

    //----------------------------------------------------------------------------
    //! \brief Append a message Json DB from the provided filepath.
    //
    //! \param[in] filePath_ The filepath to the Json file.
    //----------------------------------------------------------------------------
    template <typename T> void AppendMessages(T filePath_);

    //----------------------------------------------------------------------------
    //! \brief Append an enumeration Json DB from the provided filepath.
    //
    //! \param[in] filePath_ The filepath to the Json file.
    //----------------------------------------------------------------------------
    template <typename T> void AppendEnumerations(T filePath_);

    //----------------------------------------------------------------------------
    //! \brief Append a message Json DB from the provided filepath.
    //
    //! \param[in] iMsgId_ The message ID
    //! \param[in] bGenerateMappings_ Boolean for generating mappings
    //----------------------------------------------------------------------------
    void RemoveMessage(uint32_t iMsgId_, bool bGenerateMappings_ = true);

    //----------------------------------------------------------------------------
    //! \brief Append an enumeration Json DB from the provided filepath.
    //
    //! \param[in] strEnumeration_ The enumeration name
    //! \param[in] bGenerateMappings_ Boolean for generating mappings
    //----------------------------------------------------------------------------
    void RemoveEnumeration(const std::string& strEnumeration_, bool bGenerateMappings_);

    //----------------------------------------------------------------------------
    //! \brief Parse the Json string provided.
    //
    //! \param[in] strJsonData_ A string containing Json objects.
    //----------------------------------------------------------------------------
    void ParseJson(const std::string& strJsonData_);

    //----------------------------------------------------------------------------
    //! \brief Get a UI DB message definition for the provided message name.
    //
    //! \param[in] strMsgName_ A string containing the message name.
    //----------------------------------------------------------------------------
    [[nodiscard]] const MessageDefinition* GetMsgDef(const std::string& strMsgName_) const;

    //----------------------------------------------------------------------------
    //! \brief Get a UI DB message definition for the provided message ID.
    //
    //! \param[in] iMsgId_ The message ID.
    //----------------------------------------------------------------------------
    [[nodiscard]] const MessageDefinition* GetMsgDef(int32_t iMsgId_) const;

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
    [[nodiscard]] EnumDefinition* GetEnumDefId(const std::string& sEnumId_) const
    {
        const auto it = mEnumId.find(sEnumId_);
        return it != mEnumId.end() ? it->second : nullptr;
    }

    //----------------------------------------------------------------------------
    //! \brief Get a UI DB enum definition for the provided enum name.
    //
    //! \param[in] sEnumName_ The enum name.
    //----------------------------------------------------------------------------
    [[nodiscard]] EnumDefinition* GetEnumDefName(const std::string& sEnumName_) const
    {
        auto it = mEnumName.find(sEnumName_);
        return it != mEnumName.end() ? it->second : nullptr;
    }

  private:
    void GenerateMappings()
    {
        for (EnumDefinition& enm : vEnumDefinitions)
        {
            mEnumName[enm.name] = &enm;
            mEnumId[enm._id] = &enm;
        }

        for (MessageDefinition& msg : vMessageDefinitions)
        {
            mMessageName[msg.name] = &msg;
            mMessageId[msg.logID] = &msg;

            for (const auto& item : msg.fields) { MapMessageEnumFields(item.second); }
        }
    }

    void MapMessageEnumFields(const std::vector<BaseField*>& vMsgDefFields_)
    {
        for (const auto& field : vMsgDefFields_)
        {
            if (field->type == FieldType::ENUM)
            {
                dynamic_cast<EnumField*>(field)->enumDef = GetEnumDefId(dynamic_cast<const EnumField*>(field)->enumId);
            }
            else if (field->type == FieldType::FIELD_ARRAY) { MapMessageEnumFields(dynamic_cast<FieldArrayField*>(field)->fields); }
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

    std::vector<MessageDefinition>::iterator GetMessageIt(uint32_t iMsgId_)
    {
        return std::find_if(vMessageDefinitions.begin(), vMessageDefinitions.end(),
                            [iMsgId_](const MessageDefinition& elem_) { return (elem_.logID == iMsgId_); });
    }

    std::vector<MessageDefinition>::iterator GetMessageIt(const std::string& strMessage_)
    {
        return std::find_if(vMessageDefinitions.begin(), vMessageDefinitions.end(),
                            [strMessage_](const MessageDefinition& elem_) { return (elem_.name == strMessage_); });
    }

    std::vector<EnumDefinition>::iterator GetEnumIt(const std::string& strEnumeration_)
    {
        return std::find_if(vEnumDefinitions.begin(), vEnumDefinitions.end(),
                            [strEnumeration_](const EnumDefinition& elem_) { return (elem_.name == strEnumeration_); });
    }
};

} // namespace novatel::edie

#endif
