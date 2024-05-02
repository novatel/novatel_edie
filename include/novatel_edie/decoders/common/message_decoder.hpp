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
// ! \file message_decoder.hpp
// ===============================================================================

#ifndef MESSAGE_DECODER_HPP
#define MESSAGE_DECODER_HPP

#include <string>
#include <utility>
#include <variant>

#include "novatel_edie/common/logger.hpp"
#include "novatel_edie/decoders/common/common.hpp"
#include "novatel_edie/decoders/common/json_reader.hpp"
#include "novatel_edie/decoders/common/message_decoder.hpp"

namespace novatel::edie {

struct FieldContainer;

#define NOVATEL_TYPES bool, int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t, float, double, std::string
#define CONTAINER_TYPES NOVATEL_TYPES, std::vector<FieldContainer>
using FieldValueVariant = std::variant<CONTAINER_TYPES>;

//-----------------------------------------------------------------------
//! \struct FieldContainer
//! \brief A struct to contain different fields from OEM messages.
//-----------------------------------------------------------------------
struct FieldContainer
{
    FieldValueVariant fieldValue;
    BaseField::ConstPtr fieldDef{};

    template <class T> FieldContainer(T tFieldValue_, BaseField::ConstPtr pstFieldDef_) : fieldValue(tFieldValue_), fieldDef(pstFieldDef_) {}

    // [[deprecated("Avoid copying of FieldContainer objects")]]
    // FieldContainer([[maybe_unused]] const FieldContainer& obj) = default;
    // [[deprecated("Avoid copying of FieldContainer objects")]]
    // FieldContainer& operator=([[maybe_unused]] const FieldContainer& obj) = default;
};

//============================================================================
//! \class MessageDecoderBase
//! \brief Class to decode messages.
//============================================================================
class MessageDecoderBase
{
  private:
    static constexpr std::string_view svErrorPrefix = "ERROR:";

    std::shared_ptr<spdlog::logger> pclMyLogger{Logger::RegisterLogger("message_decoder")};

    JsonReader::Ptr pclMyMsgDb{nullptr};

    EnumDefinition::Ptr vMyResponseDefinitions{nullptr};
    EnumDefinition::Ptr vMyCommandDefinitions{nullptr};
    EnumDefinition::Ptr vMyPortAddressDefinitions{nullptr};
    EnumDefinition::Ptr vMyGpsTimeStatusDefinitions{nullptr};

    MessageDefinition::Ptr stMyRespDef{nullptr};

    // Enum util functions
    void InitEnumDefinitions();
    void InitFieldMaps();
    void CreateResponseMsgDefinitions();

  protected:
    std::unordered_map<uint32_t, std::function<void(std::vector<FieldContainer>&, BaseField::ConstPtr, char**, [[maybe_unused]] size_t,
                                                    [[maybe_unused]] JsonReader&)>>
        asciiFieldMap;
    std::unordered_map<uint32_t, std::function<void(std::vector<FieldContainer>&, BaseField::ConstPtr, json, [[maybe_unused]] JsonReader&)>>
        jsonFieldMap;

    [[nodiscard]] STATUS DecodeBinary(const std::vector<BaseField::Ptr>& vMsgDefFields_, unsigned char** ppucLogBuf_,
                                      std::vector<FieldContainer>& vIntermediateFormat_, uint32_t uiMessageLength_) const;
    template <bool Abbreviated>
    [[nodiscard]] STATUS DecodeAscii(const std::vector<BaseField::Ptr>& vMsgDefFields_, char** ppcLogBuf_,
                                     std::vector<FieldContainer>& vIntermediateFormat_) const;
    [[nodiscard]] STATUS DecodeJson(const std::vector<BaseField::Ptr>& vMsgDefFields_, json clJsonFields_,
                                    std::vector<FieldContainer>& vIntermediateFormat_) const;

    static void DecodeBinaryField(BaseField::ConstPtr pstMessageDataType_, unsigned char** ppucLogBuf_,
                                  std::vector<FieldContainer>& vIntermediateFormat_);
    void DecodeAsciiField(BaseField::ConstPtr pstMessageDataType_, char** ppcToken_, size_t tokenLength_,
                          std::vector<FieldContainer>& vIntermediateFormat_) const;
    void DecodeJsonField(BaseField::ConstPtr pstMessageDataType_, const json& clJsonField_, std::vector<FieldContainer>& vIntermediateFormat_) const;

