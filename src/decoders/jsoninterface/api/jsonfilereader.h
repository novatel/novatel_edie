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

/*! \file jsonfilereader.h
 *  \brief Class to do json file reading and parsing 
 *
 */

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef JSONFILEREADER_H
#define JSONFILEREADER_H

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "message.hpp"
#include "messageparams.hpp"
#include "types.hpp"
#include "enums.hpp"
#include <lib/nlohmann/json.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include "decoders/common/api/nexcept.h"

//-----------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------
/*! Namespace std */
using namespace std;

/*! Variable to use json object from nlohmann libray */
using json = nlohmann::json;

/*! \class JSONFileReader
 *  \brief Class with methods to read/parse the json file
 * 
 */
class JSONFileReader
{
public:
   /*! \fn void readJSONFile(string strJSONPath)
    * \brief Read the given json file with nlohmann open source json library and gets its definitions
	* 
	* \param [in] strJSONPath String contians json file full path
	*/ 
   void readJSONFile(string strJSONPath);
#ifdef WIDE_CHAR_SUPPORT
   /*! \fn void readJSONFile(wstring strJSONPath)
    * \brief Read the given json file with nlohmann open source json library and gets its definitions
	* 
	* \param [in] strJSONPath String contians json file full path
	*/ 
	void readJSONFile(wstring strJSONPath);
#endif
   /*! \fn void readElement(std::string json_obj)
    * \brief 
	* 
	* \param [in] json_obj json file
	*/ 
	void readElement(std::string json_obj);

   /*! \fn void readParamElement(std::string json_obj)
    * \brief 
	* 
	* \param [in] json_obj json file
	*/ 
	void readParamElement(std::string json_obj);

   /*! \fn void readTypeListElemt(std::string json_obj)
    * \brief 
	* 
	* \param [in] json_obj json file
	*/ 	
	void readTypeListElemt(std::string json_obj);

   /*! \fn void readEnumListElemt(std::string json_obj)
    * \brief 
	* 
	* \param [in] json_obj json file
	*/ 	
	void readEnumListElemt(std::string json_obj);

   /*! \fn std::list<INT> getMsgIdList() const
    * \brief Gets the list of Message ID's which were extracted from json file
	* 
	* \return Message ID's list which were present in json file
	*/	
	std::list<INT> getMsgIdList() const;

   /*! \fn std::list<string> getMsgNameList() const
    * \brief Gets the list of Message name's which were extracted from json file
	* 
	* \return Message names's list which were present in json file
	*/		
	std::list<string> getMsgNameList() const;

   /*! \fn std::vector<CMessage*> getMessageObj(std::string& strName)
    * \brief Gets the CMessage class objects which were created while reading json file
	* \sa CMessage 
	* 
	* \param [in] strName
	* 
	* \return Vector of CMessage class objects which were created while reading json file
	*/		
	std::vector<CMessage*> getMessageObj(std::string& strName);

   /*! \fn std::vector<CMessageParams*> getMessageParamObj(std::string& strName)
    * \brief Gets the CMessageParams class objects which were created while reading json file
	* \sa CMessageParams 
	* 
	* \param [in] strName
	* 
	* \return Vector of CMessageParams class objects which were created while reading json file
	*/
	std::vector<CMessageParams*> getMessageParamObj(std::string& strName);

   /*! \fn std::vector<CTypes*> getTypesObj(std::string& strName)
    * \brief Gets the CTypes class objects which were created while reading json file
	* \sa CMessageParams 
	* 
	* \param [in] strName
	* 
	* \return Vector of CTypes class objects which were created while reading json file
	*/	
   std::vector<CTypes*> getTypesObj(std::string& strName);

   /*! \fn std::vector<CEnums*> getEnumObj(std::string& strName)
    * \brief Gets the CEnums class objects which were created while reading json file
	* \sa CEnums 
	* 
	* \param [in] strName
	* 
	* \return Vector of CEnums class objects which were created while reading json file
	*/	
   std::vector<CEnums*> getEnumObj(std::string& strName);

   /*! \fn std::string getMsgDefCRC(INT iMsgId)
	* \brief Gets Message definition CRC of a given message id
	* 
	* \param [in] iMsgId ID of the message
	* 
	* \return CRC definition string of Message
	*/ 
   std::string getMsgDefCRC(INT iMsgId);

   /*! \fn BOOL isValidMsgId(LONG ulMessageId)
    * \brief Method to check given id of the message is valid or not
	* 
	* \param [in] ulMessageId ID of the message
	* 
	* \return TRUE if Message id is valid
	* FALSE if Messgae id is not valid
	*/ 	
   BOOL isValidMsgId(LONG ulMessageId);

