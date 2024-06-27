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
// ! \file header_decoder.hpp
// ===============================================================================

#ifndef NOVATEL_EDIE_DECODERS_HEADER_DECODER_HPP
#define NOVATEL_EDIE_DECODERS_HEADER_DECODER_HPP

#include <nlohmann/json.hpp>

#include "novatel_edie/common/common.hpp"
#include "novatel_edie/common/json_reader.hpp"
#include "novatel_edie/common/logger.hpp"
#include "novatel_edie/decoders/common.hpp"

namespace novatel::edie::oem {

//============================================================================
//! \class HeaderDecoder
//! \brief Decode framed OEM message headers.
//============================================================================
class HeaderDecoder
{
  private:
    std::shared_ptr<spdlog::logger> pclMyLogger{Logger::RegisterLogger("novatel_header_decoder")};
    JsonReader* pclMyMsgDb{nullptr};
    EnumDefinition* vMyCommandDefinitions{nullptr};
    EnumDefinition* vMyPortAddressDefinitions{nullptr};
    EnumDefinition* vMyGpsTimeStatusDefinitions{nullptr};
    MessageDefinition stMyResponseDefinition;

    // Decode novatel headers
    template <ASCII_HEADER eField> [[nodiscard]] bool DecodeAsciiHeaderField(IntermediateHeader& stInterHeader_, char** ppcLogBuf_) const;
    template <ASCII_HEADER... eFields> [[nodiscard]] bool DecodeAsciiHeaderFields(IntermediateHeader& stInterHeader_, char** ppcLogBuf_) const;
    void DecodeJsonHeader(nlohmann::json clJsonHeader_, IntermediateHeader& stInterHeader_) const;

  public:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the HeaderDecoder class.
    //
    //! \param[in] pclJsonDb_ A pointer to a JsonReader object. Defaults to nullptr.
    //----------------------------------------------------------------------------
    HeaderDecoder(JsonReader* pclJsonDb_ = nullptr);

    //----------------------------------------------------------------------------
    //! \brief Load a JsonReader object.
    //
    //! \param[in] pclJsonDb_ A pointer to a JsonReader object.
    //----------------------------------------------------------------------------
    void LoadJsonDb(JsonReader* pclJsonDb_);

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
    //! \brief Shutdown the internal logger.
    //----------------------------------------------------------------------------
    static void ShutdownLogger();

    //----------------------------------------------------------------------------
    //! \brief Decode an OEM message header from the provided frame.
    //
    //! \param[in] pucLogBuf_ A pointer to an OEM message header.
    //! \param[out] stInterHeader_ The IntermediateHeader to be populated.
    //! \param[in, out] stMetaData_ MetaDataStruct to provide information about
    //! the frame and be fully populated to help describe the decoded log.
    //
    //! \return An error code describing the result of decoding.
    //!   SUCCESS: The operation was successful.
    //!   NULL_PROVIDED: pucHeader_ is a null pointer.
    //!   NO_DATABASE: No database was ever loaded into this component.
    //!   FAILURE: Failed to decode a header field.
    //!   UNSUPPORTED: Attempted to decode an unsupported format.
    //!   UNKNOWN: The header format provided is not known.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS Decode(unsigned char* pucLogBuf_, IntermediateHeader& stInterHeader_, MetaDataStruct& stMetaData_) const;
};

} // namespace novatel::edie::oem

#endif
