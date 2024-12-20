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
// ! \file file_parser.cpp
// ===============================================================================

#include "novatel_edie/decoders/oem/file_parser.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

// -------------------------------------------------------------------------------------------------------
FileParser::FileParser(const std::string& sDbPath_) : clMyParser(Parser(sDbPath_)) { pclMyLogger->debug("FileParser initialized"); }

// -------------------------------------------------------------------------------------------------------
FileParser::FileParser(const std::u32string& sDbPath_) : clMyParser(Parser(sDbPath_))
{
    pclMyLogger = Logger::RegisterLogger("novatel_file_parser");
    pclMyLogger->debug("FileParser initialized");
}

// -------------------------------------------------------------------------------------------------------
FileParser::FileParser(JsonReader* pclJsonDb_) : clMyParser(Parser(pclJsonDb_))
{
    pclMyLogger = Logger::RegisterLogger("novatel_file_parser");
    pclMyLogger->debug("FileParser initialized");
}

// -------------------------------------------------------------------------------------------------------
FileParser::~FileParser() = default;

// -------------------------------------------------------------------------------------------------------
void FileParser::LoadJsonDb(JsonReader* pclJsonDb_)
{
    if (pclJsonDb_ != nullptr) { clMyParser.LoadJsonDb(pclJsonDb_); }
    else { pclMyLogger->debug("JSON DB is a NULL pointer."); }
}

// -------------------------------------------------------------------------------------------------------
std::shared_ptr<spdlog::logger> FileParser::GetLogger() const { return pclMyLogger; }

// -------------------------------------------------------------------------------------------------------
void FileParser::EnableFramerDecoderLogging(spdlog::level::level_enum eLevel_, const std::string& sFileName_)
{
    clMyParser.EnableFramerDecoderLogging(eLevel_, sFileName_);
}

// -------------------------------------------------------------------------------------------------------
void FileParser::SetLoggerLevel(spdlog::level::level_enum eLevel_) const { pclMyLogger->set_level(eLevel_); }

// -------------------------------------------------------------------------------------------------------
void FileParser::SetIgnoreAbbreviatedAsciiResponses(bool bIgnoreAbbreviatedAsciiResponses_)
{
    clMyParser.SetIgnoreAbbreviatedAsciiResponses(bIgnoreAbbreviatedAsciiResponses_);
}

// -------------------------------------------------------------------------------------------------------
bool FileParser::GetIgnoreAbbreviatedAsciiResponses() const { return clMyParser.GetIgnoreAbbreviatedAsciiResponses(); }

// -------------------------------------------------------------------------------------------------------
void FileParser::SetDecompressRangeCmp(bool bDecompressRangeCmp_) { clMyParser.SetDecompressRangeCmp(bDecompressRangeCmp_); }

// -------------------------------------------------------------------------------------------------------
bool FileParser::GetDecompressRangeCmp() const { return clMyParser.GetDecompressRangeCmp(); }

// -------------------------------------------------------------------------------------------------------
void FileParser::SetReturnUnknownBytes(bool bReturnUnknownBytes_) { clMyParser.SetReturnUnknownBytes(bReturnUnknownBytes_); }

// -------------------------------------------------------------------------------------------------------
bool FileParser::GetReturnUnknownBytes() const { return clMyParser.GetReturnUnknownBytes(); }

// -------------------------------------------------------------------------------------------------------
void FileParser::SetEncodeFormat(ENCODE_FORMAT eFormat_) { clMyParser.SetEncodeFormat(eFormat_); }

// -------------------------------------------------------------------------------------------------------
ENCODE_FORMAT FileParser::GetEncodeFormat() const { return clMyParser.GetEncodeFormat(); }

// -------------------------------------------------------------------------------------------------------
Filter* FileParser::GetFilter() const { return clMyParser.GetFilter(); }

// -------------------------------------------------------------------------------------------------------
void FileParser::SetFilter(Filter* pclFilter_) { return clMyParser.SetFilter(pclFilter_); }

// -------------------------------------------------------------------------------------------------------
unsigned char* FileParser::GetInternalBuffer() const { return clMyParser.GetInternalBuffer(); }

// -------------------------------------------------------------------------------------------------------
bool FileParser::SetStream(std::ifstream* pclInputStream_)
{
    if (pclInputStream_ == nullptr || pclInputStream_->eof()) { return false; }
    pclMyInputStream = pclInputStream_;
    Reset();
    return true;
}

// -------------------------------------------------------------------------------------------------------
bool FileParser::ReadStream()
{
    std::array<char, MAX_ASCII_MESSAGE_LENGTH> cData{};
    pclMyInputStream->read(cData.data(), cData.size());
    return pclMyInputStream->gcount() > 0 &&
           clMyParser.Write(reinterpret_cast<unsigned char*>(cData.data()), pclMyInputStream->gcount()) == pclMyInputStream->gcount();
}

// -------------------------------------------------------------------------------------------------------
[[nodiscard]] STATUS FileParser::Read(MessageDataStruct& stMessageData_, MetaDataStruct& stMetaData_)
{
    while (true)
    {
        STATUS eStatus = clMyParser.Read(stMessageData_, stMetaData_);

        switch (eStatus)
        {
        case STATUS::SUCCESS: return STATUS::SUCCESS;
        case STATUS::UNKNOWN: return STATUS::UNKNOWN;
        case STATUS::BUFFER_EMPTY:
            return ReadStream()                                                            ? STATUS::BUFFER_EMPTY
                   : clMyParser.Read(stMessageData_, stMetaData_, true) == STATUS::SUCCESS ? STATUS::SUCCESS
                                                                                           : STATUS::STREAM_EMPTY;
        default: pclMyLogger->info("Encountered an error: {}\n", static_cast<int32_t>(eStatus)); return eStatus;
        }
    }
}

// -------------------------------------------------------------------------------------------------------
bool FileParser::Reset()
{
    Flush();
    if (pclMyInputStream != nullptr)
    {
        pclMyInputStream->clear();
        pclMyInputStream->seekg(0, std::ios::beg);
    }
    return true;
}

// -------------------------------------------------------------------------------------------------------
uint32_t FileParser::Flush(unsigned char* pucBuffer_, uint32_t uiBufferSize_) { return clMyParser.Flush(pucBuffer_, uiBufferSize_); }
