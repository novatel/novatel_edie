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
//
//  DESCRIPTION:
//    Class to provide unknwon data statistics
//
////////////////////////////////////////////////////////////////////////////////
#include "unknowndatahandler.hpp"
#include <algorithm>
#include <string.h>
#include <cstring>

// -------------------------------------------------------------------------------------------------------
UnknownDataHandler::UnknownDataHandler()
{
   // Belo vector members are taken from compoenent_userinterface.cpp to check for valid com prompt
   // if any other new com prompts are added in firmware, need to be added here as well.
   // this list can be refined

   aszValidCOMPromptsList.push_back("COM1");
   aszValidCOMPromptsList.push_back("COM2");
   aszValidCOMPromptsList.push_back("COM3");
   aszValidCOMPromptsList.push_back("COM4");
   aszValidCOMPromptsList.push_back("COM5");
   aszValidCOMPromptsList.push_back("COM6");
   aszValidCOMPromptsList.push_back("COM7");
   aszValidCOMPromptsList.push_back("COM8");
   aszValidCOMPromptsList.push_back("COM9");
   aszValidCOMPromptsList.push_back("COM10");
   aszValidCOMPromptsList.push_back("CCOM1");
   aszValidCOMPromptsList.push_back("CCOM2");
   aszValidCOMPromptsList.push_back("CCOM3");
   aszValidCOMPromptsList.push_back("CCOM4");
   aszValidCOMPromptsList.push_back("CCOM5");
   aszValidCOMPromptsList.push_back("CCOM6");
   aszValidCOMPromptsList.push_back("CCOM7");
   aszValidCOMPromptsList.push_back("CCOM8");
   aszValidCOMPromptsList.push_back("ICOM1");
   aszValidCOMPromptsList.push_back("ICOM2");
   aszValidCOMPromptsList.push_back("ICOM3");
   aszValidCOMPromptsList.push_back("ICOM4");
   aszValidCOMPromptsList.push_back("ICOM5");
   aszValidCOMPromptsList.push_back("ICOM6");
   aszValidCOMPromptsList.push_back("ICOM7");
   aszValidCOMPromptsList.push_back("SCOM1");
   aszValidCOMPromptsList.push_back("SCOM2");
   aszValidCOMPromptsList.push_back("SCOM3");
   aszValidCOMPromptsList.push_back("SCOM4");
   aszValidCOMPromptsList.push_back("NCOM1");
   aszValidCOMPromptsList.push_back("NCOM2");
   aszValidCOMPromptsList.push_back("NCOM3");
   aszValidCOMPromptsList.push_back("USB1");
   aszValidCOMPromptsList.push_back("USB2");
   aszValidCOMPromptsList.push_back("USB3");
   aszValidCOMPromptsList.push_back("XCOM1");
   aszValidCOMPromptsList.push_back("XCOM2");
   aszValidCOMPromptsList.push_back("XCOM3");
   aszValidCOMPromptsList.push_back("ACK");
   aszValidCOMPromptsList.push_back("FILE");
   aszValidCOMPromptsList.push_back("AUX");
   aszValidCOMPromptsList.push_back("WCOM1");
}

UnknownDataHandler::~UnknownDataHandler()
{
}

// Check for unknownbyte is ascii or binary
void UnknownDataHandler::CheckUnknownByteType(UCHAR ucDataByte)
{
   if (isprint(ucDataByte))
      stMyUnknownDataStatistics.ulUnknownAsciiBytes++;
   else
      stMyUnknownDataStatistics.ulUnknownBinaryBytes++;
}

// check the received com prompt is in array list or not
BOOL in_array(const std::string &value, const std::vector<std::string> &array)
{
   return std::find(array.begin(), array.end(), value) != array.end();
}

