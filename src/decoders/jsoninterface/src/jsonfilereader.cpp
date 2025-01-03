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
//  DESCRIPTION: To read the JSON file
//    
//
////////////////////////////////////////////////////////////////////////////////

#include "jsonfilereader.h"
JSONFileReader* JSONFileReader::JSONDataObj = NULL;
INT JSONFileReader::iAdvModeLb = 0;
JSONFileReader::JSONFileReader(const string& strJSONPath) :
	pMessage(nullptr), pMessageParams(nullptr), pType(nullptr),
	pEnum(nullptr)
{
	iMessageId = 0;
	readJSONFile(strJSONPath);
}

#ifdef WIDE_CHAR_SUPPORT
JSONFileReader::JSONFileReader(const wstring& strJSONPath) :
	pMessage(nullptr), pMessageParams(nullptr), pType(nullptr),
	pEnum(nullptr)
{
	iMessageId = 0;
	readJSONFile(strJSONPath);
}
#endif

JSONFileReader::~JSONFileReader()
{
	clearDbObjectMaps();
}

void JSONFileReader::clearDbObjectMaps()
{
	for (std::map<string, std::vector<CMessageParams*> >::iterator itmessageParamIterator = messageParamObjList.begin(); itmessageParamIterator != messageParamObjList.end();)
	{
		std::vector<CMessageParams*> tempParamObj = itmessageParamIterator->second;
		vector<CMessageParams*>::iterator it;
		for (it = tempParamObj.begin(); it != tempParamObj.end();)
		{
			if (*it != nullptr)
			{
				delete *it;
				*it = nullptr;            
			}
			it = tempParamObj.erase(it);
		}
		itmessageParamIterator = messageParamObjList.erase(itmessageParamIterator);
	}
	for (std::map<string, std::vector<CMessage*> >::iterator itmessageIterator = messageObjList.begin(); itmessageIterator != messageObjList.end();)
	{
		std::vector<CMessage*> tempMessageObj = itmessageIterator->second;
		vector<CMessage*>::iterator it;
		for (it = tempMessageObj.begin(); it != tempMessageObj.end();)
		{
			if (*it != nullptr)
			{
				delete *it;
				*it = nullptr;            
			}
			it = tempMessageObj.erase(it);
		}
		itmessageIterator = messageObjList.erase(itmessageIterator);
	}
	for (std::map<string, std::vector<CTypes*> >::iterator itTypesIterator = typesObjList.begin(); itTypesIterator != typesObjList.end();)
	{
		std::vector<CTypes*> tempTypesObj = itTypesIterator->second;
		vector<CTypes*>::iterator it;
		for (it = tempTypesObj.begin(); it != tempTypesObj.end();)
		{
			if (*it != nullptr)
			{
				delete *it;
				*it = nullptr;            
			}
			it = tempTypesObj.erase(it);
		}
		itTypesIterator = typesObjList.erase(itTypesIterator);
	}
	for (std::map<string, std::vector<CEnums*> >::iterator itEnumIterator = enumObjList.begin(); itEnumIterator != enumObjList.end();)
	{
		std::vector<CEnums*> tempEnumObj = itEnumIterator->second;
		vector<CEnums*>::iterator it;
		for (it = tempEnumObj.begin(); it != tempEnumObj.end();)
		{
			if (*it != nullptr)
			{
				delete *it;
				*it = nullptr;            
			}
			it = tempEnumObj.erase(it);
		}
		itEnumIterator = enumObjList.erase(itEnumIterator);
	}
}


void JSONFileReader::changeMsgParamsKeyName(string strNewKey)
{
	std::map<string, vector<CMessageParams*> >::iterator it = messageParamObjList.find(strMsgName);
	if (it != messageParamObjList.end())
	{
		std::swap(messageParamObjList[strNewKey], it->second);
		messageParamObjList.erase(it);
	}
}

void JSONFileReader::changeTypesKeyName(string strNewKey)
{
	std::map<string, vector<CTypes*> >::iterator it = typesObjList.find(strMsgName);
	if (it != typesObjList.end())
	{
		std::swap(typesObjList[strNewKey], it->second);
		typesObjList.erase(it);
	}
}

