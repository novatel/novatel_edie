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

/*! \file   encoder.hpp
 *  \brief Encoder will provide the functionality of converting the NovAtel message from one format to another format.
 *  It will check for the current message format, get the message definition from the json file,
 *  reframe the message into the required format with CRC and return it to the application.
 *
 */

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef ENCODER_H
#define ENCODER_H

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "decoders/common/api/env.hpp"
#include "decoders/common/api/basemessagedata.hpp"
#include "decoders/jsoninterface/api/message.hpp"
#include "decoders/jsoninterface/api/messageparams.hpp"
#include "decoders/jsoninterface/api/types.hpp"
#include "decoders/jsoninterface/api/enums.hpp"
#include "composer.hpp"

#include <vector>

/*! \class Encoder
 *  \brief Provides methods to check the current format of the message
 *  and proceed to conversion if the requested format is different than
 *  the current format.
 *
 */
class Encoder
{
public:
   /*! Default constructor */
   Encoder( );
   /*! Default destructor */
   ~Encoder();
   /*! \fn void WriteMessage(BaseMessageData *pclBaseData, Format conversionType)
    *  \brief Writes the converted message which comes with given BaseMessageData
    *
    *  \param [in] pclBaseData BaseMessageData object pointer which contains message to be converted
    *  \param [in] conversionType Conversion type, could be ASCII/BINARY
    * 
    *  \return Status of the conversion
    *  \sa MsgConvertStatusEnum
    */
   MsgConvertStatusEnum WriteMessage(BaseMessageData *pclBaseData, Format conversionType);
private:
   /*! \fn BOOL CheckForFormat(BaseMessageData *pclBaseData)
    *  \brief Checking the messgae format in BaseMessageData
    *
    *  \param [in] pclBaseData BaseMessageData object pointer which contains message
    *  \return 0
    */
   BOOL CheckForFormat(BaseMessageData *pclBaseData);

   /**< composer class object to convert data fom one format to another */
   Composer *pclComposer;

   /**< Converted data buffer to be wrriten to output file */
   CHAR* writeData;
};
#endif //ENCODER_H

