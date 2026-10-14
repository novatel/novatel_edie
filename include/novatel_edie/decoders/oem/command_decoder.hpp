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
// ! \file command_decoder.hpp
// ===============================================================================

#ifndef NOVATEL_COMMAND_DECODER_HPP
#define NOVATEL_COMMAND_DECODER_HPP

#include "novatel_edie/decoders/common/common.hpp"
#include "novatel_edie/decoders/common/message_database.hpp"
#include "novatel_edie/decoders/oem/common.hpp"
#include "novatel_edie/decoders/oem/message_decoder.hpp"

namespace novatel::edie::oem {

//============================================================================
//! \class CommandDecoder
//! \brief Class to decode OEM abbreviated ASCII commands into their header and
//! fields, which can then be encoded to full ASCII or BINARY with the Encoder.
//
//! Parameters left out of the command are filled in from the database, as the
//! receiver does:
//! - An enum field with a default that the next parameter is not a value of
//! was left out, so it takes its default and the parameter is used for the
//! fields that follow. For example, "LOG BESTPOSA" is "LOG THISPORT BESTPOSA".
//! - Fields left off the end of the command take their defaults. Variable
//! length and field arrays left off the end are empty.
//! A field without a default in the database must be given.
//============================================================================
class CommandDecoder
{
  private:
    std::shared_ptr<spdlog::logger> pclMyLogger{GetBaseLoggerManager()->RegisterLogger("novatel_command_decoder")};
    MessageDecoder clMyMessageDecoder;
    MessageDatabase::Ptr pclMyMsgDb{nullptr};

  public:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the CommandDecoder class.
    //
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object. Defaults to nullptr.
    //----------------------------------------------------------------------------
    CommandDecoder(MessageDatabase::Ptr pclMessageDb_ = nullptr);

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
    //! \brief Decode an abbreviated ASCII command.
    //
    //! \param[in] pcCommand_ A buffer containing the abbreviated ASCII command,
    //! optionally terminated by a CRLF.
    //! \param[in] uiCommandLength_ The length of the command contained within
    //! pcCommand_.
    //! \param[out] stHeader_ The header of the command, as it is sent on THISPORT.
    //! \param[out] stMessage_ The fields of the command, including any that were
    //! filled in from the database.
    //
    //! \return An error code describing the result of decoding.
    //!   SUCCESS: The command was decoded into stHeader_ and stMessage_.
    //!   NULL_PROVIDED: pcCommand_ is a null pointer.
    //!   NO_DATABASE: No database was ever loaded into this component.
    //!   NO_DEFINITION: No definition was found in the database that corresponds
    //! to the command name.
    //!   MALFORMED_INPUT: The command is empty, has a parameter that is not valid
    //! for its field, has too many parameters, or leaves out a field that has no
    //! default.
    //!   BUFFER_FULL: The command, with the parameters that were filled in, is
    //! longer than the longest supported abbreviated ASCII message.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS Decode(const char* pcCommand_, uint32_t uiCommandLength_, IntermediateHeader& stHeader_, CompositeField& stMessage_) const;
};

} // namespace novatel::edie::oem

#endif // NOVATEL_COMMAND_DECODER_HPP
