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

#ifndef MESSAGECOUNTER_H
#define MESSAGECOUNTER_H

/*! \file messagecounter.hpp
 *  \brief Used for decoder message statistics
 * 
 *  \author akhan
 *  \date   FEB 2021 
 * 
 *  Will be used to store message details with message name and 
 *  Aadding new message to counter, Updating the existing message stats,
 *  count number of total messages, messate statistics of decoded messsages,  
 *  Examle: Number of ASCII/BINARY messsages, And also able to filer based on message while counting stats.
 */ 

//-----------------------------------------------------------------------
// Includes                                                               
//-----------------------------------------------------------------------
#include "env.hpp"
#include "common.hpp"
#include <map>

/*! \brief Used for decoder message statistics
 *
 *  Will be used to store message details with message name and 
 *  Aadding new message to counter, Updating the existing message stats,
 *  count number of total messages, messate statistics of decoded messsages,  
 *  Examle: Number of ASCII/BINARY messsages, And also able to filer based on message while counting stats.
 */ 
class MessageCounter
{
private:
   /*! \var mAsciiMessageCountMap
    *  \brief Map object to store Ascii Message details with Message name as Key and Message details struct as a value with filtering.
    * 
    */
   std::map<std::string, MessageInfo> mAsciiMessageCountMap;
   
   /*! \var mBinaryMessageCountMap
    *  \brief Map object to store Binary Message details with Message name as Key and Message details struct as a value with filtering.
    * 
    */  
   std::map<UINT, MessageInfo> mBinaryMessageCountMap;  

   /*! \var mTotalAsciiMessagesMap
    *  \brief Map object to store Ascii Message details with Message name as Key and Message details struct as a value without filtering. 
    * 
    */
   std::map<std::string, MessageInfo> mTotalAsciiMessagesMap;
   
   /*! \var mTotalBinaryMessagesMap
    *  \brief Map object to store Binary Message details with Message name as Key and Message details struct as a value without filtering. 
    * 
    */
   std::map<UINT, MessageInfo> mTotalBinaryMessagesMap;

   /*! A Structure.
    *
    *  Log Decoder statistics structure of total messages, 
    *  Start/End Week & Weekseconds.
    */
   DecoderStatistics stMyDecoderStatistics;

   /*! \var bMyEnableTimeIssueFix
    *  \brief Bollean Variable to enable/Disable Time Issue Fix.
    *  \remarks If enable, don't consider IONUTC and QZSSIONUTC messages to update start and end times and return from here 
    */ 
   BOOL bMyEnableTimeIssueFix;

   /*! \brief Method to Update existing message statistics in counter
    *
    *  First find the incoming message in map. And If not found return FALSE.
    *  If Message exists already in map, then increment the statistics by 1.
    * 
    *  \pre None
    *  \post None
    * 
    *  \param[in] stMessageHeader MessageHeader structure with details to be updated.  
    * 
    *  \return None.
    *  \remark None.
    */ 
   BOOL UpdateMessage(MessageHeader* stMessageHeader);

public:

   /*! \brief MessageCounter Class constructor. 
    *
    * By default Time issue fix will not enabled. If enabled, don't consider 
    * IONUTC and QZSSIONUTC messages to update start and times and return from here.
    */
   MessageCounter();

   /*! \brief MessageCounter Class destructor. */
   ~MessageCounter();
   
   /*! \brief Method to add new message to counter
    *
    *  Incoming Message with MessageHeader argument will be added to the counter.
    *  Will not be added/igonred statistics for messages which are Unknown,
    *  With week 0 to update start and end times and if time fix is enabled, 
    *  don't consider IONUTC and QZSSIONUTC messages.
    * 
    *  \pre None
    *  \post None
    * 
    *  \param[in] stMessageHeader MessageHeader structure with details to be added.  
    * 
    *  \return None.
    *  \remark None.
    */ 
   void AddNewMessage(MessageHeader* stMessageHeader);    

   /*! \brief Method to add message to counter before filtering
    *
    *  If Message is not find/added already to the counter, Then count Will start from 1.
    *  If Message exists in the counter map, increment it's count by 1.
    * 
    *  \pre None
    *  \post None
    * 
    *  \param[in] stMessageHeader MessageHeader structure with details to be added.  
    * 
    *  \return None.
    *  \remark None.
    */ 
   void CountMessage(MessageHeader* stMessageHeader);    

   /*! \brief Method to return Ascii Message statistics map object.
    *
    */
   std::map<std::string, MessageInfo>
   GetAsciiMessageStatistics();

   /*! \brief Method to return Binary Message statistics map object.
    *
    */
   std::map<UINT, MessageInfo>
   GetBinaryMessageStatistics();

   /*! \brief Method to return Ascii Message statistics map object. 
    *
    */
   std::map<std::string, MessageInfo>
   GetAsciiMessageStatisticsWithoutFilter(void) const;
   
   /*! \brief Method to return Binary Message statistics map object. 
    *
    */
   std::map<UINT, MessageInfo>
   GetBinaryMessageStatisticsWithoutFilter(void) const;
   /*! \brief Method to return decoder statistics. 
    *
    */
   DecoderStatistics
   GetDecoderStatistics(void) const;

   /*! \brief Method to enable time issue fix. 
    *
    */
   void EnableTimeIssueFix(BOOL bEnable);

   /*!  \brief Method to Reset all maps and statistics of this class. 
    *
    *  Resets the Decoder statistics Structure
    *  Resets All Maps which has message statistics.
    */ 
   void Reset(); 
};

#endif //MESSAGECOUNTER_H
