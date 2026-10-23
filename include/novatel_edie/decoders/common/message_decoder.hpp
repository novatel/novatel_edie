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

#pragma once

#include <cassert>
#include <charconv>
#include <cstring>
#include <functional>
#include <iterator>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <simdjson.h>

#include "novatel_edie/common/logger.hpp"
#include "novatel_edie/decoders/common/common.hpp"
#include "novatel_edie/decoders/common/composite_field.hpp"
#include "novatel_edie/decoders/common/message_database.hpp"

namespace novatel::edie {

//============================================================================
//! \class MessageDecoderBase
//! \brief Class to decode messages.
//============================================================================
class MessageDecoderBase
{
  private:
    static constexpr std::string_view svErrorPrefix = "ERROR:";

    std::shared_ptr<spdlog::logger> pclMyLogger{GetBaseLoggerManager()->RegisterLogger("message_decoder")};

    EnumDefinition::ConstPtr vMyResponseDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyCommandDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyPortAddressDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyGpsTimeStatusDefinitions{nullptr};

    std::string sMyExpectedMessageFamily;

    std::function<size_t(const size_t, const uintptr_t, const uintptr_t)> fMyAlignmentFunc = MessageDatabase::NoAlign;

    // Enum util functions
    void InitEnumDefinitions();
    void InitFieldMaps();

  protected:
    MessageDatabase::Ptr pclMyMsgDb{nullptr};

    std::unordered_map<uint32_t, std::function<void(CompositeField&, const BaseField&, const char**, size_t, size_t, bool, MessageDatabase&)>>
        asciiFieldMap;
    std::unordered_map<uint32_t, std::function<void(CompositeField&, const BaseField&, simdjson::dom::element, size_t, bool, MessageDatabase&)>>
        jsonFieldMap;

    [[nodiscard]] STATUS DecodeBinary(const FieldInfo& vMsgDefFields_, const unsigned char** ppucLogBuf_, CompositeField& clCompField_,
                                      uint32_t uiMessageLength_) const;
    template <bool Abbreviated>
    [[nodiscard]] STATUS DecodeAscii(const FieldInfo& vMsgDefFields_, const char** ppcLogBuf_, CompositeField& clCompField_,
                                     const char* pcBufEnd = nullptr) const;
    [[nodiscard]] STATUS DecodeJson(const FieldInfo& vMsgDefFields_, simdjson::dom::element jsonData, CompositeField& clCompField_) const;

    static void DecodeBinaryField(const BaseField& pstMessageDataType_, const unsigned char** ppucLogBuf_, CompositeField& clCompField_,
                                  size_t n = 1);
    void DecodeAsciiField(const BaseField& field_, const char** ppcToken_, size_t tokenLength_, CompositeField& clCompField_,
                          size_t elementIndex_ = 0, bool fixed_ = true) const;
    void DecodeJsonField(const BaseField& field_, simdjson::dom::element clJsonField_, CompositeField& clCompField_, size_t elementIndex_ = 0,
                         bool fixed_ = true) const;

    //----------------------------------------------------------------------------
    //! \brief Add padding after string fields if necessary to maintain alignment.
    //
    //! \param[in] start_ The starting pointer of the binary buffer. Used to
    //! calculate the current offset relative to the beginning of the payload.
    //! \param[in, out] ptr_ A pointer to the buffer pointer to be advanced
    //! if padding is needed.
    //
    //! \return None. The buffer pointer is updated in place.
    //----------------------------------------------------------------------------
    virtual void AddStringFieldPadding([[maybe_unused]] const unsigned char* start, [[maybe_unused]] const unsigned char** ptr) const { return; }

    // -------------------------------------------------------------------------------------------------------
    template <bool Fixed = true, typename T = int32_t, int R = 10>
    static void ParseAndEmplace(CompositeField& clCompField_, const BaseField& field_, const char* token, size_t tokenLength,
                                size_t elementIndex_ = 0)
    {
        T value;
        std::from_chars_result result;

        // As of 6/26/2025 spaces may appear within ascii fields of OUTPUTDATUM and SETALIGNMENTVEL as they use conversion strings with width
        // values. EDIE supports decoding of this data but will never pad fields with spaces during encoding.
        uint32_t offset = 0;
        while ((token[offset] == ' ') && (offset < tokenLength - 1)) { ++offset; }

        if constexpr (std::is_integral_v<T>) { result = std::from_chars(token + offset, token + tokenLength, value, R); }
        else if constexpr (std::is_floating_point_v<T>) { result = std::from_chars(token + offset, token + tokenLength, value); }

        if (result.ec != std::errc()) { throw std::runtime_error("Failed to parse numeric value"); }

        clCompField_.SetArrayElement<Fixed>(field_, elementIndex_, value);
    }

    // -------------------------------------------------------------------------------------------------------
    template <typename T, int R = 10>
    static std::function<void(CompositeField&, const BaseField&, const char**, size_t, size_t, bool, MessageDatabase&)> SimpleAsciiMapEntry()
    {
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>, "Template argument must be integral or float");

        return [](CompositeField& vIntermediate_, const BaseField& pstField_, const char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                  const size_t elementIndex_, const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
            if (fixed_) { ParseAndEmplace<true, T, R>(vIntermediate_, pstField_, *ppcToken_, tokenLength_, elementIndex_); }
            else { ParseAndEmplace<false, T, R>(vIntermediate_, pstField_, *ppcToken_, tokenLength_, elementIndex_); }
        };
    }

