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
//! \file jsonreader.hpp
//! \brief Class to read a JSON UI DB.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef JSONREADER_HPP
#define JSONREADER_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include <filesystem>
#include <fstream>
#include <iostream>
#include <logger/logger.hpp>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

#include "common.hpp"

using nlohmann::json;

namespace novatel::edie {

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
    char acWhatString[256];

  public:
    JsonReaderFailure(const char* func_, const char* file_, int32_t line_, const std::filesystem::path& json_file_, const char* failure_)
        : func(func_), file(file_), line(line_), clFilePath(json_file_), failure(failure_), acWhatString{}
    {
    }

    const char* what() const throw() override
    {
        sprintf(const_cast<char*>(acWhatString), "In file \"%s\" : %s() (Line %d)\n\t\"%s: %s.\"", file, func, line,
                clFilePath.generic_string().c_str(), failure);
        return acWhatString;
    }
};

//-----------------------------------------------------------------------
//! \enum CONVERSION_STRING
//! \brief Conversion string represented as an enum.
//-----------------------------------------------------------------------
enum class CONVERSION_STRING
{
    // Signed & Unsigned Integers
    d,
    ld,
    hd,
    lld,
    u,
    lu,
    hu,
    llu,
    // Single Chars/Bytes & Hexadecimal
    c,
    uc,
    Z,
    B,
    UB,
    XB,
    x,
    X,
    lx,
    ucb,
    // Floating Point & Scientific Notation
    f,
    lf,
    k,
    lk,
    e,
    le,
    g,
    lg,
    // Strings & Bytestream
    P,
    s,
    S,
    // NovAtel types
    id, // SATELLITEID
    R,  // RXCONFIG
    m,  // MessageName
    T,  // GPSTime value (<int value>/1000.0)
    UNKNOWN
};

//!< Mapping from String to data type enums.
static std::unordered_map<std::string, CONVERSION_STRING> const ConversionStringEnumLookup = {
    {"%d", CONVERSION_STRING::d},   {"%ld", CONVERSION_STRING::ld},   {"%hd", CONVERSION_STRING::hd}, {"%lld", CONVERSION_STRING::lld},
    {"%u", CONVERSION_STRING::u},   {"%lu", CONVERSION_STRING::lu},   {"%hu", CONVERSION_STRING::hu}, {"%llu", CONVERSION_STRING::llu},
    {"%c", CONVERSION_STRING::c},   {"%uc", CONVERSION_STRING::uc},   {"%B", CONVERSION_STRING::B},   {"%UB", CONVERSION_STRING::UB},
    {"%XB", CONVERSION_STRING::XB}, {"%Z", CONVERSION_STRING::Z},     {"%x", CONVERSION_STRING::x},   {"%X", CONVERSION_STRING::X},
    {"%lx", CONVERSION_STRING::lx}, {"%ucb", CONVERSION_STRING::ucb}, {"%f", CONVERSION_STRING::f},   {"%lf", CONVERSION_STRING::lf},
    {"%k", CONVERSION_STRING::k},   {"%lk", CONVERSION_STRING::lk},   {"%e", CONVERSION_STRING::e},   {"%le", CONVERSION_STRING::le},
    {"%g", CONVERSION_STRING::g},   {"%lg", CONVERSION_STRING::lg},   {"%P", CONVERSION_STRING::P},   {"%s", CONVERSION_STRING::s},
    {"%S", CONVERSION_STRING::S},   {"%id", CONVERSION_STRING::id},   {"%R", CONVERSION_STRING::R},   {"%m", CONVERSION_STRING::m},
    {"%T", CONVERSION_STRING::T}};

//-----------------------------------------------------------------------
//! \enum DATA_TYPE
//! \brief Data type name string represented as an enum.
//-----------------------------------------------------------------------
enum class DATA_TYPE
{
    BOOL,
    HEXBYTE,
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
    EMBEDDED_HEADER,
    EMBEDDED_BODY,
    SATELLITEID,
    UNKNOWN
};

