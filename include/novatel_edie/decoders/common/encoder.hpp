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
// ! \file encoder.hpp
// ===============================================================================

#ifndef ENCODER_HPP
#define ENCODER_HPP

#include <cstdarg>

#include "novatel_edie/common/logger.hpp"
#include "novatel_edie/decoders/common/message_database.hpp"
#include "novatel_edie/decoders/common/message_decoder.hpp"

namespace novatel::edie {

// -------------------------------------------------------------------------------------------------------
constexpr bool PrintAsString(const BaseField& pstFieldDef_)
{
    // Printing as a string means two things:
    // 1. The field will be surrounded by quotes
    // 2. The field will not contain null-termination or padding
    return pstFieldDef_.type == FIELD_TYPE::STRING || pstFieldDef_.conversionHash == CalculateBlockCrc32("s") ||
           pstFieldDef_.conversionHash == CalculateBlockCrc32("S");
}

// -------------------------------------------------------------------------------------------------------
constexpr bool IsCommaSeparated(const BaseField& pstFieldDef_)
{
    // In certain cases there are no separators printed between array elements
    return !PrintAsString(pstFieldDef_) && pstFieldDef_.conversionHash != CalculateBlockCrc32("Z") &&
           pstFieldDef_.conversionHash != CalculateBlockCrc32("P");
}

// -------------------------------------------------------------------------------------------------------
template <typename... Args> [[nodiscard]] bool PrintToBuffer(char** ppcBuffer_, uint32_t& uiBytesLeft_, const char* szFormat_, Args&&... args_)
{
    const uint32_t uiBytesBuffered = std::snprintf(*ppcBuffer_, uiBytesLeft_, szFormat_, std::forward<Args>(args_)...);
    if (uiBytesLeft_ < uiBytesBuffered) { return false; }
    *ppcBuffer_ += uiBytesBuffered;
    uiBytesLeft_ -= uiBytesBuffered;
    return true;
}

// -------------------------------------------------------------------------------------------------------
[[nodiscard]] inline bool SetInBuffer(unsigned char** ppucBuffer_, uint32_t& uiBytesLeft_, const int32_t iItem_, const uint32_t uiItemSize_)
{
    if (uiBytesLeft_ < uiItemSize_) { return false; }
    memset(*ppucBuffer_, iItem_, uiItemSize_);
    *ppucBuffer_ += uiItemSize_;
    uiBytesLeft_ -= uiItemSize_;
    return true;
}

// -------------------------------------------------------------------------------------------------------
template <typename T> constexpr std::array<char, 7> FloatingPointConversionString(const FieldContainer& fc_)
{
    static_assert(std::is_floating_point_v<T>, "FloatingPointConversionString must be called with a floating point type");
    std::array<char, 7> acConvert{};
    const int32_t iBefore = fc_.fieldDef->conversionBeforePoint;
    const int32_t iAfter = fc_.fieldDef->conversionAfterPoint;
    const auto absVal = fabs(std::get<T>(fc_.fieldValue));

    [[maybe_unused]] int32_t iRes = absVal < std::numeric_limits<T>::epsilon() ? snprintf(acConvert.data(), 7, "%%0.%df", iAfter)
                                    : iAfter == 0 && iBefore == 0              ? snprintf(acConvert.data(), 7, "%%0.1f")
                                    : absVal > pow(10, iBefore)                ? snprintf(acConvert.data(), 7, "%%0.%de", iBefore + iAfter - 1)
                                    : absVal < pow(10, -iBefore)               ? snprintf(acConvert.data(), 7, "%%0.%de", iAfter)
                                                                               : snprintf(acConvert.data(), 7, "%%0.%df", iAfter);

    assert(0 <= iRes && iRes < 7);
    return acConvert;
}

// -------------------------------------------------------------------------------------------------------
template <typename T> [[nodiscard]] bool CopyToBuffer(unsigned char** ppucBuffer_, uint32_t& uiBytesLeft_, T* ptItem_)
{
    uint32_t uiItemSize;

    if constexpr (std::is_same<T, const char>()) { uiItemSize = static_cast<uint32_t>(std::strlen(ptItem_)); }
    else { uiItemSize = sizeof(*ptItem_); }

    if (uiBytesLeft_ < uiItemSize) { return false; }
    memcpy(*ppucBuffer_, ptItem_, uiItemSize);
    *ppucBuffer_ += uiItemSize;
    uiBytesLeft_ -= uiItemSize;
    return true;
}

// -------------------------------------------------------------------------------------------------------
template <typename T> std::function<bool(const FieldContainer&, char**, uint32_t&, MessageDatabase&)> BasicMapEntry(const char* pcF_)
{
    return [pcF_](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcF_, std::get<T>(fc_.fieldValue));
    };
}

