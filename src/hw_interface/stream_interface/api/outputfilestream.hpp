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

/*! \file outputfilestream.hpp
 *  \brief Class definition to provide basic file operations for writing output files.
 * 
 *  Class has defined in a such a way that will provide basic file operations writing data to it, 
 *  Will support, Open/Close/Write/Read ... operations etc.
 */ 

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef OUTPUTFILESTREAM_HPP
#define OUTPUTFILESTREAM_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "outputstreaminterface.hpp"
#include "filestream.hpp"
#include "decoders/novatel/api/filters/messagedatafilter.hpp"

/*! \class OutputFileStream
 *  \brief A Derived class from parent interface class OutputStreamInterface.
 * 
 *  More detailed FileStream class description. It will also supports
 *  File names with Wide characters.
*/
class OutputFileStream : public OutputStreamInterface
{
public:
#ifdef WIDE_CHAR_SUPPORT
    /*! A Constructor.
	 *  \brief  Creates FileStream Object with wide character file name for writing output. 
    *  And intializes MessageDataFilter object to NULL.
	 *  \param [in] pcFileName Output file name with wide characters.
	 * 
	 */   
   OutputFileStream(const wchar_t*);

    /*! A Constructor.
	 *  \brief  Creates FileStream Object  with wide character file name for wrtiing output.
    *   And intializes MessageDataFilter object with provided.
	 *  \param [in] pcFileName Output file name with wide characters.
	 *  \param [in] rMessageDataFilter Object used for filtering output.
	 * 
	 */   
   OutputFileStream(const wchar_t*, MessageDataFilter&);
#endif
    /*! A Constructor.
	 *  \brief  Creates FileStream Object for writing output. And intializes MessageDataFilter object to NULL.
	 *  \param [in] pcFileName Output file name.
	 * 
	 */    
   OutputFileStream(const CHAR* pcFileName);
    /*! A Constructor.
	 *  \brief  Creates FileStream Object for wrtiing output. And intializes MessageDataFilter object with provided.
	 *  \param [in] pcFileName Output file name.
	 *  \param [in] rMessageDataFilter Object used for filtering output.
	 * 
	 */      
   OutputFileStream(const CHAR* pcFileName, MessageDataFilter& rMessageDataFilter);

    /*! A virtual destructor.
	 *  \brief  Clears MessageDataFilter and FileStream objects.
	 * 
	 */ 
   virtual ~OutputFileStream();

   /*! FileStream Class object.
    * \sa FileStream
    */ 
   FileStream* pOutFileStream;

   /*! \fn UINT WriteData(BaseMessageData&)
    *  \brief Write data to output file.
    *  \param [in] pclBaseMessageData BaseMessageData object.
    *  \sa BaseMessageData
    *  \return Number of bytes written to output file.
    *  \remark Set Split type and write data to output files. If split type was not set,
    *  Then writing can be done to only one file. 
    */ 
   UINT WriteData(BaseMessageData& pclBaseMessageData);

private:
	/*! Private Copy Constructor 
	 *
	 *  A copy constructor is a member function which initializes an object using another object of the same class. 
	 */
   OutputFileStream(const OutputFileStream& clTemp);

	/*! Private assignment operator 
	 *
	 *  The copy assignment operator is called whenever selected by overload resolution, 
	 *  e.g. when an object appears on the left side of an assignment expression.
	 */
   const OutputFileStream& operator= (const OutputFileStream& clTemp);

   /*! MessageDataFilter Class object.
    * \sa MessageDataFilter
    */ 
   MessageDataFilter* pMessageDataFilter;
};

#endif