//!< returns the size associated with a datatype
constexpr size_t DataTypeSize(DATA_TYPE eType)
{
    return eType == DATA_TYPE::BOOL        ? sizeof(int32_t)
           : eType == DATA_TYPE::HEXBYTE   ? sizeof(uint8_t)
           : eType == DATA_TYPE::CHAR      ? sizeof(int8_t)
           : eType == DATA_TYPE::UCHAR     ? sizeof(uint8_t)
           : eType == DATA_TYPE::SHORT     ? sizeof(int16_t)
           : eType == DATA_TYPE::USHORT    ? sizeof(uint16_t)
           : eType == DATA_TYPE::INT       ? sizeof(int32_t)
           : eType == DATA_TYPE::UINT      ? sizeof(uint32_t)
           : eType == DATA_TYPE::LONG      ? sizeof(int32_t)
           : eType == DATA_TYPE::ULONG     ? sizeof(uint32_t)
           : eType == DATA_TYPE::LONGLONG  ? sizeof(int64_t)
           : eType == DATA_TYPE::ULONGLONG ? sizeof(uint64_t)
           : eType == DATA_TYPE::FLOAT     ? sizeof(float)
           : eType == DATA_TYPE::DOUBLE    ? sizeof(double)
                                           : 0;
}

//!< returns conversion string associated with a datatype
constexpr CONVERSION_STRING DataTypeConversion(DATA_TYPE eType)
{
    return eType == DATA_TYPE::BOOL        ? CONVERSION_STRING::d
           : eType == DATA_TYPE::HEXBYTE   ? CONVERSION_STRING::XB
           : eType == DATA_TYPE::CHAR      ? CONVERSION_STRING::c
           : eType == DATA_TYPE::UCHAR     ? CONVERSION_STRING::uc
           : eType == DATA_TYPE::SHORT     ? CONVERSION_STRING::hd
           : eType == DATA_TYPE::USHORT    ? CONVERSION_STRING::hu
           : eType == DATA_TYPE::INT       ? CONVERSION_STRING::d
           : eType == DATA_TYPE::UINT      ? CONVERSION_STRING::u
           : eType == DATA_TYPE::LONG      ? CONVERSION_STRING::ld
           : eType == DATA_TYPE::ULONG     ? CONVERSION_STRING::lu
           : eType == DATA_TYPE::LONGLONG  ? CONVERSION_STRING::lld
           : eType == DATA_TYPE::ULONGLONG ? CONVERSION_STRING::llu
           : eType == DATA_TYPE::FLOAT     ? CONVERSION_STRING::f
           : eType == DATA_TYPE::DOUBLE    ? CONVERSION_STRING::lf
                                           : CONVERSION_STRING::UNKNOWN;
}

