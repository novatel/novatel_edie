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

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "novatel_edie/common/crc32.hpp"
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

//-----------------------------------------------------------------------
//! \struct EnumDataType
//! \brief Enum Data Type representing contents of UI DB.
//-----------------------------------------------------------------------
struct EnumDataType
{
    uint32_t value{0};
    std::string name;
    std::string description;
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
    std::unordered_map<std::string_view, uint32_t> nameValue;
    std::unordered_map<std::string_view, uint32_t> descriptionValue;
    std::unordered_map<uint32_t, std::string_view> valueName;

    using Ptr = std::shared_ptr<EnumDefinition>;
    using ConstPtr = std::shared_ptr<const EnumDefinition>;
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
    std::string description;
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
    std::optional<int32_t> width;
    std::optional<int32_t> precision;
    bool isString{false};
    bool isCsv{false};
    SimpleDataType dataType;

    BaseField() = default;

    BaseField(std::string name_, const FIELD_TYPE type_, std::string&& sConversion_, const size_t length_, const DATA_TYPE eDataTypeName_)
        : name(std::move(name_)), type(type_)
    {
        SetConversion(std::move(sConversion_));
        dataType.length = static_cast<uint16_t>(length_);
        dataType.name = eDataTypeName_;
    }

    virtual ~BaseField() = default;

    void SetConversion(std::string&& sConversion_)
    {
        conversion = std::move(sConversion_);
        width = {};
        precision = {};

        const char* sConvertString = conversion.c_str();

        if (*sConvertString != '%') { throw std::runtime_error("Encountered an unexpected character in conversion string"); }

        ++sConvertString;

        if (std::isdigit(*sConvertString))
        {
            width = std::stoi(sConvertString);
            sConvertString += std::to_string(*width).length();
        }

        if (*sConvertString == '.') { ++sConvertString; }

        if (std::isdigit(*sConvertString))
        {
            precision = std::stoi(sConvertString);
            sConvertString += std::to_string(*precision).length();
        }

        conversionHash = 0;

        while (std::isalpha(*sConvertString)) { CalculateCharacterCrc32(conversionHash, *sConvertString++); }

        if (*sConvertString != '\0') { throw std::runtime_error("Encountered an unexpected character in conversion string"); }

        isString = type == FIELD_TYPE::STRING || conversionHash == CalculateBlockCrc32("s") || conversionHash == CalculateBlockCrc32("S");
        isCsv = !isString && conversionHash != CalculateBlockCrc32("Z") && conversionHash != CalculateBlockCrc32("P");
    }

    using Ptr = std::shared_ptr<BaseField>;
    using ConstPtr = std::shared_ptr<const BaseField>;
};

//-----------------------------------------------------------------------
//! \struct EnumField
//! \brief Struct containing elements of enum fields in the UI DB.
//-----------------------------------------------------------------------
struct EnumField : BaseField
{
    std::string enumId;
    EnumDefinition::ConstPtr enumDef{nullptr};
    uint32_t length{0};

    using Ptr = std::shared_ptr<EnumField>;
    using ConstPtr = std::shared_ptr<const EnumField>;
};

//-----------------------------------------------------------------------
//! \struct ArrayField
//! \brief Struct containing elements of array fields in the UI DB.
//-----------------------------------------------------------------------
struct ArrayField : BaseField
{
    uint32_t arrayLength{0};

    using Ptr = std::shared_ptr<ArrayField>;
    using ConstPtr = std::shared_ptr<const ArrayField>;
};

//-----------------------------------------------------------------------
//! \struct FieldArrayField
//! \brief Struct containing elements of field array fields in the UI DB.
//-----------------------------------------------------------------------
struct FieldArrayField : BaseField
{
    uint32_t arrayLength{0}, fieldSize{0};
    std::string arrayLengthRef; // TODO: use something other than string
    std::vector<std::shared_ptr<BaseField>> fields;

    using Ptr = std::shared_ptr<FieldArrayField>;
    using ConstPtr = std::shared_ptr<const FieldArrayField>;
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
    std::unordered_map<uint32_t, std::vector<BaseField::Ptr>> fields; // map of crc keys to field definitions
    uint32_t latestMessageCrc{0};

    const std::vector<BaseField::Ptr>& GetMsgDefFromCrc(spdlog::logger& pclLogger_, uint32_t uiMsgDefCrc_) const;

    using Ptr = std::shared_ptr<MessageDefinition>;
    using ConstPtr = std::shared_ptr<const MessageDefinition>;
};

//============================================================================
//! \class MessageDatabase
//! \brief Holds the definitions of NovAtel messages and enums.
//============================================================================
class MessageDatabase
{
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

            for (const auto& item : msgDef->fields) { MapMessageEnumFields(item.second); }
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

            for (const auto& item : msg->fields) { MapMessageEnumFields(item.second); }
        }
    }

  private:
    void MapMessageEnumFields(const std::vector<std::shared_ptr<BaseField>>& vMsgDefFields_)
    {
        for (const auto& field : vMsgDefFields_)
        {
            if (field->type == FIELD_TYPE::ENUM)
            {
                auto* enumField = dynamic_cast<EnumField*>(field.get());
                enumField->enumDef = GetEnumDefId(enumField->enumId);
            }
            else if (field->type == FIELD_TYPE::FIELD_ARRAY)
            {
                auto* fieldArrayField = dynamic_cast<FieldArrayField*>(field.get());
                MapMessageEnumFields(fieldArrayField->fields);
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

} // namespace novatel::edie

#endif
