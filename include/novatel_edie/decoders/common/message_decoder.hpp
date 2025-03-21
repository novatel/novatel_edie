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

#include <simdjson.h>

#include "novatel_edie/common/logger.hpp"
#include "novatel_edie/decoders/common/common.hpp"
#include "novatel_edie/decoders/common/message_database.hpp"
#include "novatel_edie/decoders/common/message_decoder.hpp"

namespace novatel::edie {

struct FieldContainer;

#define NOVATEL_TYPES bool, int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t, float, double, std::string_view
#define CONTAINER_TYPES NOVATEL_TYPES, std::vector<FieldContainer>
using FieldValueVariant = std::variant<CONTAINER_TYPES>;

//-----------------------------------------------------------------------
//! \struct FieldContainer
//! \brief A struct to contain different fields from OEM messages.
//-----------------------------------------------------------------------
struct FieldContainer
{
    FieldValueVariant fieldValue;
    BaseField::ConstPtr fieldDef;

    template <class T> FieldContainer(T tFieldValue_, BaseField::ConstPtr pstFieldDef_) : fieldValue(tFieldValue_), fieldDef(pstFieldDef_) {}
};

//============================================================================
//! \class MessageDecoderBase
//! \brief Class to decode messages.
//============================================================================
class MessageDecoderBase
{
  private:
    static constexpr std::string_view svErrorPrefix = "ERROR:";

    std::shared_ptr<spdlog::logger> pclMyLogger{pclLoggerManager->RegisterLogger("message_decoder")};

    MessageDatabase::Ptr pclMyMsgDb{nullptr};

    EnumDefinition::ConstPtr vMyResponseDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyCommandDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyPortAddressDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyGpsTimeStatusDefinitions{nullptr};

    MessageDefinition::Ptr stMyRespDef{nullptr};

    // Enum util functions
    void InitEnumDefinitions();
    void InitFieldMaps();
    void CreateResponseMsgDefinitions();

  protected:
    std::unordered_map<uint32_t, std::function<void(std::vector<FieldContainer>&, BaseField::ConstPtr, const char**, [[maybe_unused]] size_t,
                                                    [[maybe_unused]] MessageDatabase&)>>
        asciiFieldMap;
    std::unordered_map<
        uint32_t, std::function<void(std::vector<FieldContainer>&, BaseField::ConstPtr, simdjson::dom::element, [[maybe_unused]] MessageDatabase&)>>
        jsonFieldMap;

    [[nodiscard]] STATUS DecodeBinary(const std::vector<BaseField::Ptr>& vMsgDefFields_, const unsigned char** ppucLogBuf_,
                                      std::vector<FieldContainer>& vIntermediateFormat_, uint32_t uiMessageLength_) const;
    template <bool Abbreviated>
    [[nodiscard]] STATUS DecodeAscii(const std::vector<BaseField::Ptr>& vMsgDefFields_, const char** ppcLogBuf_,
                                     std::vector<FieldContainer>& vIntermediateFormat_) const;
    [[nodiscard]] STATUS DecodeJson(const std::vector<BaseField::Ptr>& vMsgDefFields_, simdjson::dom::element jsonData,
                                    std::vector<FieldContainer>& vIntermediateFormat_) const;

    static void DecodeBinaryField(BaseField::ConstPtr pstMessageDataType_, const unsigned char** ppucLogBuf_,
                                  std::vector<FieldContainer>& vIntermediateFormat_);
    void DecodeAsciiField(BaseField::ConstPtr pstMessageDataType_, const char** ppcToken_, size_t tokenLength_,
                          std::vector<FieldContainer>& vIntermediateFormat_) const;
    void DecodeJsonField(BaseField::ConstPtr pstMessageDataType_, simdjson::dom::element clJsonField_,
                         std::vector<FieldContainer>& vIntermediateFormat_) const;

    // -------------------------------------------------------------------------------------------------------
    template <typename T, int R = 10>
    static std::function<void(std::vector<FieldContainer>&, BaseField::ConstPtr, const char**, size_t, MessageDatabase&)> SimpleAsciiMapEntry()
    {
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>, "Template argument must be integral or float");