//!< Mapping from String to data type enums.
static std::unordered_map<std::string, DATA_TYPE> const DataTypeEnumLookup = {
    {"BOOL", DATA_TYPE::BOOL},   {"HEXBYTE", DATA_TYPE::HEXBYTE}, {"CHAR", DATA_TYPE::CHAR},         {"UCHAR", DATA_TYPE::UCHAR},
    {"SHORT", DATA_TYPE::SHORT}, {"USHORT", DATA_TYPE::USHORT},   {"INT", DATA_TYPE::INT},           {"UINT", DATA_TYPE::UINT},
    {"LONG", DATA_TYPE::LONG},   {"ULONG", DATA_TYPE::ULONG},     {"LONGLONG", DATA_TYPE::LONGLONG}, {"ULONGLONG", DATA_TYPE::ULONGLONG},
    {"FLOAT", DATA_TYPE::FLOAT}, {"DOUBLE", DATA_TYPE::DOUBLE},   {"UNKNOWN", DATA_TYPE::UNKNOWN}};

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
static std::unordered_map<std::string, FIELD_TYPE> const FieldTypeEnumLookup = {{"SIMPLE", FIELD_TYPE::SIMPLE},
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
    std::vector<novatel::edie::EnumDataType> enumerators{};

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
    std::unordered_map<int32_t, novatel::edie::EnumDataType> enums;
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
    CONVERSION_STRING conversionStripped{CONVERSION_STRING::UNKNOWN};
    int32_t conversionBeforePoint{0};
    int32_t conversionAfterPoint{0};
    SimpleDataType dataType;

    BaseField() = default;

    BaseField(std::string name_, FIELD_TYPE type_, CONVERSION_STRING conversionStripped_, size_t length_, DATA_TYPE eDataTypeName_)
        : name(std::move(name_)), type(type_), conversionStripped(conversionStripped_)
    {
        dataType.length = static_cast<uint16_t>(length_);
        dataType.name = eDataTypeName_;
    }

    virtual ~BaseField() = default;

    virtual BaseField* clone() { return new novatel::edie::BaseField(*this); }

    void setConversion(const std::string& sConversion)
    {
        conversion = sConversion;
        std::string strStrippedConversionString;
        parseConversion(strStrippedConversionString, conversionBeforePoint, conversionAfterPoint);
        const auto itrConversionStringMapping = ConversionStringEnumLookup.find(strStrippedConversionString);
        conversionStripped =
            itrConversionStringMapping != ConversionStringEnumLookup.end() ? itrConversionStringMapping->second : CONVERSION_STRING::UNKNOWN;
    }

    void parseConversion(std::string& strStrippedConversionString_, int32_t& iBeforePoint_, int32_t& AfterPoint_) const
    {
        bool bIsBeforePoint = true;
        const char* sConvertString = conversion.c_str();
        int32_t iBytesRead;

        while (*sConvertString)
        {
            // "0x" could be a prefix on a hex field (0x%...) or a valid conversion string (%0x).
            // Prevent these two cases from occurring at the same time.
            if ((0 == memcmp(sConvertString, "0x", 2)) && (0 != strcmp(sConvertString, "0x"))) { sConvertString += 2; }

            // If the value "10" or greater is found from the conversion string, two bytes would
            // need to be consumed from the string to move past that value.  Otherwise only one byte
            // is necessary to consume.
            if ((*sConvertString >= '0') && (*sConvertString <= '9'))
            {
                if (bIsBeforePoint) // before point
                {
                    sscanf(sConvertString, "%d.", &iBeforePoint_);
                    if (iBeforePoint_ > 9)
                        iBytesRead = 2;
                    else
                        iBytesRead = 1;
                    sConvertString = sConvertString + iBytesRead; // Skip the numerals
                }
                else
                {
                    sscanf(sConvertString, "%d", &AfterPoint_);
                    if (AfterPoint_ > 9)
                        iBytesRead = 2;
                    else
                        iBytesRead = 1;
                    sConvertString = sConvertString + iBytesRead; // Skip the numerals
                }
            }

            if (std::isalpha(*sConvertString) || *sConvertString == '%')
            {
                strStrippedConversionString_.push_back(sConvertString[0]);
                sConvertString++;
            }

            if (*sConvertString == '.')
            {
                bIsBeforePoint = false; // Found the decimal
                sConvertString++;
            }
        }
    }
};

//-----------------------------------------------------------------------
//! \struct EnumField
//! \brief Struct containing elements of enum fields in the UI DB.
//-----------------------------------------------------------------------
struct EnumField : novatel::edie::BaseField
{
    std::string enumID;
    novatel::edie::EnumDefinition* enumDef{nullptr};
    uint32_t length{0};

    EnumField() = default;

    ~EnumField() = default;

    EnumField* clone() override { return new novatel::edie::EnumField(*this); }
};

//-----------------------------------------------------------------------
//! \struct ArrayField
//! \brief Struct containing elements of array fields in the UI DB.
//-----------------------------------------------------------------------
struct ArrayField : novatel::edie::BaseField
{
    uint32_t arrayLength{0};

    ArrayField() = default;

    ~ArrayField() = default;

    ArrayField* clone() override { return new novatel::edie::ArrayField(*this); }
};