// Receives string and length of it along with End Of Stream bit
// Process each byte and set statistics accordingly
void UnknownDataHandler::HandleUnknownDataBytes(CHAR* pucBuffer, UINT uiCount, BOOL bEOS)
{
   UINT uiIndex = 0;
   CHAR* pucPtr;
   CHAR ucPortInfo[6];
   std::string pucBuffer_;

   cMyCircularDataBuffer.Append((UCHAR*)pucBuffer, uiCount);
   for (UINT uiPos = 0; uiPos < cMyCircularDataBuffer.GetLength(); uiPos++)
      pucBuffer_ += cMyCircularDataBuffer[uiPos];

   while (pucBuffer_.size() - uiIndex)
   {
      UCHAR ucDataByte = (UCHAR)pucBuffer_[uiIndex];

      if (ucDataByte == LF)
      {
         stMyUnknownDataStatistics.ulLineFeeds++;
      }
      else if (ucDataByte == CR)
      {
         stMyUnknownDataStatistics.ulCarriageReturns++;
      }
      else if (ucDataByte == '<')
      {
         if ((pucBuffer_.size() - uiIndex) > 2)
         {
            if (pucBuffer_[uiIndex + 1] == 'O' && pucBuffer_[uiIndex + 2] == 'K')
            {
               stMyUnknownDataStatistics.ulOKPrompts++;
               uiIndex += 2;
            }
            else
               CheckUnknownByteType(ucDataByte);
         }
         else
         {
            // if End Of file, process remainig bytes
            // if Not End Of file, should be append with next stream
            if (bEOS != FALSE)
               CheckUnknownByteType(ucDataByte);
            else //
               break;
         }
      }
      else if (ucDataByte == '[')
      {
         pucPtr = (CHAR*)std::memchr(&(pucBuffer_.at(uiIndex)), ']', pucBuffer_.size() - uiIndex);
         if (pucPtr)
         {
            UINT uiSize = static_cast<UINT>(pucPtr - &(pucBuffer_.at(uiIndex)));
            if (uiSize <= 6) // Max size of COM prompt will be 6 including end char ']'
            {
               memset(ucPortInfo, '\0', sizeof(ucPortInfo));
               memcpy(ucPortInfo, &(pucBuffer_.at(uiIndex + 1)), uiSize - 1);
               std::string strPort(ucPortInfo);
               // Checking the received com prompt is valid or not with predefined array of list above
               // If Valid, Increment ValidCOMPortBytes to size of COM PORT in the stream(including '[' & ']')
               // If not valid, added these bytes to unknown bytes of ASCII.
               if (!in_array(strPort, aszValidCOMPromptsList))
               {
                  stMyUnknownDataStatistics.ulInvalidCOMPorts++;
                  CheckUnknownByteType(ucDataByte);
               }
               else
               {
                  stMyUnknownDataStatistics.ulCOMPorts++;
                  stMyUnknownDataStatistics.ulValidCOMPortBytes += uiSize+1;
                  uiIndex += uiSize;
               }
            }
            else
            {
               CheckUnknownByteType(ucDataByte);
            }
         }
         else
         {
            // if End Of file, process remainig bytes
            // if remaining of the buffer is >= 6, consider the byte as not valid
            // if Not End Of file, should be append with next stream
            if ((bEOS != FALSE) || ((pucBuffer_.size() - uiIndex) >= 6))
               CheckUnknownByteType(ucDataByte);
            else
               break;
         }
      }
      else
      {
         CheckUnknownByteType(ucDataByte);
      }
      uiIndex++;
   }

   cMyCircularDataBuffer.Discard(uiIndex);
   pucBuffer_.clear();
}


// API called fomr standard decoder
void UnknownDataHandler::HandleUnknownData(CHAR* pcMessageBuffer, MessageHeader* stMessageHeader, BOOL bEOS)
{
   // If length is zero, simple return
   if (!stMessageHeader->uiMessageLength)
      return;

   // If Incoming buffer invalid ASCII message
   if (stMessageHeader->eMessageFormat == MESSAGE_ASCII)
   {
      stMyUnknownDataStatistics.ulInvalidAsciiMsgs++;
   }
   else if (stMessageHeader->eMessageFormat == MESSAGE_BINARY)
   {
      // If Incoming buffer invalid BINARY message
      stMyUnknownDataStatistics.ulInvalidBinaryMsgs++;
   }
   else
   {
      // If not above, parse for LF, CR, <OK and COM Promts
      HandleUnknownDataBytes(pcMessageBuffer, stMessageHeader->uiMessageLength, bEOS);
   }
}

// Return statistics of unknown data for application
UnknownDataStatistics UnknownDataHandler::GetUnknownDataStatistics()
{
   return stMyUnknownDataStatistics;
}

// Reset UnknownDataStatistics and clear circual buffer
void UnknownDataHandler::ResetUnknownDataStatistics()
{
   memset(&stMyUnknownDataStatistics, 0x00, sizeof(stMyUnknownDataStatistics));
   cMyCircularDataBuffer.Clear();
}

//----------------------------------------------------
void UnknownDataHandler::Reset()
{
   ResetUnknownDataStatistics();
}