void JSONFileReader::changeEnumsKeyName(string strNewKey)
{
	std::map<string, vector<CEnums*> >::iterator it = enumObjList.find(strMsgName);
	if (it != enumObjList.end())
	{
		std::swap(enumObjList[strNewKey], it->second);
		enumObjList.erase(it);
	}
}
void JSONFileReader::readJSONFile(string strJSONPath)
{
	fstream json_file;
	json_file.open(strJSONPath, ios::in);
	json message_definitions = json::parse(json_file);

	for (UINT index = 0; index < message_definitions.size(); index++)
	{
		pMessage = new CMessage();
		readElement(message_definitions[index].dump());
		//strUniqueKey = strMsgName + "_" + std::to_string(iMessageId) + "_" + strMsgDefCrc;
		strUniqueKey = strMsgName + "_" + std::to_string(iMessageId);
		uniqueMsgMap.insert(std::pair<INT, string>(iMessageId, strUniqueKey));
		changeMsgParamsKeyName(strUniqueKey);
		changeTypesKeyName(strUniqueKey);
		changeEnumsKeyName(strUniqueKey);
		insertMessageObjList(pMessage);
	}
	json_file.close();
}


#ifdef WIDE_CHAR_SUPPORT
void JSONFileReader::readJSONFile(wstring strJSONPath)
{
	fstream json_file;
	json_file.open(strJSONPath, ios::in);
	json message_definitions = json::parse(json_file);

	for (UINT index = 0; index < message_definitions.size(); index++)
	{
		pMessage = new CMessage();
		readElement(message_definitions[index].dump());
		//strUniqueKey = strMsgName + "_" + std::to_string(iMessageId) + "_" + strMsgDefCrc;
		strUniqueKey = strMsgName + "_" + std::to_string(iMessageId);
		uniqueMsgMap.insert(std::pair<INT, string>(iMessageId, strUniqueKey));
		changeMsgParamsKeyName(strUniqueKey);
		changeTypesKeyName(strUniqueKey);
		changeEnumsKeyName(strUniqueKey);
		insertMessageObjList(pMessage);
	}
	json_file.close();
}
#endif
std::list<INT> JSONFileReader::getMsgIdList() const
{
	return msgIdList;
}

std::list<string> JSONFileReader::getMsgNameList() const
{
	return msgNameList;
}
void JSONFileReader::readElement(std::string json_obj)
{
	json message_obj = json::parse(json_obj);

	if (message_obj.contains("logID"))
	{
		iMessageId = message_obj["logID"];
		msgIdList.push_back(iMessageId);
		if (pMessage != nullptr)
		{
			pMessage->setMsgId(iMessageId);
		}
	}
	else
	{
		throw nExcept("%s" " KEY not in json message definitions", "logID");
	}
	if (message_obj.contains("name"))
	{
		strMsgName = message_obj["name"];
		msgNameList.push_back(strMsgName);
		msgMap.insert(std::pair<INT, string>(iMessageId, strMsgName));
		if (pMessage != nullptr)
		{
			pMessage->setMsgNameAndId(strMsgName.c_str(), iMessageId);
		}
		strUniqueKey.clear();
		if (iAdvModeLb)
			strUniqueKey = strMsgName + "_" + std::to_string(iMessageId); //+ "_" + strMsgDefCrc;
		else
			strUniqueKey = strMsgName;
	}
	else
	{
		throw nExcept("%s" " KEY not in json message definitions", "name");
	}
	if (message_obj.contains("hidden"))
	{
		BOOL hidden = message_obj["hidden"];
		if (pMessage != nullptr)
		{
			if (hidden == 1)
			{
				pMessage->setHiddenStatus(strMsgName.c_str(), "True");
			}
			else
			{
				pMessage->setHiddenStatus(strMsgName.c_str(), "False");
			}
		}
	}
	else
	{
		throw nExcept("%s" " KEY not in json message definitions", "hidden");
	}
	if (message_obj.contains("messageStyle"))
	{
		if (pMessage != nullptr)
		{
			pMessage->setMsgStyle(strMsgName.c_str(), message_obj["messageStyle"]);
		}
	}
	else
	{
		throw nExcept("%s" " KEY not in json message definitions", "messageStyle");
	}
	if (message_obj.contains("definitionCRC"))
	{
		stringstream MsgDefCRC;
		MsgDefCRC << std::hex << (UINT)message_obj["definitionCRC"];
		if (pMessage != nullptr)
		{
			pMessage->setMsgDefCRC(MsgDefCRC.str());
			msgDefCRC.insert(std::pair<INT, string>(iMessageId, MsgDefCRC.str()));
		}
		strMsgDefCrc = MsgDefCRC.str();
	}
	else
	{
		throw nExcept("%s" " KEY not in json message definitions", "definitionCRC");
	}
	if (message_obj.contains("fields"))
	{
		if (pMessage != nullptr)
		{
			pMessage->setNoOfParams((UINT)message_obj["fields"].size(), strMsgName.c_str());
		}
		readParamElement(message_obj["fields"].dump());
		
	}
	else
	{
		throw nExcept("%s" " KEY not in json message definitions", "fields");
	}
	
}