   /*! \fn std::string getMessageName(INT uiMsgId, string& strCRC)
    * \brief Method to get Message name with given message id and crc definition of it
	* 
	* \param [in] uiMsgId ID of the message
	* \param [in] strCRC CRC definition of the message
	* 
	* \return Valid Message name
	*/ 
   std::string getMessageName(INT uiMsgId, string& strCRC);

   /*! \fn std::string getMessageName(INT uiMsgId)
    * \brief Method to get Message name with given message id 
	* 
	* \param [in] uiMsgId ID of the message
	* 
	* \return Valid Message name
	*/ 
   std::string getMessageName(INT uiMsgId);

   /*! \fn BOOL isValidMsgDefCRC(LONG ulMsgId, string& strCRC)
    * \brief Method to check CRC definition of a message id is valid or not
	* 
	* \param [in] ulMsgId ID of the message
	* \param [in] strCRC CRC definition of the message
	* 
	* \return TRUE if CRC definition of the message is valid
	* FALSE if CRC definition of the message is not valid
	*/ 
   BOOL isValidMsgDefCRC(LONG ulMsgId, string& strCRC);

   /*! \fn BOOL isValidmsgName(const string& strMsgName)
    * \brief Method to check CRC definition of a message id
	* 
	* \param [in] strMsgName Name of the message
	* 
	* \return TRUE if message name is valid
	* FALSE if message is not valid
	*/ 
   BOOL isValidmsgName(const string& strMsgName);

   /*! \fn INT getMessageId(const string& strMsgName_)
    * \brief Method to get message id from given message name
	* 
	* \param [in] strMsgName_ Name of the message
	* 
	* \return ID of the message
	*/ 
   INT getMessageId(const string& strMsgName_);

   /*! A static method 
    * \fn static JSONFileReader *getDatabaseObject(const string& strName, INT iAdvMode)
    * \brief Method to get the instance of JSONFileReader class object
	* 
	* \param [in] strName Full path of json file
	* \param [in] iAdvMode if it is non zero value, message name is appended with _<message id>
	* 
	* \return JSONFileReader object pointer
	*/ 
   static JSONFileReader *getDatabaseObject(const string& strName, INT iAdvMode);
#ifdef WIDE_CHAR_SUPPORT
   /*! A static method 
    * \fn static JSONFileReader *getDatabaseObject(const string& strName, INT iAdvMode)
    * \brief Method to get the instance of JSONFileReader class object
	* 
	* \param [in] strName Full path of json file with wide characters
	* \param [in] iAdvMode if it is non zero value, message name is appended with _<message id>
	* 
	* \return JSONFileReader object pointer
	*/ 
	static JSONFileReader *getDatabaseObject(const wstring& strName, INT iAdvMode);
#endif
   /*! A static method
    * \fn static void DestroyDatabaseObject()
	* \brief Deletes JSONFileReader object instance
	*/ 
   static void DestroyDatabaseObject();

   /*! \fn std::map<std::string, std::list<std::string> > getAllCatagarizedLogs()
    * \brief Gets all categarized logs from json file
	* \return list of messages in json file with catagory
	*/
   std::map<std::string, std::list<std::string> > getAllCatagarizedLogs();

   /*! \fn UINT getNoOfParamas(INT uiMessageId, string& strCRC)
    * \brief Method to get number of paramters in a message with given message id and crc definition
	* 
	* \param [in] uiMessageId Message ID
	* \param [in] strCRC CRC definition of the message
	*
	* \return Number of parameters of the message
	*/
   UINT getNoOfParamas(INT uiMessageId, string& strCRC);

   /*! \fn std::map<std::string, std::string> getHiddenStatusOfLogs()
    * \brief Method to get hidden logs status
	* 
	* \return list of hidden log messages
	*/
   std::map<std::string, std::string> getHiddenStatusOfLogs();

   /*! \fn BOOL isValidMsgDefCRC(string& msgName, string& strCRC)
    * \brief Method to check the CRC definition of the message is valid or not
	* 
	* \param [in] msgName Name of the message
	* \param [in] strCRC CRC definitio of the message
	*
	* \return TRUE if Message definition CRC is valid
	* FALSE if Message definition CRC is not valid
	*/ 
   BOOL isValidMsgDefCRC(string& msgName, string& strCRC);

   /*! \fn void changeMsgParamsKeyName(string strNewKey)
    * \brief Changes the value of key of the message parameters
	* 
	* \param [in] strNewKey new key value string
	*/
   void changeMsgParamsKeyName(string strNewKey);

   /*! \fn void changeTypesKeyName(string strNewKey)
    * \brief Changes the value of key of the message types 
	* 
	* \param [in] strNewKey new key value string
	*/   
   void changeTypesKeyName(string strNewKey);

