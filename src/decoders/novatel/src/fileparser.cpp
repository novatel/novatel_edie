////////////////////////////////////////////////////////////////////////
//
// COPYRIGHT NovAtel Inc, 2022. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////
//                            DESCRIPTION
//
//! \file fileparser.cpp
//! \brief Parse an input stream for OEM logs.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "decoders/novatel/api/fileparser.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

// -------------------------------------------------------------------------------------------------------
FileParser::FileParser(const std::string sDbPath_)
    : clMyParser(Parser(sDbPath_)), pcMyStreamReadBuffer(new unsigned char[Parser::uiPARSER_INTERNAL_BUFFER_SIZE])
{
    stMyReadData.cData = reinterpret_cast<char*>(pcMyStreamReadBuffer);
    stMyReadData.uiDataSize = Parser::uiPARSER_INTERNAL_BUFFER_SIZE;
    pclMyInputStream = nullptr;
    pclMyLogger->debug("FileParser initialized");
}

// -------------------------------------------------------------------------------------------------------
FileParser::FileParser(const std::u32string sDbPath_)
    : clMyParser(Parser(sDbPath_)), pcMyStreamReadBuffer(new unsigned char[Parser::uiPARSER_INTERNAL_BUFFER_SIZE])
{
    pclMyLogger = Logger::RegisterLogger("novatel_fileparser");

    stMyReadData.cData = reinterpret_cast<char*>(pcMyStreamReadBuffer);
    stMyReadData.uiDataSize = Parser::uiPARSER_INTERNAL_BUFFER_SIZE;
    pclMyInputStream = nullptr;
    pclMyLogger->debug("FileParser initialized");
}

// -------------------------------------------------------------------------------------------------------
FileParser::FileParser(JsonReader* pclJsonDb_)
    : clMyParser(Parser(pclJsonDb_)), pcMyStreamReadBuffer(new unsigned char[Parser::uiPARSER_INTERNAL_BUFFER_SIZE])
{
    pclMyLogger = Logger::RegisterLogger("novatel_fileparser");

    stMyReadData.cData = reinterpret_cast<char*>(pcMyStreamReadBuffer);
    stMyReadData.uiDataSize = Parser::uiPARSER_INTERNAL_BUFFER_SIZE;
    pclMyInputStream = nullptr;
    pclMyLogger->debug("FileParser initialized");
}

// -------------------------------------------------------------------------------------------------------
FileParser::~FileParser() { delete[] pcMyStreamReadBuffer; }

// -------------------------------------------------------------------------------------------------------
void FileParser::LoadJsonDb(JsonReader* pclJsonDb_)
{
    if (pclJsonDb_ != nullptr) { clMyParser.LoadJsonDb(pclJsonDb_); }
    else { pclMyLogger->debug("JSON DB is a NULL pointer."); }
}

// -------------------------------------------------------------------------------------------------------
std::shared_ptr<spdlog::logger> FileParser::GetLogger() { return pclMyLogger; }

// -------------------------------------------------------------------------------------------------------
void FileParser::EnableFramerDecoderLogging(spdlog::level::level_enum eLevel_, std::string sFileName_)
{
    clMyParser.EnableFramerDecoderLogging(eLevel_, sFileName_);
}

// -------------------------------------------------------------------------------------------------------
void FileParser::SetLoggerLevel(spdlog::level::level_enum eLevel_) { pclMyLogger->set_level(eLevel_); }

// -------------------------------------------------------------------------------------------------------
void FileParser::ShutdownLogger() { Logger::Shutdown(); }

// -------------------------------------------------------------------------------------------------------
void FileParser::SetIgnoreAbbreviatedAsciiResponses(bool bIgnoreAbbreivatedAsciiResponses_)
{
    clMyParser.SetIgnoreAbbreviatedAsciiResponses(bIgnoreAbbreivatedAsciiResponses_);
}

// -------------------------------------------------------------------------------------------------------
bool FileParser::GetIgnoreAbbreviatedAsciiResponses() { return clMyParser.GetIgnoreAbbreviatedAsciiResponses(); }

// -------------------------------------------------------------------------------------------------------
void FileParser::SetDecompressRangeCmp(bool bDecompressRangeCmp_) { clMyParser.SetDecompressRangeCmp(bDecompressRangeCmp_); }