void JSONFileReader::readParamElement(std::string json_obj)
{
	json fields_obj = json::parse(json_obj);
	
	for (UINT index = 0; index < fields_obj.size(); index++)
	{
		pMessageParams = new CMessageParams();
		pType = new CTypes();
		pEnum = nullptr;
		
		if (fields_obj[index].contains("name"))
		{
			if (pType != nullptr)
			{
				pMessageParams->setElementname(fields_obj[index]["name"]);
			}
		}
		else
		{
			throw nExcept("%s" " KEY not in json message definitions", "name");
		}
		if (fields_obj[index].contains("arrayLength"))
		{
			if (pType != nullptr)
			{
				if (fields_obj[index]["arrayLength"].empty() != TRUE)
				{
					pType->setArrayLength(fields_obj[index]["arrayLength"]);
				}
				else
				{
					pType->setArrayLength(0);
				}
			}
		}
		if (fields_obj[index].contains("conversionString"))
		{
			if (pType != nullptr)
			{
				if (fields_obj[index]["conversionString"].empty() != TRUE)
				{
					string conver_str = fields_obj[index]["conversionString"];
					pType->setConversionString((CHAR*)conver_str.c_str());
				}
				else
				{
					pType->setConversionString("");
				}
			}
		}
		if (fields_obj[index].contains("numChildren"))
		{
			if (pType != nullptr)
			{
				if (fields_obj[index]["numChildren"].empty() != TRUE)
				{
					pMessageParams->setChildParamValue(fields_obj[index]["numChildren"]);
				}
				else
				{
					pMessageParams->setChildParamValue(0);
				}
			}
		}
		if (fields_obj[index].contains("type"))
		{
			readTypeListElemt(fields_obj[index]["type"].dump());
		}
		else
		{
			throw nExcept("%s" " KEY not in json message definitions", "type");
		}
		insertMessageParamObjectList(pMessageParams);
		insertTypesObjList(pType);
		insertEnumObjList(pEnum);
	}
	
}

void JSONFileReader::readTypeListElemt(std::string json_obj)
{
	json types_obj = json::parse(json_obj);
	if (types_obj.contains("storageType"))
	{
		if (pType != nullptr)
		{
			pType->setStorageType(types_obj["storageType"]);
		}
	}
	else
	{
		throw nExcept("%s" " KEY not in json message definitions", "storageType");
	}
	if (types_obj.contains("length"))
	{

		if (pType != nullptr && types_obj["baseType"].empty() != TRUE)
		{
			pType->setLength(types_obj["baseType"]["length"]);
		}
      else
      {
         pType->setLength(types_obj["length"]);
      }
	}
	else
	{
		throw nExcept("%s" " KEY not in json message definitions", "length");
	}
	if (types_obj.contains("name"))
	{
		if (pType != nullptr)
		{
			pType->setTypeName(types_obj["name"]);
		}
	}
	else
	{
		throw nExcept("%s" " KEY not in json message definitions", "name");
	}
	if (types_obj.contains("baseType"))
	{
		if (pType != nullptr && types_obj["baseType"].empty() != TRUE)
		{
			pType->setBaseTypeName(types_obj["baseType"]["name"]);
		}
	}
	if (types_obj.contains("enum"))
	{
		if (types_obj["enum"].empty() != TRUE)
		{
			pEnum = new CEnums();
			if (pType != nullptr)
			{
				readEnumListElemt(types_obj["enum"].dump());
			}
		}
	}
}
void JSONFileReader::readEnumListElemt(std::string json_obj)
{
	json enum_obj = json::parse(json_obj);
	for (UINT index = 0; index < enum_obj.size(); index++)
	{
		if (enum_obj[index].contains("name") && enum_obj[index].contains("value"))
		{
			try
			{
				pEnum->setEnumNameAndValue(enum_obj[index]["value"], enum_obj[index]["name"]);
			}
			catch (...)
			{
				continue;
			}
		}
		else
		{
			throw nExcept("%s" " KEY not in json message definitions", "name and value");
		}
	}
	
}