   /*! \fn void changeEnumsKeyName(string strNewKey)
    * \brief Changes the value of key of the message Enums 
	* 
	* \param [in] strNewKey new key value string
	*/     
   void changeEnumsKeyName(string strNewKey);
private:
   /*! A class constructor
    * \brief Reads the json file provided
	*/ 
   explicit JSONFileReader(const string& strJSONPath);
#ifdef WIDE_CHAR_SUPPORT
   /*! A class constructor
    * \brief Reads the json file provided with wide characters
	*/ 
   explicit JSONFileReader(const wstring& strJSONPath);
#endif
   /*! Default destructor
    * \brief Destoys the maps of data base objects 
	*/ 
   ~JSONFileReader();
   /*! \var static JSONFileReader* JSONDataObj
    * \brief A static variable to hold the instance of JSONFileReader object
	*/ 
   static JSONFileReader* JSONDataObj;

   /*! \var static INT iAdvModeLb
    * \brief A non zero value will append _<message id> to the message name
	*/    
   static INT iAdvModeLb;

   /*! \var INT iMessageId
    * \brief Message ID value
	*/
   INT iMessageId;

   /*! \var string strMsgName
    * \brief Message name
	*/
   string strMsgName;

   /*! \var string strMsgDefCrc
    * \brief CRC definition of the messgae
	*/   
   string strMsgDefCrc;

   /*! \var string strUniqueKey
    * \brief Unique key of the parameters of the messgae
	*/     
   string strUniqueKey;

   //BOOL hasChildElem;
   //BOOL hasCompletedIter;

   /*! \var CMessage *pMessage
    * \brief CMessage class object pointer
	*/ 
   CMessage *pMessage;

   /*! \var CMessageParams *pMessageParams
    * \brief CMessageParams class object pointer
	*/ 
   CMessageParams *pMessageParams;

   /*! \var CTypes *pType
    * \brief CTypes class object pointer
	*/    
   CTypes *pType;

   /*! \var CEnums *pEnum
    * \brief CEnums class object pointer
	*/    
   CEnums *pEnum;

   /*! \var std::list<INT> msgIdList
    * \brief List of Message ID's
	*/ 
   std::list<INT> msgIdList;

   /*! \var std::list<string> msgNameList
    * \brief List of Message name
	*/ 
   std::list<string> msgNameList;

   //INT iTypeId;

   /*! \var std::map<string, std::vector<CMessageParams*> > messageParamObjList
    * \brief List of CMessageParams class objects
	*/ 
   std::map<string, std::vector<CMessageParams*> > messageParamObjList;

   /*! \var std::map<string, std::vector<CMessage*> > messageObjList
    * \brief List of CMessage class objects
	*/
   std::map<string, std::vector<CMessage*> > messageObjList;

   /*! \var std::map<string, std::vector<CTypes*> > typesObjList
    * \brief List of CTypes class objects
	*/
   std::map<string, std::vector<CTypes*> > typesObjList;

   /*! \var std::map<string, std::vector<CEnums*> > enumObjList
    * \brief List of CEnums class objects
	*/
   std::map<string, std::vector<CEnums*> > enumObjList;

   /*! \var std::map<INT, string> msgMap
    * \brief
	*/   
   std::map<INT, string> msgMap;

   /*! \var std::map<INT, string> uniqueMsgMap
    * \brief
	*/   
   std::map<INT, string> uniqueMsgMap;

   /*! \var std::list<CMessage*> messageObjectList
    * \brief
	*/      
   std::list<CMessage*> messageObjectList;

   /*! \var std::map<INT, string> msgDefCRC
    * \brief
	*/      
   std::map<INT, string> msgDefCRC;

   /*! \fn void insertMessageParamObjectList(CMessageParams* ParmObj)
    * \brief Inserts CMessageParams class object into vector of CMessageParams objects
	* \param [in] ParmObj CMessageParams object
	*/
   void insertMessageParamObjectList(CMessageParams* ParmObj);

   /*! \fn void insertTypesObjList(CTypes* Types)
    * \brief Inserts CTypes class object into vector of CTypes objects
	* \param [in] Types CTypes object
	*/   
   void insertTypesObjList(CTypes* Types);

   /*! \fn void insertEnumObjList(CEnums* Enum)
    * \brief Inserts CEnums class object into vector of CEnums objects
	* \param [in] Enum CEnums object
	*/  
   void insertEnumObjList(CEnums* Enum);

   /*! \fn void insertMessageObjList(CMessage *Messages)
    * \brief Inserts CMessage class object into vector of CMessage objects
	* \param [in] Messages CMessage object
	*/ 
   void insertMessageObjList(CMessage *Messages);

   /*! \fn void clearDbObjectMaps()
    * \brief Clears all maps created in this class
	*/ 
   void clearDbObjectMaps();
};
#endif // JSONFILEREADER_H
