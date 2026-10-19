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
// ! \file commander.hpp
// ===============================================================================

#ifndef NOVATEL_COMMANDER_HPP
#define NOVATEL_COMMANDER_HPP

#include <optional>

#include "novatel_edie/decoders/common/common.hpp"
#include "novatel_edie/decoders/common/message_database.hpp"
#include "novatel_edie/decoders/oem/common.hpp"
#include "novatel_edie/decoders/oem/encoder.hpp"
#include "novatel_edie/decoders/oem/message_decoder.hpp"

namespace novatel::edie::oem {

//============================================================================
//! \struct CommandMetadata
//! \brief Metadata describing the response and optional payload of a command.
//
//! A command is answered by the receiver with a response whose header carries the command's message
//! ID. This is the case for both ASCII and Binary commands. A "LOG ... ONCE" command is also followed
//! by the requested log, which is the payload. The payload is output in the format given by the log's
//! name, such as BESTXYZA or BESTXYZB, regardless of the format the command was sent in. It is
//! identified by its message ID and the port it is output on, because the same log may also be
//! output on other ports in the background.
//
//! - commandId: The message ID of the command, which is echoed in the header of its response.
//! - payloadId: For "LOG ... ONCE", the message ID of the requested log. 0 if no log follows the
//! response.
//! - payloadPort: The port the requested log is output on. Only meaningful when payloadId is not 0.
//! Empty if the log is requested on THISPORT, which is the port the command was sent on.
//============================================================================
struct CommandMetadata
{
    uint16_t commandId{0};
    uint16_t payloadId{0};
    std::optional<uint32_t> payloadPort;
};

//============================================================================
//! \enum DEFAULT_PARAMETER_POLICY
//! \brief Controls how omitted abbreviated ASCII command parameters are handled.
//============================================================================
enum class DEFAULT_PARAMETER_POLICY
{
    REQUIRE_ALL_PARAMETERS, //!< Reject a command with omitted parameters.
    DATABASE_DEFAULTS       //!< Fill in omitted parameters with defaults from the database.
};

//============================================================================
//! \class Commander
//! \brief Class to convert OEM abbreviated ASCII commands to full ASCII or
//! BINARY commands.
//============================================================================
class Commander
{
  private:
    std::shared_ptr<spdlog::logger> pclMyLogger{GetBaseLoggerManager()->RegisterLogger("novatel_commander")};
    MessageDecoder clMyMessageDecoder;
    Encoder clMyEncoder;
    MessageDatabase::Ptr pclMyMsgDb{nullptr};
    DEFAULT_PARAMETER_POLICY eMyDefaultParameterPolicy{DEFAULT_PARAMETER_POLICY::REQUIRE_ALL_PARAMETERS};

    EnumDefinition::ConstPtr vMyCommandDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyPortAddressDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyGpsTimeStatusDefinitions{nullptr};

    // Enum util functions
    void InitEnumDefinitions();

  public:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the Commander class.
    //
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object. Defaults to nullptr.
    //----------------------------------------------------------------------------
    Commander(MessageDatabase::Ptr pclMessageDb_ = nullptr);

    //----------------------------------------------------------------------------
    //! \brief A constructor for the Commander class with an explicit parameter policy.
    //
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object.
    //! \param[in] eDefaultParameterPolicy_ The policy used for omitted parameters.
    //----------------------------------------------------------------------------
    Commander(MessageDatabase::Ptr pclMessageDb_, DEFAULT_PARAMETER_POLICY eDefaultParameterPolicy_);

    //----------------------------------------------------------------------------
    //! \brief Set the policy used for omitted abbreviated ASCII parameters.
    //
    //! \param[in] eDefaultParameterPolicy_ The policy to use for subsequent encodes.
    //----------------------------------------------------------------------------
    void SetDefaultParameterPolicy(DEFAULT_PARAMETER_POLICY eDefaultParameterPolicy_) { eMyDefaultParameterPolicy = eDefaultParameterPolicy_; }

    //----------------------------------------------------------------------------
    //! \brief Get the policy used for omitted abbreviated ASCII parameters.
    //
    //! \return The policy used for subsequent encodes.
    //----------------------------------------------------------------------------
    [[nodiscard]] DEFAULT_PARAMETER_POLICY GetDefaultParameterPolicy() const { return eMyDefaultParameterPolicy; }

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
    [[nodiscard]] std::shared_ptr<spdlog::logger> GetLogger() const { return pclMyLogger; }

    //----------------------------------------------------------------------------
    //! \brief Set the level of detail produced by the internal logger.
    //
    //! \param[in] eLevel_ The logging level to enable.
    //----------------------------------------------------------------------------
    void SetLoggerLevel(spdlog::level::level_enum eLevel_) const { pclMyLogger->set_level(eLevel_); }