void JSONFileReader::insertMessageParamObjectList(CMessageParams *ParmObj)
{
	if (messageParamObjList.find(strUniqueKey) != messageParamObjList.end())
	{
		std::vector<CMessageParams*> paramObj = messageParamObjList.at(strUniqueKey);
		paramObj.push_back(ParmObj);
		messageParamObjList[strUniqueKey] = paramObj;
	}
	else
	{
		std::vector<CMessageParams*> qlist_Parameters;
		qlist_Parameters.push_back(ParmObj);
		messageParamObjList[strUniqueKey] = qlist_Parameters;
	}
}

void JSONFileReader::insertTypesObjList(CTypes *Types)
{
	if (typesObjList.find(strUniqueKey) != typesObjList.end())
	{
		std::vector<CTypes*> typesObj = typesObjList.at(strUniqueKey);
		typesObj.push_back(Types);
		typesObjList[strUniqueKey] = typesObj;
	}
	else
	{
		std::vector<CTypes*> qlist_Types;
		qlist_Types.push_back(Types);
		typesObjList[strUniqueKey] = qlist_Types;
	}
}

void JSONFileReader::insertEnumObjList(CEnums *Enum)
{
	if (enumObjList.find(strUniqueKey) != enumObjList.end())
	{
		std::vector<CEnums*> enumObj = enumObjList.at(strUniqueKey);
		enumObj.push_back(Enum);
		enumObjList[strUniqueKey] = enumObj;
	}
	else
	{
		std::vector<CEnums*> qlist_Enums;
		qlist_Enums.push_back(pEnum);
		enumObjList[strUniqueKey] = qlist_Enums;
	}
}

void JSONFileReader::insertMessageObjList(CMessage *Messages)
{
	if (messageObjList.find(strUniqueKey) != messageObjList.end())
	{
		std::vector<CMessage*> messageObj = messageObjList.at(strUniqueKey);
		messageObj.push_back(Messages);
		messageObjList[strUniqueKey] = messageObj;
	}
	else
	{
		std::vector<CMessage*> qlist_Messages;
		qlist_Messages.push_back(Messages);
		messageObjList[strUniqueKey] = qlist_Messages;
	}
}
std::vector<CMessage*> JSONFileReader::getMessageObj(std::string& strName)
{
	vector<CMessage*> temp;
	string strTemp = strName;
	if (messageObjList.find(strTemp) != messageObjList.end())
	{
		temp = messageObjList.at(strTemp);
	}
	return temp;
}

std::vector<CMessageParams *> JSONFileReader::getMessageParamObj(std::string& strName)
{
	vector<CMessageParams*> temp;
	string strTemp = strName;
	if (messageParamObjList.find(strTemp) != messageParamObjList.end())
	{
		temp = messageParamObjList.at(strTemp);
	}
	return temp;
}

std::vector<CTypes *> JSONFileReader::getTypesObj(std::string& strName)
{
	vector<CTypes*> temp;
	string strTemp = strName;
	if (typesObjList.find(strTemp) != typesObjList.end())
	{
		return typesObjList.at(strTemp);
	}
	return temp;
}

std::vector<CEnums *> JSONFileReader::getEnumObj(std::string& strName)
{
	vector<CEnums*> temp;
	string strTemp = strName;
	if (enumObjList.find(strTemp) != enumObjList.end())
	{
		return enumObjList.at(strTemp);
	}
	return temp;
}

JSONFileReader *JSONFileReader::getDatabaseObject(const string& strPath, INT iAdvMode)
{
	iAdvModeLb = iAdvMode;
	if (!JSONDataObj)
	{
		JSONDataObj = new JSONFileReader(strPath);
	}
	return JSONDataObj;
}

#ifdef WIDE_CHAR_SUPPORT
JSONFileReader *JSONFileReader::getDatabaseObject(const wstring& strPath, INT iAdvMode)
{
	iAdvModeLb = iAdvMode;
	if (!JSONDataObj)
	{
		JSONDataObj = new JSONFileReader(strPath);
	}
	return JSONDataObj;
}
#endif

