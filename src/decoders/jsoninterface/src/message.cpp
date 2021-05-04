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
//    Class to hold the message (Id,Name,MsgDef CRC etc) information
//
////////////////////////////////////////////////////////////////////////////////
#include "message.hpp"

std::map<std::string, std::list<std::string> >CMessage::listOfMsgsByCategory;
std::list<std::string>CMessage::listOfCommandMsgName;
std::list<std::string>CMessage::listOfMeaMsgName;
std::list<std::string>CMessage::listOfNavMsgName;
std::list<std::string>CMessage::listOfPosMsgName;
std::list<std::string>CMessage::listOfOtherMsgName;
std::map<std::string, std::string>CMessage::hiddenLogs;
std::map<UINT, std::string>CMessage::msgNameSta;

CMessage::CMessage()
   :m_iCategory(0),
   m_strStyle(""),
   uiMsgId(0),
   m_Category(INVALID),
   m_strMsgDefCrc(""),
   m_style(msgStyle(-1))
{

}

CMessage::~CMessage()
{

}

void CMessage::setMsgNameAndId(const std::string& strMsgName, UINT iMsgId)
{
   msgNameById.insert(std::pair<UINT, std::string>(iMsgId, strMsgName));
   msgIdByName.insert(std::pair<std::string, UINT>(strMsgName, iMsgId));
   msgNameSta.insert(std::pair<UINT, std::string>(iMsgId, strMsgName));
   m_strName = strMsgName;
}

void CMessage::setMsgId(UINT iMsgId)
{
   uiMsgId = iMsgId;
   m_lstMessageId.push_front(iMsgId);
}

void CMessage::setMsgStyle(const std::string& strName, const std::string &strStyle)
{
   msgStyleByMsgName.insert(std::pair<const std::string, const std::string>(strName, strStyle));
   if (m_strStyle != strStyle)
   {
      m_strStyle = strStyle;
   }
}

void CMessage::setEtherType(const std::string& strName, const std::string& strEtherType)
{
   etherTypeById.insert(std::pair<const std::string, const std::string>(strName, strEtherType));
}

void CMessage::setMsgDefCRC(const std::string& strMsgDefCRC)
{
   m_strMsgDefCrc = strMsgDefCRC;
}

void CMessage::setNoOfParams(UINT iParams, const std::string& strName)
{
   noOfParams.insert(std::pair<const std::string, UINT>(strName, iParams));
   noOfParametersById.insert(std::pair<UINT, UINT>(uiMsgId, iParams));
}

void CMessage::setHiddenStatus(const std::string &strName, const std::string &strStatus)
{
   hiddenLogs.insert(std::pair<std::string, std::string>(strName, strStatus));
   if (strStatus == "True")
   {
	   bhidden = 1;
   }
   else
   {
	   bhidden = 0;
   }
}

void CMessage::setCategory(UINT iCategory, UINT iMessageId)
{
   m_iCategory = iCategory;
   if (iCategory == POSITION)
   {
      listOfPosMsgName.push_back(msgNameById.find(iMessageId)->second);
      listOfMsgsByCategory.erase("Position");
      listOfMsgsByCategory.insert(std::pair<std::string, std::list<std::string> >("Position", listOfPosMsgName));
   }
   else if (iCategory == MEASUREMENT)
   {
      listOfMeaMsgName.push_back(msgNameById.find(iMessageId)->second);
      listOfMsgsByCategory.erase("Measurement");
      listOfMsgsByCategory.insert(std::pair<std::string, std::list<std::string> >("Measurement", listOfMeaMsgName));
   }
   else if (iCategory == NAVIGATION)
   {
      listOfNavMsgName.push_back(msgNameById.find(iMessageId)->second);
      listOfMsgsByCategory.erase("Navigation");
      listOfMsgsByCategory.insert(std::pair<std::string, std::list<std::string> >("Navigation", listOfNavMsgName));
   }
   else if (iCategory == COMMAND)
   {
      listOfCommandMsgName.push_back(msgNameById.find(iMessageId)->second);
      listOfMsgsByCategory.erase("Command");
      listOfMsgsByCategory.insert(std::pair<std::string, std::list<std::string> >("Command", listOfCommandMsgName));
   }
   else if (iCategory == OTHERS)
   {
      listOfOtherMsgName.push_back(msgNameById.find(iMessageId)->second);
      listOfMsgsByCategory.erase("Others");
      listOfMsgsByCategory.insert(std::pair<std::string, std::list<std::string> >("Others", listOfOtherMsgName));
   }
}

UINT CMessage::getNoOfParams(std::string strName) const
{
   return noOfParams.find(strName)->second;
}

std::string CMessage::getMsgDefCrc() const
{
   return m_strMsgDefCrc;
}

CMessage::msgCategory CMessage::getCategory() const
{
   return m_Category;
}

UINT CMessage::getCategoryById() const
{
   return m_iCategory;
}

std::string CMessage::getEtherType(std::string strName) const
{
   if (etherTypeById.size() > 0 && etherTypeById.count(strName))
   {
      return etherTypeById.find(strName)->second;
   }
   return "";
}

std::string CMessage::getMsgStyle() const
{
   return m_strStyle;
}

CMessage::msgStyle CMessage::getMsgStyle()
{
   return m_style;
}


void CMessage::setStyle(msgStyle style)
{
   m_style = style;
}

std::map<std::string, std::list<std::string> > CMessage::getCategoryLogsMap()
{
   return listOfMsgsByCategory;
}

std::map<std::string, std::string> CMessage::getHiddenLogsMap()
{
   return hiddenLogs;
}

UINT CMessage::getNoOfParamas(UINT uiMessageId)
{
   if (noOfParams.size() > 0 && noOfParams.count(getMessageName(uiMessageId)))
   {
      return noOfParams.find((getMessageName(uiMessageId)))->second;
   }	
   return 0;
}

UINT CMessage::getId() const
{
   return uiMsgId;
}

std::string CMessage::getName() const
{
   return m_strName;
}

BOOL CMessage::getHidden() const
{
	return bhidden;
}

UINT CMessage::getMessageId(std::string strMsgName)
{
   if (msgIdByName.size() > 0 && msgIdByName.count(strMsgName))
   {
      return msgIdByName.find(strMsgName)->second;
   }
   return 0;
}

std::string CMessage::getMessageName(UINT iMsgId)
{
   if (msgNameById.size() > 0 && msgNameById.count(iMsgId))
   {
      return msgNameById.find(iMsgId)->second;
   }
   return "";
}

UINT CMessage::getNoOfParamasById(UINT uiMessageId)
{
   if (noOfParametersById.size() > 0 && noOfParametersById.count(uiMessageId))
   {
      return noOfParametersById.find(uiMessageId)->second;
   }
   return 0;
}

std::string CMessage::getMsgName(UINT uiMsgId)
{
   if (msgNameSta.size() > 0 && msgNameSta.count(uiMsgId))
   {
      return msgNameSta.find(uiMsgId)->second;
   }
   return "";
}