    // -------------------------------------------------------------------------------------------------------
    template <typename T>
    static void PushElement(CompositeField& vIntermediate_, const BaseField& field_, simdjson::dom::element clJsonField_, size_t elementIndex_ = 0,
                            bool fixed_ = true)
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
        if (clJsonField_.get(intermediateValue) != simdjson::SUCCESS)
        {
            throw std::runtime_error("Failed to decode JSON field '" + field_.name + "'");
        }
        T value = static_cast<T>(intermediateValue);
        if (fixed_) { vIntermediate_.SetArrayElement<true>(field_, elementIndex_, value); }
        else { vIntermediate_.SetArrayElement<false>(field_, elementIndex_, value); }
    }

    // -------------------------------------------------------------------------------------------------------
    template <typename T>
    static std::function<void(CompositeField&, const BaseField&, simdjson::dom::element, size_t, bool, MessageDatabase&)> SimpleJsonMapEntry()
    {
        return [](CompositeField& vIntermediate_, const BaseField& pstMessageDataType_, simdjson::dom::element clJsonField_,
                  const size_t elementIndex_, const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
            PushElement<T>(vIntermediate_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_);
        };
    }

    // -------------------------------------------------------------------------------------------------------
    uint32_t GetArrayLength(const unsigned char* pucTempStart, const unsigned char** ppucLogBuf_, const ArrayField& arrayDef,
                            const CompositeField& clCompField_) const
    {
        if (arrayDef.arrayLengthRef.empty())
        {
            const std::size_t lenBytes = arrayDef.arrayLengthFieldSize;
            if (!(lenBytes == 1 || lenBytes == 2 || lenBytes == 4))
                throw std::runtime_error("GetArrayLength: Unsupported length size; must be 1,2 or 4");

            *ppucLogBuf_ += fMyAlignmentFunc(lenBytes, reinterpret_cast<uintptr_t>(pucTempStart), reinterpret_cast<uintptr_t>(*ppucLogBuf_));

            uint32_t uiArrayLength = 0;
            for (std::size_t i = 0; i < lenBytes; ++i) { uiArrayLength |= static_cast<uint32_t>((*ppucLogBuf_)[i]) << (8 * i); }

            *ppucLogBuf_ += lenBytes;
            return uiArrayLength;
        }

        // Traverse the decoded fields to find the arrayLengthRef field by its name.
        if (auto arrLengthRefDef = clCompField_.GetFieldInfo()->GetFieldDefByName(arrayDef.arrayLengthRef))
        {
            const auto arraySize = std::visit(
                [](const auto& value) -> std::optional<uint32_t> {
                    using T = std::decay_t<decltype(value)>;
                    if constexpr (std::is_integral_v<T>)
                    {
                        if constexpr (std::is_signed_v<T>)
                        {
                            if (value < 0) { return std::nullopt; }
                        }
                        if constexpr (sizeof(T) > sizeof(uint32_t))
                        {
                            if (value > static_cast<T>(std::numeric_limits<uint32_t>::max())) { return std::nullopt; }
                        }
                        return static_cast<uint32_t>(value);
                    }
                    return std::nullopt;
                },
                clCompField_.GetFieldValueVariant(*arrLengthRefDef));

            if (arraySize) { return *arraySize; }
        }

        throw std::runtime_error("GetArrayLength(): No matching field found for arrayLengthRef");
    }

    //----------------------------------------------------------------------------
    //! \brief Get the message definition corresponding to the given metadata.
    //
    //! \param[in] stMetaData_ The metadata containing the message ID to look up.
    //! \return A pointer to the message definition.
    //----------------------------------------------------------------------------
    virtual MessageDefinition::ConstPtr GetMessageDefinition(MetaDataBase& stMetaData_) const
    {
        if (pclMyMsgDb == nullptr) { throw std::runtime_error("GetMessageDefinition(): no message database loaded"); }

        auto msgDef = pclMyMsgDb->GetMsgDef(stMetaData_.usMessageId);
        return msgDef;
    }

  public:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the MessageDecoderBase class.
    //
    //! \param[in] expectedMessageFamily_ The expected message family for the encoder.
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object. Defaults to nullptr.
    //----------------------------------------------------------------------------
    MessageDecoderBase(std::string expectedMessageFamily_, MessageDatabase::Ptr pclMessageDb_ = nullptr)
        : sMyExpectedMessageFamily(std::move(expectedMessageFamily_)), pclMyMsgDb(std::move(pclMessageDb_))
    {
        const auto it = MessageDatabase::GetAlignmentFunctions().find(sMyExpectedMessageFamily);
        if (it != MessageDatabase::GetAlignmentFunctions().end()) { fMyAlignmentFunc = it->second; }
        InitFieldMaps();
        if (pclMyMsgDb != nullptr) { LoadJsonDb(std::move(pclMyMsgDb)); }
    }

    virtual ~MessageDecoderBase() = default;

    //----------------------------------------------------------------------------
    //! \brief Load a MessageDatabase object.
    //
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object.
    //----------------------------------------------------------------------------
    virtual void LoadJsonDb(MessageDatabase::Ptr pclMessageDb_);

    //----------------------------------------------------------------------------
    //! \brief Get the internal logger.
    //
    //! \return A shared_ptr to the spdlog::logger.
    //----------------------------------------------------------------------------
    std::shared_ptr<spdlog::logger> GetLogger() const { return pclMyLogger; }

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
    //! \param[out] stInterMessage_ The CompositeField to be populated.
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
    [[nodiscard]] STATUS Decode(const unsigned char* pucMessage_, CompositeField& stInterMessage_, MetaDataBase& stMetaData_) const;
};

} // namespace novatel::edie
