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

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "decoders/common/api/common.hpp"
#include "decoders/common/api/jsonreader.hpp"
#include "decoders/novatel/api/common.hpp"
#include "decoders/novatel/api/encoder.hpp"
#include "decoders/novatel/api/message_decoder.hpp"

namespace novatel::edie::oem {

//============================================================================
//! \class Commander
//! \brief Class to convert OEM abbreviated ASCII commands to full ASCII or
//! BINARY commands.
//============================================================================
class Commander
{
  private:
    std::shared_ptr<spdlog::logger> pclMyLogger{Logger::RegisterLogger("novatel_commander")};
    MessageDecoder clMyMessageDecoder;
    Encoder clMyEncoder;
    JsonReader* pclMyMsgDb{nullptr};

    EnumDefinition* vMyRespDefns{nullptr};
    EnumDefinition* vMyCommandDefns{nullptr};
    EnumDefinition* vMyPortAddrDefns{nullptr};
    EnumDefinition* vMyGPSTimeStatusDefns{nullptr};

    MessageDefinition stMyRespDef;

    // Enum util functions
    void InitEnumDefns();
    void CreateResponseMsgDefns();

  public:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the Commander class.
    //
    //! \param[in] pclJsonDb_ A pointer to a JsonReader object. Defaults to nullptr.
    //----------------------------------------------------------------------------
    Commander(JsonReader* pclJsonDb_ = nullptr);

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
    std::shared_ptr<spdlog::logger> GetLogger() const;

    //----------------------------------------------------------------------------
    //! \brief Set the level of detail produced by the internal logger.
    //
    //! \param[in] eLevel_ The logging level to enable.
    //----------------------------------------------------------------------------
    void SetLoggerLevel(spdlog::level::level_enum eLevel_);

    //----------------------------------------------------------------------------
    //! \brief Shutdown the internal logger.
    //----------------------------------------------------------------------------
    static void ShutdownLogger();

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
    //!   UNSUPPORTED: eEncodeFormat_ is not ASCII or BINARY.
    //!   NO_DATABASE: No database was ever loaded into this component.
    //!   NO_DEFINITION: No definition was found in the database that corresponds
    //! to the provided Abbreviated ASCII command.
    //!   BUFFER_FULL: An attempt was made to write bytes to the provided buffer,
    //! but the buffer is already full or could not write the bytes without
    //! over-running.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS Encode(const char* pcAbbrevAsciiCommand_, uint32_t uiAbbrevAsciiCommandLength_, char* pcEncodeBuffer_,
                                uint32_t& uiEncodeBufferSize_, ENCODEFORMAT eEncodeFormat_);

    //----------------------------------------------------------------------------
    //! \brief A static method to encode an abbreviated ASCII command to a full
    //! ASCII or BINARY command.
    //
    //! \param[in] clJsonDb_ A reference to a JsonReader object.
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
    //!   UNSUPPORTED: eEncodeFormat_ is not ASCII or BINARY.
    //!   NO_DATABASE: No database was ever loaded into this component.
    //!   NO_DEFINITION: No definition was found in the database that corresponds
    //! to the provided Abbreviated ASCII command.
    //!   BUFFER_FULL: An attempt was made to write bytes to the provided buffer,
    //! but the buffer is already full or could not write the bytes without
    //! over-running.
    //----------------------------------------------------------------------------
    [[nodiscard]] static STATUS Encode(JsonReader& clJsonDb_, MessageDecoder& clMessageDecoder_, Encoder& clEncoder_,
                                       const char* pcAbbrevAsciiCommand_, const uint32_t uiAbbrevAsciiCommandLength_, char* pcEncodeBuffer_,
                                       uint32_t& uiEncodeBufferSize_, const ENCODEFORMAT eEncodeFormat_)
    {
        constexpr uint32_t thisPort = 0xC0;

        if (!pcAbbrevAsciiCommand_ || !pcEncodeBuffer_) { return STATUS::NULL_PROVIDED; }

        if (eEncodeFormat_ != ENCODEFORMAT::ASCII && eEncodeFormat_ != ENCODEFORMAT::BINARY) { return STATUS::UNSUPPORTED; }

        const std::string strAbbrevAsciiCommand = std::string(pcAbbrevAsciiCommand_, uiAbbrevAsciiCommandLength_);
        const size_t ullPos = strAbbrevAsciiCommand.find_first_of(' ');
        const std::string strCmdName = strAbbrevAsciiCommand.substr(0, ullPos);
        const std::string strCmdParams = strAbbrevAsciiCommand.substr(ullPos + 1, strAbbrevAsciiCommand.length());

        unsigned char acCmdParams[MAX_ASCII_MESSAGE_LENGTH];
        unsigned char* pucCmdParams = acCmdParams;
        strcpy(reinterpret_cast<char*>(acCmdParams), strCmdParams.c_str());

        const MessageDefinition* pclMessageDef = clJsonDb_.GetMsgDef(strCmdName);
        if (!pclMessageDef) { return STATUS::NO_DEFINITION; }

        MessageDataStruct stMessageData;
        MetaDataStruct stMetaData;
        IntermediateHeader stIntermediateHeader;
        IntermediateMessage stIntermediateMessage;

        // Prime the metadata with information we already know
        stMetaData.eFormat = HEADERFORMAT::ABB_ASCII;
        stMetaData.usMessageID = static_cast<uint16_t>(pclMessageDef->logID);
        stMetaData.uiMessageCRC = static_cast<uint32_t>(pclMessageDef->fields.begin()->first);

        const STATUS eDecoderStatus = clMessageDecoder_.Decode(pucCmdParams, stIntermediateMessage, stMetaData);
        if (eDecoderStatus != STATUS::SUCCESS) { return eDecoderStatus; }

        // Prime the intermediate header with information we already know
        stIntermediateHeader.uiPortAddress = thisPort;
        stIntermediateHeader.usMessageID = stMetaData.usMessageID;
        stIntermediateHeader.uiMessageDefinitionCRC = stMetaData.uiMessageCRC;

        const STATUS eEncoderStatus = clEncoder_.Encode(reinterpret_cast<unsigned char**>(&pcEncodeBuffer_), uiEncodeBufferSize_,
                                                        stIntermediateHeader, stIntermediateMessage, stMessageData, stMetaData, eEncodeFormat_);

        if (eEncoderStatus != STATUS::SUCCESS) { return eEncoderStatus; }

        // Null-terminate the command, if possible.  Otherwise the command will be the size of the
        // buffer.
        if (stMessageData.uiMessageLength < uiEncodeBufferSize_) { stMessageData.pucMessage[stMessageData.uiMessageLength] = '\0'; }
        uiEncodeBufferSize_ = stMessageData.uiMessageLength;

        return STATUS::SUCCESS;
    }
};

} // namespace novatel::edie::oem

#endif // NOVATEL_COMMANDER_HPP