//-----------------------------------------------------------------------
//! \struct FieldArrayField
//! \brief Struct containing elements of field array fields in the UI DB.
//-----------------------------------------------------------------------
struct FieldArrayField : novatel::edie::BaseField
{
    uint32_t arrayLength{0}, fieldSize{0};
    std::vector<novatel::edie::BaseField*> fields;

    FieldArrayField() = default;

    ~FieldArrayField()
    {
        for (const auto& field : fields) { delete field; }
    }

    FieldArrayField(const FieldArrayField& that) : BaseField(that)
    {
        for (const auto& field : that.fields) { fields.emplace_back(field->clone()); }

        arrayLength = that.arrayLength;
        fieldSize = that.fieldSize;
    }

    FieldArrayField& operator=(FieldArrayField that)
    {
        if (this != &that)
        {
            BaseField::operator=(that);

            fields.clear();
            for (const auto& field : that.fields) { fields.emplace_back(field->clone()); }

            arrayLength = that.arrayLength;
            fieldSize = that.fieldSize;
        }

        return *this;
    }

    FieldArrayField* clone() override { return new FieldArrayField(*this); }
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
    std::map<uint32_t, std::vector<novatel::edie::BaseField*>> fields; // map of crc keys to field defs
    uint32_t latestMessageCrc{0};

    MessageDefinition() = default;
    MessageDefinition(const MessageDefinition& that)
    {
        for (const auto& fielddefs : that.fields)
        {
            uint32_t key = fielddefs.first;
            // Ensure a 0-length vector exists for this key in the case the message has no fields.
            fields[key] = std::vector<novatel::edie::BaseField*>();
            for (const auto& field : fielddefs.second) { fields[key].emplace_back(field->clone()); }
        }

        _id = that._id;
        logID = that.logID;
        name = that.name;
        description = that.description;
        latestMessageCrc = that.latestMessageCrc;
    }

    ~MessageDefinition()
    {
        for (auto field : fields)
        {
            for (auto f : field.second) { delete f; }
        }
    }

    MessageDefinition& operator=(MessageDefinition that)
    {
        if (this != &that)
        {
            fields.clear();
            for (const auto& fielddefs : that.fields)
            {
                uint32_t key = fielddefs.first;
                // Ensure a 0-length vector exists for this key in the case the message has no
                // fields.
                fields[key] = std::vector<novatel::edie::BaseField*>();
                for (const auto& field : fielddefs.second) { fields[key].emplace_back(field->clone()); }
            }

            _id = that._id;
            logID = that.logID;
            name = that.name;
            description = that.description;
            latestMessageCrc = that.latestMessageCrc;
        }

        return *this;
    }

    std::vector<BaseField*> const* GetMsgDefFromCRC(std::shared_ptr<spdlog::logger> pclLogger_, uint32_t& uiMsgDefCRC_) const;
};

// Forward declaration of from_json
void from_json(const json& j, novatel::edie::EnumDataType& f);
void from_json(const json& j, novatel::edie::BaseDataType& f);
void from_json(const json& j, novatel::edie::SimpleDataType& f);
void from_json(const json& j, novatel::edie::BaseField& f);
void from_json(const json& j, novatel::edie::EnumField& f);
void from_json(const json& j, novatel::edie::ArrayField& fd);
void from_json(const json& j, novatel::edie::FieldArrayField& fd);
void from_json(const json& j, MessageDefinition& md);
void from_json(const json& j, EnumDefinition& ed);

// Forward declaration of parse_fields and parse_enumerators
uint32_t parse_fields(const json& j, std::vector<novatel::edie::BaseField*>& vFields);
void parse_enumerators(const json& j, std::vector<novatel::edie::EnumDataType>& vEnumerators);

} // namespace novatel::edie