    //----------------------------------------------------------------------------
    //! \brief Encode an abbreviated ASCII command to a full ASCII or BINARY
    //! command.
    //
    //! \param[in] pcAbbrevAsciiCommand_ A buffer containing the abbreviated
    //! ASCII command to be .
    //! \param[in] uiAbbrevAsciiCommandLength_ The length of the command contained
    //! within pcAbbrevAsciiCommand_.
    //! \param[out] pcEncodeBuffer_ The buffer to return the encoded command to.
    //! \param[out] uiEncodeBufferSize_ The length of pcEncodeBuffer_, upon return
    //! will indicate the length of the encoded message
    //! \param[in] eEncodeFormat_ The format that the abbreviated ASCII command
    //! contained in pcAbbrevAsciiCommand_ should be encoded to.
    //
    //! \return An error code describing the result of encoding.
    //!   SUCCESS: The command was successfully encoded and is now contained in
    //! pcEncodeBuffer_.
    //!   NULL_PROVIDED: Either pcAbbrevAsciiCommand_ or pcEncodeBuffer_ are null
    //! pointers.
    //!   UNSUPPORTED: eEncodeFormat_ is not ASCII or BINARY, or the selected
    //! policy does not support the command.
    //!   MALFORMED_INPUT: The command contains an unknown or extra parameter, or
    //! omits a parameter not supported by the selected policy.
    //!   NO_DATABASE: No database was ever loaded into this component.
    //!   NO_DEFINITION: No definition was found in the database that corresponds
    //! to the provided Abbreviated ASCII command.
    //!   BUFFER_FULL: An attempt was made to write bytes to the provided buffer,
    //! but the buffer is already full or could not write the bytes without
    //! over-running.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS Encode(const char* pcAbbrevAsciiCommand_, uint32_t uiAbbrevAsciiCommandLength_, char* pcEncodeBuffer_,
                                uint32_t& uiEncodeBufferSize_, ENCODE_FORMAT eEncodeFormat_);

    //! \brief Encode a command and return metadata describing its expected response.
    [[nodiscard]] STATUS Encode(const char* pcAbbrevAsciiCommand_, uint32_t uiAbbrevAsciiCommandLength_, char* pcEncodeBuffer_,
                                uint32_t& uiEncodeBufferSize_, ENCODE_FORMAT eEncodeFormat_, CommandMetadata& stMetadata_);

    //----------------------------------------------------------------------------
    //! \brief A static method to encode an abbreviated ASCII command to a full
    //! ASCII or BINARY command.
    //
    //! \param[in] clJsonDb_ A reference to a MessageDatabase object.
    //! \param[in] clMessageDecoder_ A reference to a MessageDecoder object.
    //! \param[in] clEncoder_ A reference to an Encoder object.
    //! \param[in] pcAbbrevAsciiCommand_ A buffer containing the abbreviated
    //! ASCII command to be .
    //! \param[in] uiAbbrevAsciiCommandLength_ The length of the command contained
    //! within pcAbbrevAsciiCommand_.
    //! \param[out] pcEncodeBuffer_ The buffer to return the encoded command to.
    //! \param[out] uiEncodeBufferSize_ The length of pcEncodeBuffer_, upon return
    //! will indicate the length of the encoded message
    //! \param[in] eEncodeFormat_ The format that the abbreviated ASCII command
    //! contained in pcAbbrevAsciiCommand_ should be encoded to.
    //
    //! \return An error code describing the result of encoding.
    //!   SUCCESS: The command was successfully encoded and is now contained in
    //! pcEncodeBuffer_.
    //!   NULL_PROVIDED: Either pcAbbrevAsciiCommand_ or pcEncodeBuffer_ are null
    //! pointers.
    //!   UNSUPPORTED: eEncodeFormat_ is not ASCII or BINARY, or the selected
    //! policy does not support the command.
    //!   MALFORMED_INPUT: The command contains an unknown or extra parameter, or
    //! omits a parameter not supported by the selected policy.
    //!   NO_DATABASE: No database was ever loaded into this component.
    //!   NO_DEFINITION: No definition was found in the database that corresponds
    //! to the provided Abbreviated ASCII command.
    //!   BUFFER_FULL: An attempt was made to write bytes to the provided buffer,
    //! but the buffer is already full or could not write the bytes without
    //! over-running.
    //----------------------------------------------------------------------------
    [[nodiscard]] static STATUS Encode(const MessageDatabase& clJsonDb_, const MessageDecoder& clMessageDecoder_, Encoder& clEncoder_,
                                       const char* pcAbbrevAsciiCommand_, uint32_t uiAbbrevAsciiCommandLength_, char* pcEncodeBuffer_,
                                       uint32_t& uiEncodeBufferSize_, ENCODE_FORMAT eEncodeFormat_);

    //! \brief Encode a command using an explicit omitted-parameter policy.
    [[nodiscard]] static STATUS Encode(const MessageDatabase& clJsonDb_, const MessageDecoder& clMessageDecoder_, Encoder& clEncoder_,
                                       const char* pcAbbrevAsciiCommand_, uint32_t uiAbbrevAsciiCommandLength_, char* pcEncodeBuffer_,
                                       uint32_t& uiEncodeBufferSize_, ENCODE_FORMAT eEncodeFormat_,
                                       DEFAULT_PARAMETER_POLICY eDefaultParameterPolicy_);

    //! \brief Encode a command and return metadata describing its expected response.
    [[nodiscard]] static STATUS Encode(const MessageDatabase& clJsonDb_, const MessageDecoder& clMessageDecoder_, Encoder& clEncoder_,
                                       const char* pcAbbrevAsciiCommand_, uint32_t uiAbbrevAsciiCommandLength_, char* pcEncodeBuffer_,
                                       uint32_t& uiEncodeBufferSize_, ENCODE_FORMAT eEncodeFormat_, CommandMetadata& stMetadata_);

    //! \brief Encode a command, return metadata, and use an explicit omitted-parameter policy.
    [[nodiscard]] static STATUS Encode(const MessageDatabase& clJsonDb_, const MessageDecoder& clMessageDecoder_, Encoder& clEncoder_,
                                       const char* pcAbbrevAsciiCommand_, uint32_t uiAbbrevAsciiCommandLength_, char* pcEncodeBuffer_,
                                       uint32_t& uiEncodeBufferSize_, ENCODE_FORMAT eEncodeFormat_, CommandMetadata& stMetadata_,
                                       DEFAULT_PARAMETER_POLICY eDefaultParameterPolicy_);
};

} // namespace novatel::edie::oem

#endif // NOVATEL_COMMANDER_HPP