//============================================================================
//! \class EncoderBase
//! \brief Class to encode messages.
//============================================================================
class EncoderBase
{
  protected:
    std::shared_ptr<spdlog::logger> pclMyLogger{Logger::RegisterLogger("encoder")};
    MessageDatabase::Ptr pclMyMsgDb{nullptr};

    EnumDefinition::ConstPtr vMyCommandDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyPortAddressDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyGpsTimeStatusDefinitions{nullptr};

    std::unordered_map<uint64_t, std::function<bool(const FieldContainer&, char**, uint32_t&, [[maybe_unused]] MessageDatabase&)>> asciiFieldMap;
    std::unordered_map<uint64_t, std::function<bool(const FieldContainer&, char**, uint32_t&, [[maybe_unused]] MessageDatabase&)>> jsonFieldMap;
    // is there a way to do this with static variables instead?
    [[nodiscard]] virtual char SeparatorAscii() const { return ','; }
    [[nodiscard]] virtual char SeparatorAbbAscii() const { return ' '; }
    [[nodiscard]] virtual uint32_t IndentationLengthAbbAscii() const { return 5; }

    // Encode binary
    template <bool Flatten, bool Align>
    [[nodiscard]] bool EncodeBinaryBody(const std::vector<FieldContainer>& stInterMessage_, unsigned char** ppucOutBuf_,
                                        uint32_t& uiBytesLeft_) const;
    [[nodiscard]] virtual bool FieldToBinary(const FieldContainer& fc_, unsigned char** ppcOutBuf_, uint32_t& uiBytesLeft_) const;

    // Encode ascii
    template <bool Abbreviated>
    [[nodiscard]] bool EncodeAsciiBody(const std::vector<FieldContainer>& vIntermediateFormat_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                       [[maybe_unused]] uint32_t uiIndentationLevel_ = 1) const;
    [[nodiscard]] bool FieldToAscii(const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const;

    // Encode JSON
    [[nodiscard]] bool FieldToJson(const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const;
    [[nodiscard]] bool EncodeJsonBody(const std::vector<FieldContainer>& vIntermediateFormat_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const;

    virtual void InitEnumDefinitions();
    virtual void InitFieldMaps();

  public:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the Encoder class.
    //
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object. Defaults to nullptr.
    //----------------------------------------------------------------------------
    EncoderBase(MessageDatabase::Ptr pclMessageDb_ = nullptr);

    //----------------------------------------------------------------------------
    //! \brief A destructor for the Encoder class.
    //----------------------------------------------------------------------------
    virtual ~EncoderBase() = default;

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
    [[nodiscard]] std::shared_ptr<spdlog::logger> GetLogger() const;

    //----------------------------------------------------------------------------
    //! \brief Set the level of detail produced by the internal logger.
    //
    //! \param[in] eLevel_  The logging level to enable.
    //----------------------------------------------------------------------------
    void SetLoggerLevel(spdlog::level::level_enum eLevel_) const;
};

} // namespace novatel::edie

#endif // ENCODER_HPP
