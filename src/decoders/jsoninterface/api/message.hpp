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

/*! \file message.hpp
 *  \brief  Class to hold the message information(Id,Name,MsgDef CRC etc)
 *
 */

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef CMESSAGE_H
#define CMESSAGE_H

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include <string>
#include <map>
#include <list>

#include "decoders/common/api/env.hpp"


/*! \class CMessage
 *  \brief Class with methods to Set/Get Message information(Id,Name,MsgDef CRC etc)
 * 
 */
class CMessage
{
public:
   /*! A constructor
    * \brief Initializes the message information category with "INVALID", style with "-1", 
	* id "0" and CRC definition with "".
	*/ 
   CMessage();
   /*! Default destructor */
   ~CMessage();

   /*! An enumaraion
    * Enumaration values for message style identification
	*/ 
   enum msgStyle{
      OEM4_MESSAGE_STYLE,        /*!< OEM4 messages*/
      OEM3_NOTIME_MESSAGE_STYLE, /*!< OEM3 no time messages */
      OEM3_TIME_MESSAGE_STYLE,   /*!< OEM3 time messages */
      RTCA_STYLE,                /*!< RTCA messages */
      RTCM_STYLE,                /*!< RTCM messages */
      OEM4_COMPRESSED_STYLE,     /*!< OEM4 compressed messages */
      NOVATELX_MESSAGE_STYLE     /*!< NOVATELX messages */
   };
   
   /*! An enumaraion
    * Enumaration values for message category
	*/    
   enum msgCategory{
      INVALID = -1,    /*!< Invalid messages*/
      POSITION = 0,    /*!< Positioning engine messages*/
      MEASUREMENT = 1, /*!< Measurement engine messages*/
      NAVIGATION = 2,  /*!< Navigation engine messages*/  
      COMMAND = 3,     /*!< Command messages*/ 
      OTHERS = 4       /*!< Other than messages*/ 
   };

   /*! \fn void setMsgNameAndId(const std::string& strMsgName, UINT iMsgId)
    * \brief Store Message name and ID as key-value pair 
	* And also ID and name as key-value pair and store Message names with message id as key
	* \param [in] strMsgName Message name 
	* \param [in] iMsgId Message ID value
	*/ 
   void setMsgNameAndId(const std::string& strMsgName, UINT iMsgId);

   /*! \fn void setEtherType(const std::string& strMsgName, const std::string& strEtherType)
    * \brief Store message name and ether type as key-value pair
	* \param [in] strMsgName Message name string
	* \param [in] strEtherType Message Ether type string
	*/
   void setEtherType(const std::string& strMsgName, const std::string& strEtherType);

   /*! \fn void setMsgDefCRC(const std::string& iMsgDefCRC)
    * \brief Store CRC definitino of the message
	* \param [in] iMsgDefCRC CRC definition of the message
	*/ 
   void setMsgDefCRC(const std::string& iMsgDefCRC);

    /*! \fn void setNoOfParams(UINT iParams, const std::string& strName)
    * \brief Store Number of parameters of a message in a list with message name as key
	* Store Number of parameters of a message in a list with message id as key
	* \param [in] iParams Number of parameters in a message
	* \param [in] strName Message name
	*/   
   void setNoOfParams(UINT iParams, const std::string& strName);

    /*! \fn void setMsgId(UINT iMsgId)
    * \brief Store message in a list 
	* \param [in] iMsgId Message ID
	*/ 
   void setMsgId(UINT iMsgId);

   /*! \fn void setMsgStyle(const std::string& strName, const std::string& strStyle)
    * \brief Store message style in a list with message name as key
	* \param [in] strName Message name
	* \param [in] strStyle Message style
	*/
   void setMsgStyle(const std::string& strName, const std::string& strStyle);

   /*! \fn void setHiddenStatus(const std::string& strName, const std::string& strStatus)
    * \brief Store hidden log status in a list with message name as key
	* \param [in] strName Hidden log name
	* \param [in] strStatus Hidden log status string
	*/   
   void setHiddenStatus(const std::string& strName, const std::string& strStatus);

   /*! \fn UINT getMessageId(std::string strMsgName)
    * \brief Method to get message ID from Message name
	* \param [in] strMsgName Message name
	* \return Message ID
	*/
   UINT getMessageId(std::string strMsgName);

   /*! \fn std::string getMessageName(UINT iMsgId)
    * \brief Method to get message name from message ID
	* \param [in] iMsgId Message ID
	* \return Message name
	*/
   std::string getMessageName(UINT iMsgId);

   /*! \fn void setCategory(UINT iCategory, UINT iMessageId)
    * \brief Method to set message category with message ID as key
	* \param [in] iCategory Message category
	* \param [in] iMessageId Message ID
	*/   
   void setCategory(UINT iCategory, UINT iMessageId);

   /*! \fn UINT getNoOfParams(std::string strName) const
    * \brief Method to get number of parameters in a message 
	* \param [in] strName Message name
	* \return Number of parameters of the message
	*/ 
   UINT getNoOfParams(std::string strName) const;

   /*! \fn std::string getMsgDefCrc() const
    * \brief Method to get Message def CRC of the message 
	* \return Message definition CRC string
	*/
   std::string getMsgDefCrc() const;

   /*! \fn msgCategory getCategory() const
    * \brief Method to get Message category from message name
	* \return Message category enumaration
	*/
   msgCategory getCategory() const;

   /*! \fn UINT getCategoryById()const
    * \brief Method to get Message category from message id
	* \return Message category integer value
	*/
   UINT getCategoryById()const;

