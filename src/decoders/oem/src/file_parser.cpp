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

#include <istream>

using namespace novatel::edie;
using namespace novatel::edie::oem;

// -------------------------------------------------------------------------------------------------------
FileParser::FileParser(const std::filesystem::path& sDbPath_) : clMyParser(Parser(sDbPath_)) { pclMyLogger->debug("FileParser initialized"); }

// -------------------------------------------------------------------------------------------------------
FileParser::FileParser(const MessageDatabase::Ptr& pclMessageDb_) : clMyParser(Parser(pclMessageDb_))
{
    pclMyLogger = Logger::RegisterLogger("novatel_file_parser");
    pclMyLogger->debug("FileParser initialized");
}

// -------------------------------------------------------------------------------------------------------
void FileParser::LoadJsonDb(const MessageDatabase::Ptr& pclMessageDb_)
{
    if (pclMessageDb_ != nullptr) { clMyParser.LoadJsonDb(pclMessageDb_); }
    else { pclMyLogger->debug("JSON DB is a NULL pointer."); }
}

// -------------------------------------------------------------------------------------------------------
bool FileParser::SetStream(std::shared_ptr<std::istream> pclInputStream_)
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
    const STATUS eStatus = clMyParser.Read(stMessageData_, stMetaData_);

    switch (eStatus)
    {
    case STATUS::SUCCESS: return STATUS::SUCCESS;
    case STATUS::UNKNOWN: return STATUS::UNKNOWN;
    case STATUS::BUFFER_EMPTY: {
        if (ReadStream()) { return STATUS::BUFFER_EMPTY; }
        return clMyParser.Read(stMessageData_, stMetaData_, true) == STATUS::SUCCESS ? STATUS::SUCCESS : STATUS::STREAM_EMPTY;
    }
    default: pclMyLogger->info("Encountered an error: {}\n", eStatus); return eStatus;
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
