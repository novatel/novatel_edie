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

/*! \file outputstreaminterface.hpp
 *  \brief An interface class for writing data to the output.
 * 
 */ 

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef OUTPUTSTREAMINTERFACE_HPP
#define OUTPUTSTREAMINTERFACE_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "decoders/common/api/common.hpp"
#include "decoders/common/api/basemessagedata.hpp"

//-----------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------
class MemoryStream;


/*! \class OutputStreamInterface
 *  \brief An Interface Class used by application to write dat to the output.
 * 
 *  \details Output stream is an interface for writing data to file and port handles. 
 *  Output Stream has three derived classes to support file and port access, OutputFileStream, OutputPortStream and MultiOutputFileStream. 
 *  The ideal usage of Output Stream is to support Encoder to write the converted data to given stream object. 
*/
class OutputStreamInterface
{
public:
   /** A virtual member.
    *
    * \sa WriteData
    *  \param [in] pclBaseMessageData BaseMessageData object.
    *  \sa BaseMessageData
    *  \return Number of bytes written to output file.
    *  \remark Set Split type and write data to output files. If split type was not set,
    *  Then writing can be done to only one file. 
    */ 
   virtual UINT WriteData(BaseMessageData& pclBaseMessageData) = 0;

   /*! a virtual destructor. */
   virtual ~OutputStreamInterface(){};

   /** A virtual member.
    *  \sa SelectFileStream
    *  \brief Sets the output file in which to be decoded ouput will be written.
    *  \param [in] stFileName - Name of the File to be created.
    *  \sa FileStream
    *  \remark FileStream Object will be created and added to map. 
    *  If already created, The file with name stFileName will be set for writing.
    */ 
   virtual void SelectFileStream(std::string stFileName){};

   /** A virtual member.
    *  \sa ConfigureSplitByLog
    *  \brief Enalbe/Diable Splitting of logs into different output files.
    *  \param [in] bStatus
    *  \remark Enable/Disable log Splitting. If enable, split type will be set to SPLIT_LOG
    *  If disabled split type will be set to SPLIT_NONE
    */   
   virtual void ConfigureSplitByLog(BOOL){};

   /** A virtual member.
    *  \sa ConfigureBaseFileName
    *  \brief Gets base file name and extension of it.
    *  \param [in] stFileName - Name of the File.
    *  \remark Sets Base file name (before '.' in file name)
    *          Sets Extension of the file.
    */     
   virtual void ConfigureBaseFileName(std::string stFileName){};

   /** A virtual member.
    *  \sa ConfigureSplitBySize
    *  \brief Split file into different output file with defined size.
    *  \param [in] ullFileSplitSize
    *  \remark Output files with ullFileSplitSize size will be created while writing to the output.
    */   
   virtual void ConfigureSplitBySize(ULONGLONG){};

    /** A virtual member.
    *  \sa ConfigureSplitByTime
    *  \brief Sets the interval of time the file to be splitted.
    *  \param [in] dFileSplitTime 
    *  \remark Diffrent output files will be created with the logs,
    *  in which will be captured in the tiem interval provided.
    */
    virtual void ConfigureSplitByTime(DOUBLE){};

    /** A virtual member.
      * \sa MemoryStream
      * \return MemoryStream Obejct which had buffer.
      */    
   virtual MemoryStream* GetMemoryStream(){return NULL;};

   #ifdef WIDE_CHAR_SUPPORT
   /** A virtual member.
    *  \sa SelectFileStream
    *  \brief Sets the output file in which to be decoded ouput will be written.
    *  \param [in] stFileName - Wide character Name of the File to be created.
    *  \sa FileStream
    *  \remark FileStream Object will be created and added to map. 
    *  If already created, The file with name stFileName will be set for writing.
    */    
   virtual void SelectFileStream(std::wstring){};
   
   /** A virtual member.
    *  \sa ConfigureBaseFileName
    *  \brief Gets wide character base file name and extension of it.
    *  \param [in] stFileName - Name of the File with wide characters.
    *  \remark Sets Base file name (before '.' in file name)
    *          Sets Extension of the file.
    */          
      virtual void ConfigureBaseFileName(std::wstring){};
   #endif

private:
};

#endif