   /*! \fn std::string getEtherType(std::string strMsgName) const
    * \brief Method to get Message ether type from message name
	* \param [in] strMsgName Message name
	* \return Message ether type string
	*/
   std::string getEtherType(std::string strMsgName) const;

   /*! \fn std::string getMsgStyle() const
    * \brief Method to get Message style
	* \return Message style string
	*/   
   std::string getMsgStyle() const;

   /*! \fn msgStyle getMsgStyle()
    * \brief Method to get Message style
	* \return Message style enumaration
	*/ 
   msgStyle getMsgStyle();

   /*! \fn void setStyle(msgStyle style) 
    * \brief Method to set Message style
	* \param [in] style Message style enumaration
	*/ 
   void setStyle(msgStyle style);

   /*! \fn static std::map<std::string, std::list<std::string> > getCategoryLogsMap()
    * \brief Method to get category logs map
	* \return map of category logs
	*/ 
   static std::map<std::string, std::list<std::string> > getCategoryLogsMap();

   /*! \fn static std::map<std::string, std::string> getHiddenLogsMap()
    * \brief Method to get hidden logs map
	* \return map of hidden logs
	*/ 
   static std::map<std::string, std::string> getHiddenLogsMap();

   /*! \fn UINT getNoOfParamasById(UINT uiMessageId)
    * \brief Method to get number of parameters in a message from message id
	* \return Number of parameters in a message
	*/
   UINT getNoOfParamasById(UINT uiMessageId);

   /*! \fn static std::string getMsgName(UINT uiMsgId)
    * \brief Method to get message name from given message id
	* \param [in] uiMsgId Message ID
	* \return Message name string
	*/
   static std::string getMsgName(UINT uiMsgId);

   /*! \fn UINT getNoOfParamas(UINT uiMessageId)
    * \brief Method to get number of parameters in a messgae from given message id
	* \param [in] uiMessageId Message ID
	* \return Number of parameters
	*/
   UINT getNoOfParamas(UINT uiMessageId);

   /*! \fn UINT getId() const
    * \brief Method to get message id
	* \return ID of the message
	*/
   UINT getId() const;

   /*! \fn std::string getName() const
    * \brief Method to get message name
	* \return Name of the message
	*/
   std::string getName() const;

   /*! \fn BOOL getHidden() const
    * \brief Method to check for hidden logs 
	* \return TRUE if it is hidden message else FALSE.
	*/
   BOOL getHidden() const;

private:
   /*! \var std::multimap<std::string, UINT> msgIdByName
    * \brief multi map variable with message name and id as key-value pair
	*/
   std::multimap<std::string, UINT> msgIdByName;

   /*! \var std::multimap<UINT, std::string> msgNameById
    * \brief multi map variable with message id and name as key-value pair
	*/
   std::multimap<UINT, std::string> msgNameById;

   /*! \var std::map<std::string, std::string> etherTypeById
    * \brief multi map variable with message name and ether type as key-value pair
	*/
   std::map<std::string, std::string> etherTypeById;

   /*! \var static std::map<std::string, std::list<std::string> > listOfMsgsByCategory
    * \brief  Embedded map for category logs with category as key
	*/
   static std::map<std::string, std::list<std::string> > listOfMsgsByCategory;

   /*! \var std::map<std::string, std::string> msgStyleByMsgName
    * \brief  Map for message name and style as key-value pair
	*/   
   std::map<std::string, std::string> msgStyleByMsgName;

   /*! \var static std::list<std::string> listOfPosMsgName
    * \brief  List of positioning engine messages names
	*/  
   static std::list<std::string> listOfPosMsgName;

   /*! \var static std::list<std::string> listOfNavMsgName
    * \brief  List of navigation engine messages names
	*/  
   static std::list<std::string> listOfNavMsgName;

   /*! \var static std::list<std::string> listOfMeaMsgName
    * \brief  List of measurement engine messages names
	*/  
   static std::list<std::string> listOfMeaMsgName;

   /*! \var static std::list<std::string> listOfCommandMsgName
    * \brief  List of Command messages names
	*/    
   static std::list<std::string> listOfCommandMsgName;

   /*! \var static std::list<std::string> listOfOtherMsgName
    * \brief  List of Other category messages names
	*/  
   static std::list<std::string> listOfOtherMsgName;

   /*! \var static std::map<std::string, std::string> hiddenLogs
    * \brief  Map for hidden logs name and status as key-value pair.
	*/
   static std::map<std::string, std::string> hiddenLogs;

   /*! \var std::map<UINT, UINT> noOfParametersById
    * \brief  Map for Number of parameters in a message and id of it as key-value pair
	*/
   std::map<UINT, UINT> noOfParametersById;

   /*! \var static std::map<UINT, std::string> msgNameSta
    * \brief  Map for Message name and status as key-value pair
	*/
   static std::map<UINT, std::string> msgNameSta;

   /*! \var std::map<std::string, UINT> noOfParams
    * \brief  Map for Message name and number of parameters in it as key-value pair
	*/
   std::map<std::string, UINT> noOfParams;

   /*! List of Message ID's variable*/
   std::list<UINT> m_lstMessageId;

   /*! Message style string variable*/
   std::string m_strStyle;

   /*! Message def CRC string variable*/
   std::string m_strMsgDefCrc;

   /*! Message category enumaration variable */
   msgCategory m_Category;

   /*! Message category integer value */
   UINT m_iCategory;

   /*! Message style enum value */
   msgStyle m_style;

   /*! Message ID variable */
   UINT uiMsgId;
   /*! Message name string variable*/
   std::string m_strName;

   /*! Boolean value for Hidden logs check */
   BOOL bhidden;
};

#endif // CMESSAGE_H