// -------------------------------------------------------------------------------------------------------
bool FileParser::GetDecompressRangeCmp() { return clMyParser.GetDecompressRangeCmp(); }

// -------------------------------------------------------------------------------------------------------
void FileParser::SetReturnUnknownBytes(bool bReturnUnknownBytes_) { clMyParser.SetReturnUnknownBytes(bReturnUnknownBytes_); }

// -------------------------------------------------------------------------------------------------------
bool FileParser::GetReturnUnknownBytes() { return clMyParser.GetReturnUnknownBytes(); }

// -------------------------------------------------------------------------------------------------------
void FileParser::SetEncodeFormat(ENCODEFORMAT eFormat_) { clMyParser.SetEncodeFormat(eFormat_); }

// -------------------------------------------------------------------------------------------------------
ENCODEFORMAT
FileParser::GetEncodeFormat() { return clMyParser.GetEncodeFormat(); }

// -------------------------------------------------------------------------------------------------------
Filter* FileParser::GetFilter() { return clMyParser.GetFilter(); }

// -------------------------------------------------------------------------------------------------------
void FileParser::SetFilter(Filter* pclFilter_) { return clMyParser.SetFilter(pclFilter_); }

// -------------------------------------------------------------------------------------------------------
uint32_t FileParser::GetPercentRead() const { return stMyStreamReadStatus.uiPercentStreamRead; }

// -------------------------------------------------------------------------------------------------------
unsigned char* FileParser::GetInternalBuffer() { return clMyParser.GetInternalBuffer(); }

// -------------------------------------------------------------------------------------------------------
bool FileParser::SetStream(InputFileStream* pclInputStream_)
{
    if (!pclInputStream_) { return false; }
    // TODO: This call is not implemented and returns false in the interface.
    // Is the stream available?
    // if (!pclInputStream_->IsStreamAvailable())
    // {
    //    return false;
    // }

    // Are there any bytes left to read in the stream?
    uint32_t uiReadSizeSave = stMyReadData.uiDataSize;
    stMyReadData.uiDataSize = 0;
    stMyStreamReadStatus = pclInputStream_->ReadData(stMyReadData);
    if (stMyStreamReadStatus.bEOS || stMyStreamReadStatus.uiPercentStreamRead >= 100) { return false; }
    stMyReadData.uiDataSize = uiReadSizeSave;

    pclMyInputStream = pclInputStream_;

    Reset();

    return true;
}

// -------------------------------------------------------------------------------------------------------
bool FileParser::ReadStream()
{
    stMyReadData.uiDataSize = MAX_ASCII_MESSAGE_LENGTH;
    stMyStreamReadStatus = pclMyInputStream->ReadData(stMyReadData);
    return stMyStreamReadStatus.uiCurrentStreamRead > 0 &&
           clMyParser.Write(reinterpret_cast<unsigned char*>(stMyReadData.cData), stMyStreamReadStatus.uiCurrentStreamRead) ==
               stMyStreamReadStatus.uiCurrentStreamRead;
}

// -------------------------------------------------------------------------------------------------------
[[nodiscard]] STATUS FileParser::Read(MessageDataStruct& stMessageData_, MetaDataStruct& stMetaData_)
{
    STATUS eStatus = STATUS::UNKNOWN;
    while (true)
    {
        eStatus = clMyParser.Read(stMessageData_, stMetaData_);

        if (eStatus == STATUS::SUCCESS || eStatus == STATUS::UNKNOWN) { break; }
        if (eStatus == STATUS::BUFFER_EMPTY)
        {
            if (!ReadStream())
            {
                return clMyParser.Read(stMessageData_, stMetaData_, true) == STATUS::SUCCESS ? STATUS::SUCCESS : STATUS::STREAM_EMPTY;
            }
        }
        else
        {
            pclMyLogger->info("Encountered an error: {}\n", static_cast<int32_t>(eStatus));
            break;
        }
    }

    return eStatus;
}

// -------------------------------------------------------------------------------------------------------
bool FileParser::Reset()
{
    Flush();
    if (pclMyInputStream != nullptr) { pclMyInputStream->Reset(0, std::ios::beg); }
    return true;
}

// -------------------------------------------------------------------------------------------------------
uint32_t FileParser::Flush(unsigned char* pucBuffer_, uint32_t uiBufferSize_) { return clMyParser.Flush(pucBuffer_, uiBufferSize_); }
