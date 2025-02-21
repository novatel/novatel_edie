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
// ! \file json_db_reader.hpp
// ===============================================================================

#ifndef JSON_DB_READER_HPP
#define JSON_DB_READER_HPP

#include <filesystem>
#include <sstream>

#include <nlohmann/json.hpp>

#include "novatel_edie/decoders/common/message_database.hpp"

namespace novatel::edie {

//============================================================================
//! \class JsonDbReaderFailure
//! \brief Exception to be thrown when JsonDbReader fails to parse a JSON.
//============================================================================
class JsonDbReaderFailure : public std::exception
{
  private:
    std::string whatString;

  public:
    JsonDbReaderFailure(const char* func_, const char* file_, const int32_t line_, const std::filesystem::path& json_, const char* failure_)
    {
        std::ostringstream oss;
        oss << "In file \"" << file_ << "\" : " << func_ << "() (Line " << line_ << ")\n\t\"" << json_.generic_string() << ": " << failure_ << ".\"";
        whatString = oss.str();
    }

    [[nodiscard]] const char* what() const noexcept override { return whatString.c_str(); }
};

//============================================================================
//! \class JsonDbReader
//! \brief Parses JSON NovAtel UI Database files into MessageDatabase objects.
//============================================================================
class JsonDbReader
{
public:
    JsonDbReader() = delete;

    //----------------------------------------------------------------------------
    //! \brief Load a JSON DB from the provided file path.
    //
    //! \param[in] filePath_ The filepath to the Json file.
    //
    //! \return A shared pointer to the loaded MessageDatabase.
    //----------------------------------------------------------------------------
    static MessageDatabase::Ptr LoadFile(const std::filesystem::path& filePath_);

    //----------------------------------------------------------------------------
    //! \brief Parse database definitions from the provided JSON data.
    //
    //! \param[in] strJsonData_ A JSON string.
    //
    //! \return A shared pointer to the loaded MessageDatabase.
    //----------------------------------------------------------------------------
    static MessageDatabase::Ptr Parse(std::string_view strJsonData_);

    //----------------------------------------------------------------------------
    //! \brief Append messages from the provided JSON file to an existing MessageDatabase.
    //
    //! \param[in] messageDb_ The MessageDatabase to append to.
    //! \param[in] filePath_ The filepath to the JSON file.
    //----------------------------------------------------------------------------
    static void AppendMessages(const MessageDatabase::Ptr& messageDb_, const std::filesystem::path& filePath_);

    //----------------------------------------------------------------------------
    //! \brief Append enumerations from the provided JSON file to an existing MessageDatabase.
    //
    //! \param[in] messageDb_ The MessageDatabase to append to.
    //! \param[in] filePath_ The filepath to the Json file.
    //----------------------------------------------------------------------------
    static void AppendEnumerations(const MessageDatabase::Ptr& messageDb_, const std::filesystem::path& filePath_);
};

} // namespace novatel::edie

#endif
