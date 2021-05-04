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

/*! \file   decoder.hpp
 *  \brief Interface class for applciation to decode
 */

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef DECODER_H
#define DECODER_H

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "framer.hpp"
#include "hw_interface/stream_interface/api/inputstreaminterface.hpp"
#include "hw_interface/stream_interface/api/inputfilestream.hpp"
#include "filters/messagedatafilter.hpp"
#include "decoders/jsoninterface/api/loaddatafromjson.hpp"
#include "decoders/jsoninterface/api/logutils.hpp"

/*! \class Decoder
 *  \brief A derived class from Framer
 *  \sa Framer
 *
 */
class Decoder : public Framer
{
private:
   /**< Class Pointer to hold LogUtils object used to decode input and get flatten message buffer and json string */
   LogUtils* pclMyLogUtils;

   /**< Class pointer to hold JSONFileReader object used to get the message definition */
   JSONFileReader *dbData;

public:
   /*! A constructor
    *  \brief Intializes parent Framer with Input Stream provided
    *  Gets the data base object(json object)
    *
    *  \param [in] pclInputStreamInterface Input stream interface(File/Port or buffer) used for intilaizing Framer class
    *  \sa Framer
    */
   Decoder(InputStreamInterface* pclInputStreamInterface);
   /*! A constructor with MessageDataFilter
    *  \brief Intializes parent Framer with Input Stream provided
    *  Gets the data base object(json object)
    *
    *  \param [in] pclInputStreamInterface Input stream interface(File/Port or buffer) used for intilaizing Framer class
    *  \param [in] rMessageDataFilter Message data filter object with which messages to be filtered out.
    *  \sa Framer
    *
    *  \remark MessageDataFilter object has not been handling in current version
    */
   Decoder(InputStreamInterface* pclInputStreamInterface, MessageDataFilter& rMessageDataFilter);

   /*! \fn void GenerateBaseMessageData(BaseMessageData** pclBaseMessageData,
                                        MessageHeader* pstMessageHeader,
                                        CHAR* pcData)
    *  \brief Method to generate BaseMessageData object with decoded message with the use of Framer.
    *
    *  \param [in] pclBaseMessageData BaseMessageData pointer to be generated
    *  \param [in] pstMessageHeader Decoded message statistics by Framer
    *  \param [in] pcData  Decoded message by Framer
    */
   void GenerateBaseMessageData(
      BaseMessageData** pclBaseMessageData, 
      MessageHeader* pstMessageHeader, 
      CHAR* pcData);
};
#endif // DECODER_H
