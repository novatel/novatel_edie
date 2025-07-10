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
    pclMyLogger = GetBaseLoggerManager()->RegisterLogger("novatel_file_parser");
    pclMyLogger->debug("FileParser initialized");
}

// -------------------------------------------------------------------------------------------------------
void FileParser::LoadJsonDb(const MessageDatabase::Ptr& pclMessageDb_)
{
    if (pclMessageDb_ != nullptr) { clMyParser.LoadJsonDb(pclMessageDb_); }
    else { pclMyLogger->debug("JSON DB is a NULL pointer."); }
}

// ---------------------------------------------------------------------------
MessageDatabase::ConstPtr FileParser::MessageDb() const { return this->clMyParser.MessageDb(); }

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
    size_t ullBytesRead = pclMyInputStream->gcount();
    return ullBytesRead > 0 && clMyParser.Write(reinterpret_cast<unsigned char*>(cData.data()), ullBytesRead) == ullBytesRead;
}

// -------------------------------------------------------------------------------------------------------

template <typename ParserFunc, typename... Args> [[nodiscard]] STATUS FileParser::HandleRead(ParserFunc parserFunc_, Args&&... args_)
{

    while (true)
    {
        const STATUS eStatus = parserFunc_(std::forward<Args>(args_)...);
        switch (eStatus)
        {
        case STATUS::SUCCESS: return STATUS::SUCCESS;
        case STATUS::UNKNOWN: return STATUS::UNKNOWN;
        case STATUS::NO_DEFINITION: return STATUS::NO_DEFINITION;
        case STATUS::BUFFER_EMPTY: {
            if (ReadStream()) { continue; }
            return parserFunc_(std::forward<Args>(args_)..., true) == STATUS::SUCCESS ? STATUS::SUCCESS : STATUS::STREAM_EMPTY;
        }
        default: pclMyLogger->info("Encountered an error: {}\n", eStatus); return eStatus;
        }
    }
}

[[nodiscard]] STATUS FileParser::ReadIntermediate(MessageDataStruct& stMessageData_, IntermediateHeader& header_,
                                                  std::vector<FieldContainer>& stMessage_, MetaDataStruct& stMetaData_)
{
    return HandleRead([this](auto&&... params) { return clMyParser.ReadIntermediate(std::forward<decltype(params)>(params)...); }, stMessageData_,
                      header_, stMessage_, stMetaData_);
}

[[nodiscard]] STATUS FileParser::Read(MessageDataStruct& stMessageData_, MetaDataStruct& stMetaData_)
{
    return HandleRead([this](auto&&... params) { return clMyParser.Read(std::forward<decltype(params)>(params)...); }, stMessageData_, stMetaData_);
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