        return [](std::vector<FieldContainer>& vIntermediate_, BaseField::ConstPtr pstField_, const char** ppcToken_,
                  [[maybe_unused]] const size_t tokenLength_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
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
    template <typename T>
    static void PushElement(std::vector<FieldContainer>& vIntermediate_, BaseField::ConstPtr pstMessageDataType_, simdjson::dom::element clJsonField_)
    {
        // Determine the intermediate type to use for simdjson::get
        using IntermediateType =
            std::conditional_t<std::is_same_v<T, bool>, bool,                                                   // Handle bool directly
                               std::conditional_t<std::is_integral_v<T> && sizeof(T) <= sizeof(int64_t),        // For integers <= 64 bits
                                                  std::conditional_t<std::is_signed_v<T>, int64_t, uint64_t>,   // Use int64_t or uint64_t
                                                  std::conditional_t<std::is_floating_point_v<T>, double, void> // Use double for floating-point
                                                  >>;

        static_assert(!std::is_same_v<IntermediateType, void>, "Unsupported type for PushElement");

        IntermediateType intermediateValue;
        if (clJsonField_.get(intermediateValue) == simdjson::SUCCESS)
        {
            T value = static_cast<T>(intermediateValue);
            vIntermediate_.emplace_back(value, pstMessageDataType_);
        }
        else
        {
            // Handle error (e.g., log a warning or throw an exception)
        }
    }

    // -------------------------------------------------------------------------------------------------------
    template <typename T>
    static std::function<void(std::vector<FieldContainer>&, BaseField::ConstPtr, simdjson::dom::element, MessageDatabase&)> SimpleJsonMapEntry()
    {
        return [](std::vector<FieldContainer>& vIntermediate_, BaseField::ConstPtr pstMessageDataType_, simdjson::dom::element clJsonField_,
                  [[maybe_unused]] MessageDatabase& pclMsgDb_) { PushElement<T>(vIntermediate_, pstMessageDataType_, clJsonField_); };
    }

  public:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the MessageDecoderBase class.
    //
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object. Defaults to nullptr.
    //----------------------------------------------------------------------------
    MessageDecoderBase(MessageDatabase::Ptr pclMessageDb_ = nullptr);

    //----------------------------------------------------------------------------
    //! \brief Load a MessageDatabase object.
    //
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object.
    //----------------------------------------------------------------------------
    void LoadJsonDb(MessageDatabase::Ptr pclMessageDb_);

    //----------------------------------------------------------------------------
    //! \brief Get the internal logger.
    //
    //! \return A shared_ptr to the spdlog::logger.
    //----------------------------------------------------------------------------
    std::shared_ptr<spdlog::logger> GetLogger() { return pclMyLogger; }

    //----------------------------------------------------------------------------
    //! \brief Set the level of detail produced by the internal logger.
    //
    //! \param[in] eLevel_ The logging level to enable.
    //----------------------------------------------------------------------------
    void SetLoggerLevel(spdlog::level::level_enum eLevel_) const { pclMyLogger->set_level(eLevel_); }

    // ---------------------------------------------------------------------------
    //! \brief Get the MessageDatabase object.
    //
    //! \return A shared pointer to the MessageDatabase object.
    // ---------------------------------------------------------------------------
    MessageDatabase::ConstPtr MessageDb() const { return std::const_pointer_cast<const MessageDatabase>(pclMyMsgDb); }

    //----------------------------------------------------------------------------
    //! \brief Decode a message payload from the provided frame.
    //
    //! \param[in] pucMessage_ A pointer to a message payload.
    //! \param[out] stInterMessage_ The IntermediateLog to be populated.
    //! \param[in, out] stMetaData_ MetaDataStruct to provide information about
    //! the frame and be fully populated to help describe the decoded log.
    //
    //! \remark Note, that pucMessage_ must not point to the message header,
    //! rather the message payload. This can be done by advancing the pointer
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
    [[nodiscard]] STATUS Decode(const unsigned char* pucMessage_, std::vector<FieldContainer>& stInterMessage_, MetaDataBase& stMetaData_) const;
};

} // namespace novatel::edie

#endif // MESSAGE_DECODER_HPP
