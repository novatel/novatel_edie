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
// ! \file inputfilestream.hpp
// ===============================================================================

#ifndef INPUT_FILE_STREAM_HPP
#define INPUT_FILE_STREAM_HPP

#include "novatel_edie/stream_interface/filestream.hpp"

/*! \class InputFileStream
 *   \brief A Derived class will be used by decoder, if the decoded input is file.
 *
 *  Will also support file names with Wide characters.
 */
class InputFileStream
{
  public:
    /*! A Constructor
     *  \brief  Creates FileStream class object with wide character filename string.
     *
     *  \param[in] s32FileName_ Wide Character file name as Character pointer.
     *
     *  \remark If pwcFileName is NULL, then exception "Filename name not valid" will be thrown.
     */
    InputFileStream(const std::u32string& s32FileName_);

    /*! A Constructor
     *  \brief  Creates FileStream class object with filename string.
     *
     *  \param[in] pFileName file name as Character pointer.
     *
     *  \remark If pFileName is NULL, then exception "Filename name not valid" will be thrown.
     */
    InputFileStream(const char* pFileName);

    /*! Disabled Copy Constructor
     *
     *  A copy constructor is a member function which initializes an InputFileStream object
     *  using another object of the same class.
     */
    InputFileStream(const InputFileStream& clTemp_) = delete;

    /*! Disabled assignment operator
     *
     *  The copy assignment operator is called whenever selected by overload resolution,
     *  e.g. when an object appears on the left side of an assignment expression.
     */
    const InputFileStream& operator=(const InputFileStream& clTemp_) = delete;

    /*! FileStream pointer to hold created FileStream object in constructor */
    std::unique_ptr<FileStream> pInFileStream;

    /*! \brief Hold/copy decoded log and size of it in ReadDataStructure
     *
     *  \param[in] pReadDataStructure ReadDataStructure pointer to hold decoded log
     *  \return StreamReadStatus read data statistics
     */
    StreamReadStatus ReadData(ReadDataStructure& pReadDataStructure);
    /*! \brief Read one line from the file.
     *
     *  \param[in] szLine String pointer to hold one line dats read from the file.
     *  \return Returns Read statistics structure (StreamReadStatus)
     */
    StreamReadStatus ReadLine(std::string& szLine);

    /*! \brief Set/Reset File Position from which next read will be done.
     *
     *  \param[in] offset the position of the file pointer to read.
     *  \param[in] dir Seeking direction from beginning or end.
     *
     *  \remark After reset, the current file size will be changed accordingly.
     */
    void Reset(std::streamoff offset = 0, std::ios_base::seekdir dir = std::ios::beg);

    /*! \return the extension of the input file to be decoded.
     *
     *  \return  std::string - File Extension name
     */
    [[nodiscard]] std::string GetFileExtension() const;
    [[nodiscard]] std::string GetFileName() const;

    /*! \return the current file position from which next read will be done.
     *
     *  \return uint64_t  - File current offset
     */
    uint64_t GetCurrentFilePosition();

    /*! \brief Sets the current file offset. It could be read bytes so far.
     *
     *  \param[in] ullCurrentFileOffset Size of the data from one read size,
     *  Will be appended to calculate read bytes so far
     */
    void SetCurrentFileOffset(uint64_t ullCurrentFileOffset);

    /*! \return Current file offset.
     *
     *  \return Current file offset.
     */
    [[nodiscard]] uint64_t GetCurrentFileOffset() const;

  private:
    /*! \brief Provides file extension name.
     *
     *  \return Returns the file extension name.
     */
    [[nodiscard]] std::string FileExtension() const;

    /*! \brief Provides wide character file extension name.
     *
     *  \return Returns wide character file extension name.
     */
    [[nodiscard]] std::string WCFileExtension() const;

    /*! File with Wide character name.
     */
    std::wstring stwFileName;
    /*! Boolean variable to disable/enable wide char files.
     */
    bool bEnableWideCharSupport;
};

#endif
