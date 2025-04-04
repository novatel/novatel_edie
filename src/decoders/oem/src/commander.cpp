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
// ! \file commander.cpp
// ===============================================================================

#include "novatel_edie/decoders/oem/commander.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

// -------------------------------------------------------------------------------------------------------
Commander::Commander(MessageDatabase::Ptr pclMessageDb_) : clMyMessageDecoder(pclMessageDb_), clMyEncoder(pclMessageDb_)
{
    pclMyLogger->debug("Commander initializing...");
    if (pclMessageDb_ != nullptr) { LoadJsonDb(pclMessageDb_); }
    pclMyLogger->debug("Commander initialized");
}

// -------------------------------------------------------------------------------------------------------
void Commander::LoadJsonDb(MessageDatabase::Ptr pclMessageDb_)
{
    pclMyMsgDb = pclMessageDb_;
    InitEnumDefinitions();
    CreateResponseMsgDefinitions();
}

// -------------------------------------------------------------------------------------------------------
void Commander::InitEnumDefinitions()
{
    vMyResponseDefinitions = pclMyMsgDb->GetEnumDefName("Responses");
    vMyCommandDefinitions = pclMyMsgDb->GetEnumDefName("Commands");
    vMyPortAddressDefinitions = pclMyMsgDb->GetEnumDefName("PortAddress");
    vMyGpsTimeStatusDefinitions = pclMyMsgDb->GetEnumDefName("GPSTimeStatus");
}

// -------------------------------------------------------------------------------------------------------
void Commander::CreateResponseMsgDefinitions()
{
    // Numerical response ID
    SimpleDataType stRespIdDataType;
    stRespIdDataType.description = "Response as numerical id";
    stRespIdDataType.length = 4;
    stRespIdDataType.name = DATA_TYPE::UINT;

    EnumField stRespIdField;
    stRespIdField.name = "response_id";
    stRespIdField.type = FIELD_TYPE::RESPONSE_ID;
    stRespIdField.dataType = stRespIdDataType;
    if (vMyResponseDefinitions != nullptr) { stRespIdField.enumId = vMyResponseDefinitions->_id; }
    stRespIdField.enumDef = vMyResponseDefinitions;

    // String response ID
    SimpleDataType stRespStrDataType;
    stRespStrDataType.description = "Response as a string";
    stRespStrDataType.length = 1;
    stRespStrDataType.name = DATA_TYPE::CHAR;

    BaseField stRespStrField;
    stRespStrField.name = "response_str";
    stRespStrField.type = FIELD_TYPE::RESPONSE_STR;
    stRespStrField.dataType = stRespStrDataType;

    // Message Definition
    stMyRespDef = std::make_shared<MessageDefinition>();
    stMyRespDef->name = "response";
    stMyRespDef->fields[0]; // responses don't have CRCs, hardcoding in 0 as the key to the fields map
    stMyRespDef->fields[0].emplace_back(std::make_shared<EnumField>(stRespIdField));
    stMyRespDef->fields[0].emplace_back(std::make_shared<BaseField>(stRespStrField));
}

// -------------------------------------------------------------------------------------------------------
STATUS Commander::Encode(const char* pcAbbrevAsciiCommand_, const uint32_t uiAbbrevAsciiCommandLength_, char* pcEncodeBuffer_,
                         uint32_t& uiEncodeBufferSize_, const ENCODE_FORMAT eEncodeFormat_)
{
    constexpr uint32_t thisPort = 0xC0;

    if ((pcAbbrevAsciiCommand_ == nullptr) || (pcEncodeBuffer_ == nullptr)) { return STATUS::NULL_PROVIDED; }

    if (eEncodeFormat_ != ENCODE_FORMAT::ASCII && eEncodeFormat_ != ENCODE_FORMAT::BINARY) { return STATUS::UNSUPPORTED; }

    const auto strAbbrevAsciiCommand = std::string(pcAbbrevAsciiCommand_, uiAbbrevAsciiCommandLength_);
    const size_t ullPos = strAbbrevAsciiCommand.find_first_of(' ');
    const std::string strCmdName = strAbbrevAsciiCommand.substr(0, ullPos);
    const std::string strCmdParams = strAbbrevAsciiCommand.substr(ullPos + 1);

    char acCmdParams[MAX_ASCII_MESSAGE_LENGTH];
    char* pcCmdParams = acCmdParams;
    std::copy(strCmdParams.begin(), strCmdParams.end(), acCmdParams);
    acCmdParams[strCmdParams.size()] = '\0';

    if (pclMyMsgDb == nullptr) { return STATUS::NO_DATABASE; }

    MessageDefinition::ConstPtr pclMessageDef = pclMyMsgDb->GetMsgDef(strCmdName);
    if (!pclMessageDef) { return STATUS::NO_DEFINITION; }

    MessageDataStruct stMessageData;
    MetaDataStruct stMetaData;
    IntermediateHeader stIntermediateHeader;
    std::vector<FieldContainer> stIntermediateMessage;

    // Prime the metadata with information we already know
    stMetaData.eFormat = HEADER_FORMAT::ABB_ASCII;
    stMetaData.usMessageId = static_cast<uint16_t>(pclMessageDef->logID);
    stMetaData.uiMessageCrc = static_cast<uint32_t>(pclMessageDef->fields.begin()->first);

    STATUS eStatus = clMyMessageDecoder.Decode(reinterpret_cast<unsigned char*>(pcCmdParams), stIntermediateMessage, stMetaData);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    // Prime the intermediate header with information we already know
    stIntermediateHeader.uiPortAddress = thisPort;
    stIntermediateHeader.usMessageId = stMetaData.usMessageId;
    stIntermediateHeader.uiMessageDefinitionCrc = stMetaData.uiMessageCrc;

    eStatus = clMyEncoder.Encode(reinterpret_cast<unsigned char**>(&pcEncodeBuffer_), uiEncodeBufferSize_, stIntermediateHeader,
                                 stIntermediateMessage, stMessageData, stMetaData.eFormat, eEncodeFormat_);

    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    // Null-terminate the command, if possible. Otherwise, the command will be the size of the buffer.
    if (stMessageData.uiMessageLength < uiEncodeBufferSize_) { stMessageData.pucMessage[stMessageData.uiMessageLength] = '\0'; }
    uiEncodeBufferSize_ = stMessageData.uiMessageLength;

    return STATUS::SUCCESS;
}