    // -------------------------------------------------------------------------------------------------------
    template <typename T, int R = 10>
    static std::function<void(std::vector<FieldContainer>&, novatel::edie::BaseField::ConstPtr, char**, size_t, JsonReader&)> SimpleAsciiMapEntry()
    {
        return [](std::vector<FieldContainer>& vIntermediate_, BaseField::ConstPtr pstField_, char** ppcToken_,
                  [[maybe_unused]] const size_t tokenLength_, [[maybe_unused]] JsonReader& pclMsgDb_) {
            if constexpr (std::is_same_v<T, int8_t>)
            {
                vIntermediate_.emplace_back(static_cast<T>(strtol(*ppcToken_, nullptr, R)), std::move(pstField_));
            }
            if constexpr (std::is_same_v<T, int16_t>)
            {
                vIntermediate_.emplace_back(static_cast<T>(strtol(*ppcToken_, nullptr, R)), std::move(pstField_));
            }
            if constexpr (std::is_same_v<T, int32_t>)
            {
                vIntermediate_.emplace_back(static_cast<T>(strtol(*ppcToken_, nullptr, R)), std::move(pstField_));
            }
            if constexpr (std::is_same_v<T, int64_t>)
            {
                vIntermediate_.emplace_back(static_cast<T>(strtoll(*ppcToken_, nullptr, R)), std::move(pstField_));
            }
            if constexpr (std::is_same_v<T, uint8_t>)
            {
                vIntermediate_.emplace_back(static_cast<T>(strtoul(*ppcToken_, nullptr, R)), std::move(pstField_));
            }
            if constexpr (std::is_same_v<T, uint16_t>)
            {
                vIntermediate_.emplace_back(static_cast<T>(strtoul(*ppcToken_, nullptr, R)), std::move(pstField_));
            }
            if constexpr (std::is_same_v<T, uint32_t>)
            {
                vIntermediate_.emplace_back(static_cast<T>(strtoul(*ppcToken_, nullptr, R)), std::move(pstField_));
            }
            if constexpr (std::is_same_v<T, uint64_t>)
            {
                vIntermediate_.emplace_back(static_cast<T>(strtoull(*ppcToken_, nullptr, R)), std::move(pstField_));
            }
            if constexpr (std::is_same_v<T, float>) { vIntermediate_.emplace_back(strtof(*ppcToken_, nullptr), std::move(pstField_)); }
            if constexpr (std::is_same_v<T, double>) { vIntermediate_.emplace_back(strtod(*ppcToken_, nullptr), std::move(pstField_)); }
        };
    }

    // -------------------------------------------------------------------------------------------------------
    template <typename T> static std::function<void(std::vector<FieldContainer>&, BaseField::ConstPtr, json, JsonReader&)> SimpleJsonMapEntry()
    {
        return [](std::vector<FieldContainer>& vIntermediate_, BaseField::ConstPtr pstMessageDataType_, json clJsonField_,
                  [[maybe_unused]] JsonReader& pclMsgDb_) { vIntermediate_.emplace_back(clJsonField_.get<T>(), pstMessageDataType_); };
    }

  public:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the MessageDecoderBase class.
    //
    //! \param[in] pclJsonDb_ A pointer to a JsonReader object. Defaults to nullptr.
    //----------------------------------------------------------------------------
    MessageDecoderBase(JsonReader::Ptr pclJsonDb_ = nullptr);

    //----------------------------------------------------------------------------
    //! \brief Load a JsonReader object.
    //
    //! \param[in] pclJsonDb_ A pointer to a JsonReader object.
    //----------------------------------------------------------------------------
    void LoadJsonDb(JsonReader::Ptr pclJsonDb_);

    //----------------------------------------------------------------------------
    //! \brief Get the internal logger.
    //
    //! \return A shared_ptr to the spdlog::logger.
    //----------------------------------------------------------------------------
    std::shared_ptr<spdlog::logger> GetLogger();

    //----------------------------------------------------------------------------
    //! \brief Set the level of detail produced by the internal logger.
    //
    //! \param[in] eLevel_ The logging level to enable.
    //----------------------------------------------------------------------------
    void SetLoggerLevel(spdlog::level::level_enum eLevel_) const;

    //----------------------------------------------------------------------------
    //! \brief Decode a message body from the provided frame.
    //
    //! \param[in] pucMessage_ A pointer to a message body.
    //! \param[out] stInterMessage_ The IntermediateLog to be populated.
    //! \param[in, out] stMetaData_ MetaDataStruct to provide information about
    //! the frame and be fully populated to help describe the decoded log.
    //
    //! \remark Note, that pucMessage_ must not point to the message header,
    //! rather the message body. This can be done by advancing the pointer
    //! of a message frame by stMetaData.uiHeaderLength.
    //! \return An error code describing the result of decoding.
    //!   SUCCESS: The operation was successful.
    //!   NULL_PROVIDED: pucMessage_ is a null pointer.
    //!   NO_DATABASE: No database was ever loaded into this component.
    //!   NO_DEFINITION: No definition for the current log was found in the
    //! provided database.
    //!   FAILURE: Failed to decode a header field.
    //!   UNSUPPORTED: Attempted to decode an unsupported format.
    //!   UNKNOWN: The header format provided is not known.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS Decode(unsigned char* pucMessage_, std::vector<FieldContainer>& stInterMessage_, MetaDataBase& stMetaData_) const;
};

} // namespace novatel::edie

#endif // MESSAGE_DECODER_HPP
