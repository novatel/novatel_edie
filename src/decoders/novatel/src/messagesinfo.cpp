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
//
//This File is used to get Message ID and Names of Messsages
//
////////////////////////////////////////////////////////////////////////////////


#include "messagesinfo.hpp"

//---------------------------------------------------------------------------------
INT GetMessageIDByName(std::string& messageName)
{
   JSONFileReader *dbData;
   INT MessageId;
   dbData = loadDataFromJSON::getDatabaseObj();
   if(dbData!= NULL)
   {
      MessageId = dbData->getMessageId(messageName);
	  if (MessageId != -1)
		  return MessageId;
	  else
		  return 0;
      
   }
   
   return 0;
}
//----------------------------------------------------------------------------------
std::string GetMessageNameByID(INT messageID)
{
   JSONFileReader *dbData;
   dbData = loadDataFromJSON::getDatabaseObj();
   std::string szMessageName;
   dbData = loadDataFromJSON::getDatabaseObj();
   if(dbData!= NULL)
   {
      szMessageName = dbData->getMessageName(messageID);
      return szMessageName.substr(0, szMessageName.find("_"));
      
   }
   return "UNKNOWN";
}
//---------------------------------------------------------------------------------
