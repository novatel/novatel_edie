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
// ! \file file_parser.hpp
// ===============================================================================

#pragma once

#include "novatel_edie/decoders/common/file_parser.hpp"
#include "novatel_edie/decoders/oem/parser.hpp"

namespace novatel::edie::oem {

//============================================================================
//! \class FileParser
//! \brief Frame, decode and re-encode OEM logs from an InputFileStream.
//============================================================================
class FileParser : public FileParserBase<oem::Parser, oem::Filter, oem::MetaDataStruct, oem::IntermediateHeader>
{
    using Base = FileParserBase<oem::Parser, oem::Filter, oem::MetaDataStruct, oem::IntermediateHeader>;

  public:
    //! NOTE: The following constructors prevent this class from ever being
    //! constructed from a copy, move or assignment.
    FileParser(const FileParser&) = delete;
    FileParser(FileParser&&) = delete;
    FileParser& operator=(const FileParser&) = delete;
    FileParser& operator=(FileParser&&) = delete;

    //----------------------------------------------------------------------------
    //! \brief A constructor for the FileParser class.
    //
    //! \param[in] sDbPath_ Filepath to a JSON message DB.
    //----------------------------------------------------------------------------
    FileParser(const std::filesystem::path& sDbPath_) : Base("novatel_file_parser", sDbPath_) { pclMyLogger->debug("FileParser initialized"); }

    //----------------------------------------------------------------------------
    //! \brief A constructor for the FileParser class.
    //
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object. Defaults to nullptr.
    //----------------------------------------------------------------------------
    FileParser(const MessageDatabase::Ptr& pclMessageDb_ = {nullptr}) : Base("novatel_file_parser", pclMessageDb_)
    {
        pclMyLogger->debug("FileParser initialized");
    }

    //----------------------------------------------------------------------------
    //! \brief Set the abbreviated ASCII response option.
    //
    //! \param[in] bIgnoreAbbreviatedAsciiResponses_ true to ignore abbreviated
    //! ASCII responses.
    //----------------------------------------------------------------------------
    void SetIgnoreAbbreviatedAsciiResponses(bool bIgnoreAbbreviatedAsciiResponses_)
    {
        clMyParser.SetIgnoreAbbreviatedAsciiResponses(bIgnoreAbbreviatedAsciiResponses_);
    }

    //----------------------------------------------------------------------------
    //! \brief Get the abbreviated ASCII response option.
    //
    //! \return The current option for ignoring abbreviated ASCII responses.
    //----------------------------------------------------------------------------
    [[nodiscard]] bool GetIgnoreAbbreviatedAsciiResponses() const { return clMyParser.GetIgnoreAbbreviatedAsciiResponses(); }
};

} // namespace novatel::edie::oem
