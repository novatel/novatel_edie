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

#ifndef UNKNOWNDATAHANDLER_H
#define UNKNOWNDATAHANDLER_H

/*! \file unknowndatahandler.hpp
 *  \brief Class definition to provide the statistics of unknown data
 *  \author akhan
 *  \date FEB 2021
 *
 *  Class has methods to be find unknown data bytes from given buffer.
 *  Unknown data could be any kind of bytes which are not able to decoded.
 *  and also capture UnknownDataStatistics structure values.
 */

//-----------------------------------------------------------------------
// Includes                                                               
//-----------------------------------------------------------------------
#include "common.hpp"
#include "circularbuffer.hpp"

/*! \class UnknownDataHandler
    \brief A class contains unknown data handlers.
  
    A more detailed UnknownDataHandler class description.
    \author GPR
    \date   FEB 2021
*/
class UnknownDataHandler
{
private:
   UnknownDataStatistics stMyUnknownDataStatistics;  /**<Unknown data statistics structure*/
   CircularBuffer cMyCircularDataBuffer;             /**<Circular Buffer to hold data read from stream*/
   std::vector<std::string> aszValidCOMPromptsList;  /**<Pre defined Valid COM Promtps from Novatel Receiver*/

   /*! \fn void CheckUnknownByteType(UCHAR ucDataByte) 
       \brief Checks the input CHAR is unknown byte or not.
       \param [in] ucDataByte Byte to be checked for unknown or not?.
   */   
   void CheckUnknownByteType(UCHAR ucDataByte); 
public:
   /*! \brief UnknownDataHandler Class constructor.
    *
    *  All predefined recevier Valid COM ports will be emplace back to vector.
    */
   UnknownDataHandler();

   /*! \brief UnknownDataHandler Class destructor.
    *
    */
   ~UnknownDataHandler();

   /*! \fn void HandleUnknownDataBytes(CHAR* pucBuffer, UINT uiCount, BOOL bEOS)
       \brief Find unknown bytes from given string 
       \param [in] pucBuffer string in which unknown bytes will be catch.
       \param [in] uiCount Number of bytes in input buffer
       \param [in] bEOS Is End Of Stream/File reached with this buffer in the given stream.
   */
   void HandleUnknownDataBytes(CHAR* pucBuffer, UINT uiCount, BOOL bEOS);

   /*! \fn void HandleUnknownData(CHAR* pcMessageBuffer, MessageHeader* stMessageHeader, BOOL bEOS);       
       \brief Find unknown bytes from given string 
       \param [in] pcMessageBuffer Message pointer to parse a frame and store.
       \param [in] stMessageHeader Header structure to fill from handler
       \param [in] bEOS Is End Of Stream/File reached with this buffer in the given stream.
   */
   void HandleUnknownData(CHAR* pcMessageBuffer, MessageHeader* stMessageHeader, BOOL bEOS);       

   /*! \fn UnknownDataStatistics GetUnknownDataStatistics(void)
       \brief Returns Unknown Data statistics
       \return Unknown data statistics like Number of LF's, CR's, Unknown binary/Ascii bytes,
               Com ports, OK's, Valid COM ports and Invalid ASCII/BINARY Messages.
   */
   UnknownDataStatistics GetUnknownDataStatistics(void);

   /*! \fn void ResetUnknownDataStatistics(void)
       \brief Reset the all unknown data statistics structure,
              And clears circullar buffer which was used for catching unknown bytes.
   */
   void ResetUnknownDataStatistics();

   /*! \fn void Reset()
       \brief Reset the all unknown data statistics structure,
              And clears circullar buffer which was used for catching unknown bytes.
   */
   void Reset();
};
#endif // UNKNOWNDATAHANDLER_H