void JSONFileReader::DestroyDatabaseObject()
{
	if (JSONDataObj != NULL)
	{
		delete JSONDataObj;
		JSONDataObj = NULL;
	}
}

std::map<std::string, std::list<std::string> > JSONFileReader::getAllCatagarizedLogs()
{
	return CMessage::getCategoryLogsMap();
}

UINT JSONFileReader::getNoOfParamas(INT uiMessageId, string& strCRC)
{
	if (msgMap.find(uiMessageId) != msgMap.end())
	{
		string MsgName = uniqueMsgMap.find(uiMessageId)->second;
		std::vector<CMessage*> temp = messageObjList.find(MsgName)->second;

		for (INT iSize = 0; iSize < (INT)temp.size(); ++iSize)
		{
			CMessage *Message = temp.at(iSize);
			if (Message->getMsgDefCrc() == strCRC)
			{
				string msgName = msgMap.find(uiMessageId)->second;
				return temp.at(iSize)->getNoOfParams(msgName);
			}
		}
	}
	return 0;
}

std::map<std::string, std::string> JSONFileReader::getHiddenStatusOfLogs()
{
	return CMessage::getHiddenLogsMap();
}

std::string JSONFileReader::getMsgDefCRC(INT iMsgId)
{
	if (uniqueMsgMap.find(iMsgId) != uniqueMsgMap.end())
	{
		string MsgName = uniqueMsgMap.find(iMsgId)->second;
		std::vector<CMessage*> temp = messageObjList.find(MsgName)->second;

		for (INT iSize = 0; iSize < (INT)temp.size(); ++iSize)
		{
			return temp.at(iSize)->getMsgDefCrc();
		}
	}
	return "";
}

std::string JSONFileReader::getMessageName(INT uiMsgId, string& strCRC)
{
	if (msgMap.find(uiMsgId) != msgMap.end())
	{
		string MsgName = uniqueMsgMap.find(uiMsgId)->second;
		std::vector<CMessage*> temp = messageObjList.find(MsgName)->second;

		for (INT i = 0; i < (INT)temp.size(); ++i)
		{
			CMessage *Message = temp.at(i);
			if (Message->getMsgDefCrc() == strCRC)
			{
				return Message->getName();
			}
		}
	}
	return "";
}
std::string JSONFileReader::getMessageName(INT uiMsgId)
{
	if (msgMap.find(uiMsgId) != msgMap.end())
	{
		string MsgName = uniqueMsgMap.find(uiMsgId)->second;
		return MsgName;			
	}
	return "";
}
BOOL JSONFileReader::isValidMsgId(LONG ulMessageId)
{
	std::list<INT>::iterator findIter = std::find(msgIdList.begin(), msgIdList.end(), ulMessageId);
	if (findIter != msgIdList.end())
	{
		return TRUE;
	}
	return FALSE;
}

BOOL JSONFileReader::isValidmsgName(const string& MsgName)
{
	std::list<string>::iterator findIter = std::find(msgNameList.begin(), msgNameList.end(), MsgName);
	if (findIter != msgNameList.end())
	{
		return TRUE;
	}
	return FALSE;
}

BOOL JSONFileReader::isValidMsgDefCRC(LONG ulMsgId, string& strCRC)
{
	if (msgDefCRC.find(ulMsgId) != msgDefCRC.end())
	{
		string strDefCrc = msgDefCRC.find(ulMsgId)->second;
		if (strCRC == strDefCrc)
		{
			return TRUE;
		}
	}
	return FALSE;
}

INT JSONFileReader::getMessageId(const string &strMsgName_)
{
	INT key = -1;
	std::map<INT, std::string>::const_iterator it;
	for (it = msgMap.begin(); it != msgMap.end(); ++it)
	{
		if (it->second == strMsgName_)
		{
			key = it->first;
			return key;
		}
	}
	return key;
}
BOOL JSONFileReader::isValidMsgDefCRC(string& msgName, string& strCRC)
{
	long msgId = getMessageId(msgName);
	if (msgDefCRC.find(msgId) != msgDefCRC.end())
	{
		string strDefCrc = msgDefCRC.find(msgId)->second;
		if (strCRC == strDefCrc)
		{
			return TRUE;
		}
	}
	return FALSE;
}

