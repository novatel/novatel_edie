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

/*! \file   composer.hpp
 *  \brief  Class to start conversion from ASCII to BINARY and vice vera process
 */

#pragma once
//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef COMPOSER_H
#define COMPOSER_H

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "hw_interface/stream_interface/api/outputstreaminterface.hpp"
#include "decoders/common/api/basemessagedata.hpp"
#include "decoders/common/api/common.hpp"
#include "binarytoasciicomposer.hpp"
#include "asciitobinarycomposer.hpp"

/*! \class Composer
 *  \brief Class has methods to convert messages from Ascii to Binary and vice versa.
 *  And also can write converted messgaes to output stream(file/port or buffer.
 *
 */
class Composer
{
public:
   /*! Default constructor */
   Composer();

   /*! \fn void ComposeData(BaseMessageData *pclBaseData, Format conversionType, CHAR* writeData)
    *  \brief Convert decoded message in given BaseMessageData object and store in writedata as per the given conversionType
    *  \param [in] pclBaseData BaseMessageData class pointer which has decoded message
    *  \param [in] conversionType Conversion type (Ascii/Binary)
    *  \param [in] writeData pointer which holds the converted message during conversion
    *  \param [out] pclBaseData BaseMessageData obejct pointer with converted message
    *
    *  \return Status of the conversion
    *  \sa MsgConvertStatusEnum
    */
   MsgConvertStatusEnum ComposeData(BaseMessageData *pclBaseData, Format conversionType, CHAR* writeData);

   /*! Default destructor */
   ~Composer();
private:
   /*! \var BinaryToAsciiComposer bBinaryToAscii
    * \brief BinaryToAsciiComposer class object used to convert message from binary to ascii
    * \sa BinaryToAsciiComposer
    */
   BinaryToAsciiComposer bBinaryToAscii;

   /*! \var AsciiToBinaryComposer pAsciiToBinary
    * \brief AsciiToBinaryComposer class object used to convert message from ascii to binary
    * \sa AsciiToBinaryComposer
    */
   AsciiToBinaryComposer pAsciiToBinary;

   /*! \var MsgConvertStatusEnum m_status
    * \brief Message convertion status enumaration variable
    * \sa MsgConvertStatusEnum
    */
   MsgConvertStatusEnum m_status;
};
#endif //COMPOSER_H