// -------------------------------------------------------------------------------------------------------
STATUS Commander::Encode(const MessageDatabase& clJsonDb_, const MessageDecoder& clMessageDecoder_, Encoder& clEncoder_,
                         const char* pcAbbrevAsciiCommand_, const uint32_t uiAbbrevAsciiCommandLength_, char* pcEncodeBuffer_,
                         uint32_t& uiEncodeBufferSize_, const ENCODE_FORMAT eEncodeFormat_)
{
    constexpr uint32_t thisPort = 0xC0;

    if ((pcAbbrevAsciiCommand_ == nullptr) || (pcEncodeBuffer_ == nullptr)) { return STATUS::NULL_PROVIDED; }

    if (eEncodeFormat_ != ENCODE_FORMAT::ASCII && eEncodeFormat_ != ENCODE_FORMAT::BINARY) { return STATUS::UNSUPPORTED; }

    const auto strAbbrevAsciiCommand = std::string(pcAbbrevAsciiCommand_, uiAbbrevAsciiCommandLength_);
    const size_t ullPos = strAbbrevAsciiCommand.find_first_of(' ');
    const std::string strCmdName = strAbbrevAsciiCommand.substr(0, ullPos);
    const std::string strCmdParams = strAbbrevAsciiCommand.substr(ullPos + 1);

    unsigned char acCmdParams[MAX_ASCII_MESSAGE_LENGTH];
    unsigned char* pucCmdParams = acCmdParams;
    std::copy(strCmdParams.begin(), strCmdParams.end(), acCmdParams);
    acCmdParams[strCmdParams.size()] = '\0';

    MessageDefinition::ConstPtr pclMessageDef = clJsonDb_.GetMsgDef(strCmdName);
    if (!pclMessageDef) { return STATUS::NO_DEFINITION; }

    MessageDataStruct stMessageData;
    MetaDataStruct stMetaData;
    IntermediateHeader stIntermediateHeader;
    std::vector<FieldContainer> stIntermediateMessage;

    // Prime the metadata with information we already know
    stMetaData.eFormat = HEADER_FORMAT::ABB_ASCII;
    stMetaData.usMessageId = static_cast<uint16_t>(pclMessageDef->logID);
    stMetaData.uiMessageCrc = static_cast<uint32_t>(pclMessageDef->fields.begin()->first);

    const STATUS eDecoderStatus = clMessageDecoder_.Decode(pucCmdParams, stIntermediateMessage, stMetaData);
    if (eDecoderStatus != STATUS::SUCCESS) { return eDecoderStatus; }

    // Prime the intermediate header with information we already know
    stIntermediateHeader.uiPortAddress = thisPort;
    stIntermediateHeader.usMessageId = stMetaData.usMessageId;
    stIntermediateHeader.uiMessageDefinitionCrc = stMetaData.uiMessageCrc;

    const STATUS eEncoderStatus = clEncoder_.Encode(reinterpret_cast<unsigned char**>(&pcEncodeBuffer_), uiEncodeBufferSize_, stIntermediateHeader,
                                                    stIntermediateMessage, stMessageData, stMetaData.eFormat, eEncodeFormat_);

    if (eEncoderStatus != STATUS::SUCCESS) { return eEncoderStatus; }

    // Null-terminate the command, if possible. Otherwise, the command will be the size of the buffer.
    if (stMessageData.uiMessageLength < uiEncodeBufferSize_) { stMessageData.pucMessage[stMessageData.uiMessageLength] = '\0'; }
    uiEncodeBufferSize_ = stMessageData.uiMessageLength;

    return STATUS::SUCCESS;
}
