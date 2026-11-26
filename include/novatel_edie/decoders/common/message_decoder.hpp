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
#include <string_view>
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
    static constexpr char ASCII_END_DELIM = '*';
    static constexpr char ASCII_TOKEN_DELIM = ',';
    static constexpr char ABBREV_ASCII_END_DELIM = '\r';
    static constexpr char ABBREV_ASCII_TOKEN_DELIM = ' ';

    template <bool Abbreviated> static inline size_t FindFirstAsciiTokenDelimiter(std::string_view sMessage_)
    {
        constexpr char tokenDelimiter = Abbreviated ? ABBREV_ASCII_TOKEN_DELIM : ASCII_TOKEN_DELIM;
        constexpr char endDelimiter = Abbreviated ? ABBREV_ASCII_END_DELIM : ASCII_END_DELIM;

        size_t i = 0;
        for (; i < sMessage_.size(); i++)
        {
            if (sMessage_[i] == tokenDelimiter || sMessage_[i] == endDelimiter) { return i; }
        }
        return i;
    }

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

    // The token passed to each entry is a view into the message and is not NUL-terminated.
    std::unordered_map<uint32_t, std::function<void(CompositeField&, const BaseField&, std::string_view, size_t, MessageDatabase&)>> asciiFieldMap;
    std::unordered_map<uint32_t, std::function<void(CompositeField&, const BaseField&, simdjson::dom::element, size_t, MessageDatabase&)>>
        jsonFieldMap;

    [[nodiscard]] STATUS DecodeBinary(const FieldInfo& vMsgDefFields_, const unsigned char** ppucLogBuf_, CompositeField& clCompField_,
                                      uint32_t uiMessageLength_) const;
    template <bool Abbreviated>
    [[nodiscard]] STATUS DecodeAscii(const FieldInfo& vMsgDefFields_, std::string_view& sMessage_, CompositeField& clCompField_) const;
    [[nodiscard]] STATUS DecodeJson(const FieldInfo& vMsgDefFields_, simdjson::dom::element jsonData, CompositeField& clCompField_) const;

    static void DecodeBinaryField(const BaseField& pstMessageDataType_, const unsigned char** ppucLogBuf_, CompositeField& clCompField_,
                                  size_t n = 1);
    void DecodeAsciiField(const BaseField& field_, std::string_view token_, CompositeField& clCompField_, size_t elementIndex_ = 0) const;
    void DecodeJsonField(const BaseField& field_, simdjson::dom::element clJsonField_, CompositeField& clCompField_, size_t elementIndex_ = 0) const;

    //----------------------------------------------------------------------------
    //! \brief Add padding after string fields if necessary to maintain alignment.
    //
    //! \param[in] start_ The starting pointer of the binary buffer. Used to
    //! calculate the current offset relative to the beginning of the payload.
    //! \param[in, out] ptr_ A pointer to the buffer pointer to be advanced
    //! if padding is needed.
    //! \param[in] bytesLeft Remaining bytes available for padding.
    //
    //! \return True if the padding fits and the buffer pointer was updated.
    //----------------------------------------------------------------------------
    virtual bool AddStringFieldPadding([[maybe_unused]] const unsigned char* start, [[maybe_unused]] const unsigned char** ptr,
                                       [[maybe_unused]] size_t bytesLeft) const
    {
        return true;
    }

    // -------------------------------------------------------------------------------------------------------
    template <typename T = int32_t, int R = 10>
    static void ParseAndEmplace(CompositeField& clCompField_, const BaseField& field_, std::string_view token, size_t elementIndex_ = 0)
    {
        // As of 6/26/2025 spaces may appear within ascii fields of OUTPUTDATUM and SETALIGNMENTVEL as they use conversion strings with width
        // values. EDIE supports decoding of this data but will never pad fields with spaces during encoding.
        const auto offset = token.find_first_not_of(' ');
        if (offset == std::string_view::npos) { throw std::runtime_error("Empty numeric value"); }
        token.remove_prefix(offset);

        T value;
        if (!FromCharsExact<R>(token, value)) { throw std::runtime_error("Failed to parse numeric value"); }

        clCompField_.SetArrayElement(field_, elementIndex_, value);
    }

    // -------------------------------------------------------------------------------------------------------
    template <typename T, int R = 10>
    static std::function<void(CompositeField&, const BaseField&, std::string_view, size_t, MessageDatabase&)> SimpleAsciiMapEntry()
    {
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>, "Template argument must be integral or float");

        return [](CompositeField& vIntermediate_, const BaseField& pstField_, std::string_view token_, const size_t elementIndex_,
                  [[maybe_unused]] MessageDatabase& pclMsgDb_) { ParseAndEmplace<T, R>(vIntermediate_, pstField_, token_, elementIndex_); };
    }

    // -------------------------------------------------------------------------------------------------------
    template <typename T>
    static void PushElement(CompositeField& vIntermediate_, const BaseField& field_, simdjson::dom::element clJsonField_, size_t elementIndex_ = 0)
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
        vIntermediate_.SetArrayElement(field_, elementIndex_, value);
    }

    // -------------------------------------------------------------------------------------------------------
    template <typename T>
    static std::function<void(CompositeField&, const BaseField&, simdjson::dom::element, size_t, MessageDatabase&)> SimpleJsonMapEntry()
    {
        return
            [](CompositeField& vIntermediate_, const BaseField& pstMessageDataType_, simdjson::dom::element clJsonField_, const size_t elementIndex_,
               [[maybe_unused]] MessageDatabase& pclMsgDb_) { PushElement<T>(vIntermediate_, pstMessageDataType_, clJsonField_, elementIndex_); };
    }

    // -------------------------------------------------------------------------------------------------------
    // Returns {length field size, array length}, or std::nullopt if a referenced length field holds a negative or out-of-range value.
    std::optional<std::pair<std::size_t, uint32_t>> GetArrayLength(const unsigned char* pucTempStart, const unsigned char** ppucLogBuf_,
                                                                   const ArrayField& arrayDef, const CompositeField& clCompField_) const
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
            return std::pair{lenBytes, uiArrayLength};
        }

        // Traverse the decoded fields to find the arrayLengthRef field by its name.
        if (auto arrLengthRefDef = clCompField_.GetFieldInfo()->GetFieldDefByName(arrayDef.arrayLengthRef))
        {
            size_t arrayLengthFieldSize = 0;
            std::optional<uint32_t> arraySize = std::nullopt;
            SimpleTypeVisitor(*arrLengthRefDef, [&](auto value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_integral_v<T>)
                {
                    arrayLengthFieldSize = sizeof(T);
                    // Reject negative or out-of-range lengths rather than letting them wrap to huge array sizes
                    const T length = clCompField_.GetFieldValue<T>(*arrLengthRefDef);
                    if constexpr (std::is_signed_v<T>)
                    {
                        if (length < 0) { return; }
                    }
                    if (static_cast<uint64_t>(length) <= std::numeric_limits<uint32_t>::max()) { arraySize = static_cast<uint32_t>(length); }
                }
            });

            if (arrayLengthFieldSize != 0) { return arraySize ? std::make_optional(std::pair{arrayLengthFieldSize, *arraySize}) : std::nullopt; }
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
    //! \brief Decode a message body into a CompositeField.
    //
    //! \param[in] pucMessage_ A pointer to the message body payload, or to a JSON
    //! document containing a top-level "body" field for JSON.
    //! \param[out] stInterMessage_ Decoded body. It may be partially populated on failure.
    //! \param[in, out] stMetaData_ Metadata describing this input. The base and OEM implementations
    //! read it without populating missing values; derived definition lookup may modify it.
    //
    //! \details Metadata fields used by the decoder:
    //! - bResponse: True for command responses. Responses supported for ASCII, ABB_ASCII, and BINARY only.
    //! - eFormat: ASCII, ABB_ASCII, BINARY, or JSON; Decode returns UNKNOWN for any other value.
    //! - uiLength: Length of the message frame, including any checksum and trailer.
    //!   For JSON, this is the entire document length. Ignored for BINARY.
    //! - uiBinaryMsgLength: Binary body length, excluding the header and frame CRC. Ignored for
    //!   ASCII, ABB_ASCII, and JSON.
    //! - uiHeaderLength: Number of frame bytes before pucMessage_ for ASCII and ABB_ASCII; must not
    //!   exceed uiLength. Set to zero for body-only input. Ignored for BINARY and JSON.
    //! - usMessageId: Used to look up the message definition; OEM responses use a response definition.
    //! - uiMessageCrc: Used to look up the field definitions; if not found, the latest definitions are used.
    //!   Ignored for responses.
    //! - messageName: If equal to "RXCONFIG", Decode returns UNSUPPORTED.
    //
    //! \remark The caller must provide at least uiBinaryMsgLength readable bytes for BINARY, uiLength
    //! for JSON, or uiLength - uiHeaderLength for ASCII and ABB_ASCII at pucMessage_. Input need not
    //! have a trailing NUL sentinel.
    //
    //! \remark Framers supply uiLength; HeaderDecoders supply eFormat, uiBinaryMsgLength, uiHeaderLength,
    //! usMessageId, uiMessageCrc, and messageName, but do not set uiLength. Direct callers must supply
    //! uiLength themselves.
    //
    //! \remark BINARY message fields of type FIELD_TYPE::STRING must be NUL-terminated.
    //
    //! \remark For ASCII, ABB_ASCII, and BINARY, Decode will return MALFORMED_INPUT when any message
    //! field is missing from the input buffer.
    //
    //! \return An error code describing the result of decoding.
    //!   SUCCESS: The operation was successful.
    //!   NULL_PROVIDED: pucMessage_ is a null pointer.
    //!   NO_DATABASE: No database was ever loaded into this component.
    //!   NO_DEFINITION: No message definition was found, or a response has an unsupported format.
    //!   MALFORMED_INPUT: The input lengths or body are invalid, or the JSON document is invalid
    //!   or lacks a top-level "body" field.
    //!   UNSUPPORTED: RXCONFIG requires RxConfigHandler.
    //!   UNKNOWN: The payload format provided is not supported.
    //! \throws std::runtime_error If a required field definition is missing or unsupported.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS Decode(const unsigned char* pucMessage_, CompositeField& stInterMessage_, MetaDataBase& stMetaData_) const;
};

} // namespace novatel::edie