//============================================================================
//! \class JsonReader
//! \brief Responsible for translating the Json representation of the
//! NovAtel UI DB.
//============================================================================
class JsonReader
{
    std::vector<novatel::edie::MessageDefinition> vMessageDefinitions;
    std::vector<novatel::edie::EnumDefinition> vEnumDefinitions;
    std::unordered_map<std::string, novatel::edie::MessageDefinition*> mMessageName;
    std::unordered_map<int32_t, novatel::edie::MessageDefinition*> mMessageID;
    std::unordered_map<std::string, novatel::edie::EnumDefinition*> mEnumName;
    std::unordered_map<std::string, novatel::edie::EnumDefinition*> mEnumID;

  public:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the JsonReader class.
    //----------------------------------------------------------------------------
    JsonReader() = default;

    //----------------------------------------------------------------------------
    //! \brief A copy constructor for the JsonReader class.
    //
    //! \param [in] that The JsonReader object to copy.
    //----------------------------------------------------------------------------
    JsonReader(const JsonReader& that)
    {
        // TODO Verify it's calling the copy constructor for the messages
        vEnumDefinitions = that.vEnumDefinitions;
        vMessageDefinitions = that.vMessageDefinitions;
        GenerateMappings();
    }

    //----------------------------------------------------------------------------
    //! \brief Overloaded assignment operator for the JsonReader class.
    //
    //! \param [in] that The JsonReader object to assign.
    //----------------------------------------------------------------------------
    JsonReader& operator=(JsonReader that)
    {
        if (this != &that)
        {
            vEnumDefinitions = that.vEnumDefinitions;
            vMessageDefinitions = that.vMessageDefinitions;
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
    //! \param [in] filePath_ The filepath to the Json file.
    //----------------------------------------------------------------------------
    template <typename T> void LoadFile(T filePath_);

    //----------------------------------------------------------------------------
    //! \brief Append a message Json DB from the provided filepath.
    //
    //! \param [in] filePath_ The filepath to the Json file.
    //----------------------------------------------------------------------------
    template <typename T> void AppendMessages(T filePath_);

    //----------------------------------------------------------------------------
    //! \brief Append an enumeration Json DB from the provided filepath.
    //
    //! \param [in] filePath_ The filepath to the Json file.
    //----------------------------------------------------------------------------
    template <typename T> void AppendEnumerations(T filePath_);

    //----------------------------------------------------------------------------
    //! \brief Append an message Json DB from the provided filepath.
    //
    //! \param [in] iMsgId_ The message Id
    //----------------------------------------------------------------------------
    void RemoveMessage(uint32_t iMsgId_, bool bGenerateMappings_ = true);

    //----------------------------------------------------------------------------
    //! \brief Append an enumeration Json DB from the provided filepath.
    //
    //! \param [in] strEnumeration_ The enumeration name
    //----------------------------------------------------------------------------
    void RemoveEnumeration(std::string strEnumeration_, bool bGenerateMappings_);

    //----------------------------------------------------------------------------
    //! \brief Parse the Json string provided.
    //
    //! \param [in] strJsonData_ A string containing Json objects.
    //----------------------------------------------------------------------------
    void ParseJson(const std::string& strJsonData_);

    //----------------------------------------------------------------------------
    //! \brief Get a UI DB message definition for the provided message name.
    //
    //! \param [in] strMsgName_ A string containing the message name.
    //----------------------------------------------------------------------------
    const novatel::edie::MessageDefinition* GetMsgDef(const std::string& strMsgName_) const;

    //----------------------------------------------------------------------------
    //! \brief Get a UI DB message definition for the provided message ID.
    //
    //! \param [in] iMsgID_ The message ID.
    //----------------------------------------------------------------------------
    const novatel::edie::MessageDefinition* GetMsgDef(int32_t iMsgID_) const;

    //----------------------------------------------------------------------------
    //! \brief Convert a message name string to an message ID number
    //
    //! \param [in] sMsgName_ The message name string
    //----------------------------------------------------------------------------
    uint32_t MsgNameToMsgId(std::string sMsgName_) const;

    //----------------------------------------------------------------------------
    //! \brief Convert a message ID number to a message name string
    //
    //! \param [in] uiMessageID_ The message ID number
    //----------------------------------------------------------------------------
    std::string MsgIdToMsgName(const uint32_t uiMessageID_) const;

    //----------------------------------------------------------------------------
    //! \brief Get a UI DB enum definition for the provided enum ID.
    //
    //! \param [in] sEnumNameOrID_ The enum ID.
    //----------------------------------------------------------------------------
    novatel::edie::EnumDefinition* GetEnumDefID(const std::string& sEnumID) const
    {
        auto it = mEnumID.find(sEnumID);
        return it != mEnumID.end() ? it->second : nullptr;
    }

    //----------------------------------------------------------------------------
    //! \brief Get a UI DB enum definition for the provided enum name.
    //
    //! \param [in] sEnumNameOrID_ The enum name.
    //----------------------------------------------------------------------------
    novatel::edie::EnumDefinition* GetEnumDefName(const std::string& sEnumName) const
    {
        auto it = mEnumName.find(sEnumName);
        return it != mEnumName.end() ? it->second : nullptr;
    }

  private:
    void GenerateMappings()
    {
        for (novatel::edie::EnumDefinition& enm : vEnumDefinitions)
        {
            mEnumName[enm.name] = &enm;
            mEnumID[enm._id] = &enm;
        }

        for (novatel::edie::MessageDefinition& msg : vMessageDefinitions)
        {
            mMessageName[msg.name] = &msg;
            mMessageID[msg.logID] = &msg;

            for (const auto& field : msg.fields) { MapMessageEnumFields(field.second); }
        }
    }

    void MapMessageEnumFields(const std::vector<novatel::edie::BaseField*>& vMsgDefFields_)
    {
        for (const auto& field : vMsgDefFields_)
        {
            if (field->type == novatel::edie::FIELD_TYPE::ENUM)
            {
                dynamic_cast<novatel::edie::EnumField*>(field)->enumDef = GetEnumDefID(dynamic_cast<const novatel::edie::EnumField*>(field)->enumID);
            }
            else if (field->type == novatel::edie::FIELD_TYPE::FIELD_ARRAY)
            {
                MapMessageEnumFields(dynamic_cast<novatel::edie::FieldArrayField*>(field)->fields);
            }
        }
    }

    void RemoveMessageMapping(novatel::edie::MessageDefinition& msg)
    {
        // Check string against name map
        auto itName = mMessageName.find(msg.name);
        if (itName != mMessageName.end()) mMessageName.erase(itName);

        auto itId = mMessageID.find(msg.logID);
        if (itId != mMessageID.end()) mMessageID.erase(itId);
    }

    void RemoveEnumerationMapping(novatel::edie::EnumDefinition& enm)
    {
        // Check string against name map
        auto itName = mEnumName.find(enm.name);
        if (itName != mEnumName.end()) mEnumName.erase(itName);

        auto itId = mEnumID.find(enm._id);
        if (itId != mEnumID.end()) mEnumID.erase(itId);
    }

    std::vector<novatel::edie::MessageDefinition>::iterator GetMessageIt(uint32_t iMsgId_)
    {
        return find_if(vMessageDefinitions.begin(), vMessageDefinitions.end(),
                       [iMsgId_](novatel::edie::MessageDefinition elem) { return (elem.logID == iMsgId_); });
    }

    std::vector<novatel::edie::MessageDefinition>::iterator GetMessageIt(const std::string& strMessage_)
    {
        return find_if(vMessageDefinitions.begin(), vMessageDefinitions.end(),
                       [strMessage_](novatel::edie::MessageDefinition elem) { return (elem.name == strMessage_); });
    }

    std::vector<novatel::edie::EnumDefinition>::iterator GetEnumIt(const std::string& strEnumeration_)
    {
        return find_if(vEnumDefinitions.begin(), vEnumDefinitions.end(),
                       [strEnumeration_](novatel::edie::EnumDefinition elem) { return (elem.name == strEnumeration_); });
    }
};

#endif
