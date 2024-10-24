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
    std::string whatString;

  public:
    JsonReaderFailure(const char* func_, const char* file_, const int32_t line_, const std::filesystem::path& json_, const char* failure_)
    {
        std::ostringstream oss;
        oss << "In file \"" << file_ << "\" : " << func_ << "() (Line " << line_ << ")\n\t\"" << json_.generic_string() << ": " << failure_ << ".\"";
        whatString = oss.str();
    }

    [[nodiscard]] const char* what() const noexcept override { return whatString.c_str(); }
};

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
    return eType_ == DATA_TYPE::BOOL          ? sizeof(int32_t)
           : eType_ == DATA_TYPE::HEXBYTE     ? sizeof(uint8_t)
           : eType_ == DATA_TYPE::CHAR        ? sizeof(int8_t)
           : eType_ == DATA_TYPE::UCHAR       ? sizeof(uint8_t)
           : eType_ == DATA_TYPE::SHORT       ? sizeof(int16_t)
           : eType_ == DATA_TYPE::USHORT      ? sizeof(uint16_t)
           : eType_ == DATA_TYPE::INT         ? sizeof(int32_t)
           : eType_ == DATA_TYPE::UINT        ? sizeof(uint32_t)
           : eType_ == DATA_TYPE::LONG        ? sizeof(int32_t)
           : eType_ == DATA_TYPE::ULONG       ? sizeof(uint32_t)
           : eType_ == DATA_TYPE::LONGLONG    ? sizeof(int64_t)
           : eType_ == DATA_TYPE::ULONGLONG   ? sizeof(uint64_t)
           : eType_ == DATA_TYPE::FLOAT       ? sizeof(float)
           : eType_ == DATA_TYPE::DOUBLE      ? sizeof(double)
           : eType_ == DATA_TYPE::SATELLITEID ? sizeof(uint32_t)
                                              : 0;
}

// TODO: this table is misleading, as one DATA_TYPE may correspond to many different conversion strings
//!< returns conversion string associated with a datatype
inline std::string DataTypeConversion(const DATA_TYPE eType_)
{
    return eType_ == DATA_TYPE::BOOL          ? "%d"
           : eType_ == DATA_TYPE::CHAR        ? "%c"
           : eType_ == DATA_TYPE::UCHAR       ? "%uc"
           : eType_ == DATA_TYPE::SHORT       ? "%hd"
           : eType_ == DATA_TYPE::USHORT      ? "%hu"
           : eType_ == DATA_TYPE::INT         ? "%d"
           : eType_ == DATA_TYPE::UINT        ? "%u"
           : eType_ == DATA_TYPE::LONG        ? "%ld"
           : eType_ == DATA_TYPE::ULONG       ? "%lu"
           : eType_ == DATA_TYPE::LONGLONG    ? "%lld"
           : eType_ == DATA_TYPE::ULONGLONG   ? "%llu"
           : eType_ == DATA_TYPE::FLOAT       ? "%f"
           : eType_ == DATA_TYPE::DOUBLE      ? "%lf"
           : eType_ == DATA_TYPE::HEXBYTE     ? "%Z" // these are not valid default conversion strings
           : eType_ == DATA_TYPE::SATELLITEID ? "%id"
                                              : "%";
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
    DATA_TYPE name{DATA_TYPE::UNKNOWN};
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
    FIELD_TYPE type{FIELD_TYPE::UNKNOWN};
    std::string description;
    std::string conversion;
    uint32_t conversionHash{0ULL};
    int32_t conversionBeforePoint{0};
    int32_t conversionAfterPoint{0};
    SimpleDataType dataType;

    BaseField() = default;

    BaseField(std::string name_, const FIELD_TYPE type_, const std::string& sConversion_, const size_t length_, const DATA_TYPE eDataTypeName_)
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

        const char* sConvertString = conversion.c_str();

        if (*sConvertString != '%') { throw std::runtime_error("Encountered an unexpected character in conversion string"); }

        ++sConvertString;

        if (std::isdigit(*sConvertString))
        {
            conversionBeforePoint = std::stoi(sConvertString);
            sConvertString += std::to_string(conversionBeforePoint).length();
        }

        if (*sConvertString == '.') { ++sConvertString; }

        if (std::isdigit(*sConvertString))
        {
            conversionAfterPoint = std::stoi(sConvertString);
            sConvertString += std::to_string(conversionAfterPoint).length();
        }

        conversionHash = 0;

        while (std::isalpha(*sConvertString)) { CalculateCharacterCrc32(conversionHash, *sConvertString++); }

        if (*sConvertString != '\0') { throw std::runtime_error("Encountered an unexpected character in conversion string"); }
    }

    [[nodiscard]] bool IsString() const
    {
        return type == FIELD_TYPE::STRING || conversionHash == CalculateBlockCrc32("s") || conversionHash == CalculateBlockCrc32("S");
    }

    [[nodiscard]] bool IsCsv() const
    {
        return !IsString() && conversionHash != CalculateBlockCrc32("Z") && conversionHash != CalculateBlockCrc32("P");
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
    void RemoveEnumeration(std::string_view strEnumeration_, bool bGenerateMappings_);

    //----------------------------------------------------------------------------
    //! \brief Parse the Json string provided.
    //
    //! \param[in] strJsonData_ A string containing Json objects.
    //----------------------------------------------------------------------------
    void ParseJson(std::string_view strJsonData_);

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
            if (field->type == FIELD_TYPE::ENUM)
            {
                dynamic_cast<EnumField*>(field)->enumDef = GetEnumDefId(dynamic_cast<const EnumField*>(field)->enumId);
            }
            else if (field->type == FIELD_TYPE::FIELD_ARRAY) { MapMessageEnumFields(dynamic_cast<FieldArrayField*>(field)->fields); }
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
                            [iMsgId_](const MessageDefinition& elem_) { return elem_.logID == iMsgId_; });
    }

    std::vector<MessageDefinition>::iterator GetMessageIt(std::string_view strMessage_)
    {
        return std::find_if(vMessageDefinitions.begin(), vMessageDefinitions.end(),
                            [strMessage_](const MessageDefinition& elem_) { return elem_.name == strMessage_; });
    }

    std::vector<EnumDefinition>::iterator GetEnumIt(std::string_view strEnumeration_)
    {
        return std::find_if(vEnumDefinitions.begin(), vEnumDefinitions.end(),
                            [strEnumeration_](const EnumDefinition& elem_) { return elem_.name == strEnumeration_; });
    }
};

} // namespace novatel::edie

#endif
