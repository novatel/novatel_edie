////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020 NovAtel Inc.
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
////////////////////////////////////////////////////////////////////////////////

/*! \file inputfilestream.hpp
 *  \brief It is a Derived class from main InputStream. Input to the decoder is file.
 * 
 */

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef INPUTFILESTREAM_HPP
#define INPUTFILESTREAM_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "inputstreaminterface.hpp"
#include "filestream.hpp"

/*! \class InputFileStream
 *   \brief A Derived class will be used by decoder, if the decoded input is file.
 * 
 *  Derived from base class InputStreamInterface. And will also supports
 *  File names with Wide characters.
*/
class InputFileStream : public InputStreamInterface
{
public:
#ifdef WIDE_CHAR_SUPPORT
    /*! A Constructor
	 *  \brief  Creates FileStream class object with wide character filename string.
	 * 
	 *  \param [in] pwFileName Wide Characer file name as Character pointer.
	 * 
	 *  \remark If pwcFileName is NULL, then exception "Filename name not valid" will be thrown.
	 */ 
   InputFileStream(const wchar_t* pwFileName);
#endif
    /*! A Constructor
	 *  \brief  Creates FileStream class object with filename string.
	 * 
	 *  \param [in] pFileName file name as Character pointer.
	 * 
	 *  \remark If pFileName is NULL, then exception "Filename name not valid" will be thrown.
	 */ 
   InputFileStream(const CHAR* pFileName);
   
   /*! A default destructor */
   virtual ~InputFileStream();

   /*! FileStream pointer to hold created FileStream object in constructor */
   FileStream* pInFileStream;

   /*! \fn StreamReadStatus ReadData(ReadDataStructure&)
    *  \brief Hold/copy decoded log and size of it in ReadDataStructure
    *   
    *  \param [in] pReadDataStructure ReadDataStructure pointer to hold decoded log
    *  \return StreamReadStatus read data statistics
    */ 
   StreamReadStatus ReadData(ReadDataStructure& pReadDataStructure);
   /*! \fn StreamReadStatus ReadLine
    *  \brief Read one line from the file.
	* 
    *  \param [in] szLine String pointer to hold one line dats read from the file.
    *  \return Returns Read statistics structure (StreamReadStatus)
    */   
   StreamReadStatus ReadLine(std::string& szLine);

   /*! \fn void Reset(std::streamoff = 0, std::ios_base::seekdir = std::ios::beg)
    *  \brief Set/Reset File Position from which next read will be done.
	 * 
	 *  \param [in] offset the position of the file pointer to read.
	 *  \param [in] dir Seeking direction from begining or end.
    *
	 *  \remark After reset, the current file size will be changed accrdingly. 
    */
   void Reset(std::streamoff offset = 0, std::ios_base::seekdir dir = std::ios::beg);

   /*! \n std::string GetFileExtension()
    *  \brief Returns the extension of the input file to be decoded.
    * 
    *  \return  std::string - File Extension name 
    */ 
   std::string GetFileExtension();

   /*! \n ULONGLONG GetCurrentFilePosition()
    *  \brief Returns the current file position from which next read will be done.
    * 
    *  \return ULONGLONG - File current offset
    */ 
   ULONGLONG GetCurrentFilePosition();

   /*! \fn void SetCurrentFileOffset(ULONGLONG ullCurrentFileOffset)
    *  \brief Sets the current file offset. It could be read bytes so far.
	 * 
    *  \param [in] ullCurrentFileOffset Size of the data from one read size, 
	 *  Will be append to calculate read bytes so far
    */
   void SetCurrentFileOffset(ULONGLONG ullCurrentFileOffset);

   /*! \fn ULONGLONG GetCurrentFileOffset(void)
    *  \brief Returns Cuurent file offset.
	 *  
    *  \return Cuurent file offset. 
    */    
   ULONGLONG GetCurrentFileOffset(void) const;

private:
	/*! Private Copy Constructor 
	 *
	 *  A copy constructor is a member function which initializes an InputFileStream object 
    *  using another object of the same class. 
	 */
   InputFileStream(const InputFileStream& clTemp);

	/*! Private assignment operator 
	 *
	 *  The copy assignment operator is called whenever selected by overload resolution, 
	 *  e.g. when an object appears on the left side of an assignment expression.
	 */ 
   const InputFileStream& operator= (const InputFileStream& clTemp);

   /*! \fn std::string FileExtension()
    *  \brief Provides file extension name.
    * 
    *  \return Returns the file extension name.
    */ 
   std::string FileExtension();
#ifdef WIDE_CHAR_SUPPORT
   /*! \fn std::string WCFileExtension()
    *  \brief Provides wide character file extension name. 
    * 
    *  \return Returns wide character file extension name.
    */ 
   std::string WCFileExtension();
#endif

#ifdef WIDE_CHAR_SUPPORT
   /*! \var stwFileName
    *
    *  File with Wide charater name.
    */ 
   std::wstring stwFileName;
   /*! \var bEnableWideCharSupport
    *
    *  Boolean variable to disable/enable wide char files.
    */ 
   BOOL bEnableWideCharSupport;
#endif
   /*! \var stFileName
    *
    *  File name.
    */ 
   std::string stFileName;   
};

#endif
