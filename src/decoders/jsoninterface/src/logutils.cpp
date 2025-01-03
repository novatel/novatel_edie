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
//    Class to provide log processing functionality
//
////////////////////////////////////////////////////////////////////////////////
#include "logutils.hpp"

LogUtils::LogUtils(JSONFileReader* dbData, std::string strMsgName):uiMyParameterNumber(0),uiMaxNoOfParameters(0), 
																	uiaclParameterNumber(0),offset(0),jsonarray_obj(json::array()),
																	uiFlattenbinarylength(0)
{
	LoadLogs(dbData,strMsgName);
}

LogUtils::~LogUtils()
{
	
}

void LogUtils::LoadLogs(JSONFileReader* dbData, std::string strMsgName)
{
	 messgeParamList = dbData->getMessageParamObj(strMsgName);
	 typesList = dbData->getTypesObj(strMsgName);
	 enumsList = dbData->getEnumObj(strMsgName);
	 messgeList = dbData->getMessageObj(strMsgName);
}

void LogUtils::CopyParameterToBuffer(CMessageParams* messageparam,CTypes* types, char** pLogBuf, char** pOutBuf)
{
	switch (types->getStorageTypeEnum())
	{
	case SIMPLE:
	case FIXEDARRAY:
	case ENUM:
		if (types->getStorageTypeEnum() == FIXEDARRAY)
		{
			memcpy(*pOutBuf, *pLogBuf , types->getLength()
				*types->getArrayLength());
			*pLogBuf += types->getLength()*types->getArrayLength();
			*pOutBuf += types->getLength()*types->getArrayLength();
			uiFlattenbinarylength += types->getLength()*types->getArrayLength();
		}
		else
		{
			memcpy(*pOutBuf, *pLogBuf , types->getLength());
			*pLogBuf += types->getLength();
			*pOutBuf += types->getLength();
			uiFlattenbinarylength += types->getLength();
		}
		break;
	case VARARRAY:
		UINT vArrayLength;
		memcpy(&vArrayLength, *pLogBuf, sizeof(UINT));
		memcpy(*pOutBuf , *pLogBuf, sizeof(UINT));
		*pLogBuf += sizeof(UINT);
		*pOutBuf += sizeof(UINT);
      uiFlattenbinarylength += sizeof(UINT);
		memcpy(*pOutBuf, *pLogBuf, types->getLength()*vArrayLength);
		*pLogBuf += types->getLength()*vArrayLength;
		*pOutBuf += types->getLength()*vArrayLength;
		uiFlattenbinarylength += types->getArrayLength()*types->getLength();
		if (vArrayLength < types->getArrayLength())
		{
			UINT RemaingBytes = (types->getArrayLength() - (vArrayLength))*types->getLength();
			memset(*pOutBuf , '\0', RemaingBytes);
			*pOutBuf += RemaingBytes;
		}
		break;
	case STRING:
		for (UINT szSize = 0; szSize < types->getArrayLength(); szSize++)
		{
			CHAR* cbyte = (CHAR*)(*pLogBuf);
			if ((*cbyte) != '\0')
			{
				memcpy(*pOutBuf , *pLogBuf , types->getLength());
				*pLogBuf += types->getLength();
				*pOutBuf += types->getLength();
				uiFlattenbinarylength += types->getLength();
			}
			else
			{
				UINT RemaingBites = types->getArrayLength() - (szSize);
				memset(*pOutBuf, '\0', RemaingBites);
				*pLogBuf += 4 - (szSize % 4);
				*pOutBuf += RemaingBites;
				uiFlattenbinarylength += RemaingBites;
				break;
			}
		}
		break;
	case CLASSARRAY:
	{
		UINT uiNoItrations;
		memcpy(&uiNoItrations, *pLogBuf, sizeof(UINT));
		memcpy(*pOutBuf, *pLogBuf, types->getLength());
		UINT NoofParameters = messageparam->getChildParamValue();
		*pLogBuf += types->getLength();
		*pOutBuf += types->getLength();
		uiFlattenbinarylength += types->getLength();
		UINT parameter = uiMyParameterNumber+1;
		for (UINT it = 0; it < uiNoItrations; it++)
		{
			for (UINT parameterit = 0; parameterit < NoofParameters; parameterit++)
			{
				CopyParameterToBuffer(messgeParamList[parameter + parameterit], typesList[parameter + parameterit], pLogBuf, pOutBuf);
			}
		}
		uiaclParameterNumber = NoofParameters;
		break;
	}
		
	default:
		break;
	}
}

void LogUtils::flatten_binary_log(char** pLogBuf, char** pOutBuf)
{
	CHAR* inputlog = *pLogBuf;
	uiMaxNoOfParameters = (UINT)typesList.size();
	while (uiMyParameterNumber < uiMaxNoOfParameters) 
	{
		if (typesList[uiMyParameterNumber]->getStorageType() == "CLASSARRAY")
		{
			UINT uiNoItrations;
			memcpy(&uiNoItrations, *pLogBuf, sizeof(UINT));
			memcpy(*pOutBuf, *pLogBuf, typesList[uiMyParameterNumber]->getLength());
			UINT NoofParameters = messgeParamList[uiMyParameterNumber]->getChildParamValue();
			*pLogBuf += typesList[uiMyParameterNumber]->getLength();
			*pOutBuf += typesList[uiMyParameterNumber]->getLength();
			uiFlattenbinarylength += typesList[uiMyParameterNumber]->getLength();
			uiMyParameterNumber++;
			for (UINT it = 0; it < uiNoItrations; it++)
			{
				for (UINT parameterit = 0; parameterit < NoofParameters; parameterit++)
				{
					CopyParameterToBuffer(messgeParamList[uiMyParameterNumber + parameterit], typesList[uiMyParameterNumber + parameterit], pLogBuf, pOutBuf);
				}

			}
			if (uiaclParameterNumber == 0)
			{
				uiMyParameterNumber += NoofParameters;
			}
			else
			{
				uiMyParameterNumber += NoofParameters + uiaclParameterNumber;
			}
		}
		else if (typesList[uiMyParameterNumber]->getStorageType() != "CLASS")
		{
			CopyParameterToBuffer(messgeParamList[uiMyParameterNumber], typesList[uiMyParameterNumber], pLogBuf, pOutBuf);
			uiMyParameterNumber++;
		}
		else
		{
			uiMyParameterNumber++;
		}
	}
	*pLogBuf = inputlog;
}
UINT LogUtils::getFlattenbinarylength(void)
{
	return uiFlattenbinarylength;
}

void LogUtils::CopyParameterToJson(std::string szElementname, CTypes* types, CEnums* enums, char* pLogBuf, nlohmann::ordered_json* obj)
{
	if (types->getStorageTypeEnum() == SIMPLE || types->getStorageType() == "ENUM")
	{
		if (types->getLength() == 4 && (types->getTypeName() != "FLOAT" && 
			types->getStorageType() != "ENUM"))
		{
         if (types->getTypeName() == "UINT" || types->getTypeName() == "ULONG" || types->getTypeName() == "MessageIdType")
         {
            UINT UINT_value;
            memcpy(&UINT_value, pLogBuf, types->getLength());
            obj->push_back({ szElementname.substr(szElementname.find('.') + 1,szElementname.length()), UINT_value });

         }
         else if (types->getTypeName() == "INT" || types->getTypeName() == "LONG")
         {
            INT INT_value;
            memcpy(&INT_value, pLogBuf, types->getLength());
            obj->push_back({ szElementname.substr(szElementname.find('.') + 1,szElementname.length()), INT_value });

         }
         else if (types->getTypeName() == "HEXULONG")
         {
            ULONG long_value;
            memcpy(&long_value, pLogBuf, types->getLength());
            std::stringstream stream;
            stream << std::hex  << long_value;
            obj->push_back({ szElementname.substr(szElementname.find('.') + 1,szElementname.length()), stream.str() });

         }
         else if (types->getTypeName() == "BOOL")
         {
            UINT Bool_value;
            memcpy(&Bool_value, pLogBuf, types->getLength());
            string bool_str;
            Bool_value == 1 ? bool_str = "TRUE" : bool_str = "FALSE";
            obj->push_back({ szElementname.substr(szElementname.find('.') + 1,szElementname.length()), bool_str });

         }
		}
		else if (types->getLength() == 4 && (types->getTypeName() != "FLOAT" &&
			types->getStorageType() == "ENUM"))
		{
			UINT UINT_value;
			std::string Enumname;
			memcpy(&UINT_value, pLogBuf, types->getLength());
			Enumname=enums->getEnumNameByValue(UINT_value);
			obj->push_back({ szElementname.substr(szElementname.find('.') + 1,szElementname.length()), Enumname });
		}
		else if (types->getLength() == 4 && types->getTypeName() == "FLOAT")
		{
			float float_value;
			memcpy(&float_value, pLogBuf, types->getLength());
			obj->push_back({ szElementname.substr(szElementname.find('.') + 1,szElementname.length()), float_value });
		}
		else if (types->getLength() == 8)
		{
			double DOU_value;
			memcpy(&DOU_value, pLogBuf, types->getLength());
			obj->push_back({ szElementname.substr(szElementname.find('.') + 1,szElementname.length()), DOU_value });
		}
      else if (types->getLength() == 2)
      {
         if (types->getTypeName() == "USHORT")
         {
            USHORT ushort_value;
            memcpy(&ushort_value, pLogBuf, types->getLength());
            obj->push_back({ szElementname.substr(szElementname.find('.') + 1,szElementname.length()), ushort_value });
         }
         else
         {
            SHORT short_value;
            memcpy(&short_value, pLogBuf, types->getLength());
            obj->push_back({ szElementname.substr(szElementname.find('.') + 1,szElementname.length()), short_value });

         }
      }
		else if (types->getLength() == 1)
		{
			CHAR char_value;
			memcpy(&char_value, pLogBuf, types->getLength());
			obj->push_back({ szElementname.substr(szElementname.find('.') + 1,szElementname.length()), char_value });

		}
	}
	else if (types->getStorageTypeEnum() == FIXEDARRAY || types->getStorageTypeEnum() == VARARRAY)
	{
		UINT arraylength = 0;
		if (types->getStorageTypeEnum() == FIXEDARRAY)
		{
			arraylength = types->getArrayLength();
		}
		else
		{
			memcpy(&arraylength, pLogBuf,4);
			pLogBuf += 4;
		}
		if (types->getLength() == 4 && types->getTypeName() != "FLOAT")
		{
			UINT UINT_value ;
			UINT offset_var = 0;
			json loc_obj = json::array();
			for (UINT index = 0; index < arraylength; index++)
			{
				memcpy(&UINT_value, pLogBuf+ offset_var, types->getLength());
				loc_obj[index] = UINT_value;
				offset_var += types->getLength();
			}
			obj->push_back({ szElementname.substr(szElementname.find('.') + 1,szElementname.length()),loc_obj });
		}
		else if (types->getLength() == 4 && types->getBaseTypeName() == "FLOAT")
		{
			FLOAT float_value;
			UINT offset_var = 0;
			json loc_obj = json::array();
			for (UINT index = 0; index < arraylength; index++)
			{
				memcpy(&float_value, pLogBuf+ offset_var, types->getLength());
				loc_obj[index] = float_value;
				offset_var += types->getLength();
			}
			obj->push_back({ szElementname.substr(szElementname.find('.') + 1,szElementname.length()), loc_obj });
		}
      else if (types->getLength() == 2)
      {
         USHORT ushort_value;
         UINT offset_var = 0;
         json loc_obj = json::array();
         for (UINT index = 0; index < arraylength; index++)
         {
            memcpy(&ushort_value, pLogBuf + offset_var, types->getLength());
            loc_obj[index] = ushort_value;
            offset_var += types->getLength();
         }
         obj->push_back({ szElementname.substr(szElementname.find('.') + 1,szElementname.length()), loc_obj });
      }
		else if (types->getLength() == 8)
		{
			DOUBLE DOU_value;
			UINT offset_var = 0;
			json loc_obj = json::array();
			for (UINT index = 0; index < arraylength; index++)
			{
				memcpy(&DOU_value, pLogBuf+ offset_var, types->getLength());
				loc_obj[index] = DOU_value;
				offset_var += types->getLength();
			}
			obj->push_back({ szElementname.substr(szElementname.find('.') + 1,szElementname.length()), loc_obj });
		}
		else if (types->getLength() == 1 )
		{
			if(types->getTypeName() == "FixedCharArray")
			{
				std::string CharArray="";
				for (UINT index = 0; index < arraylength; index++)
				{
					if (pLogBuf[index] != '\0')
					{
						CharArray += pLogBuf[index];
					}
				}
				obj->push_back({ szElementname.substr(szElementname.find('.') + 1,szElementname.length()), CharArray });
			}
			else
			{
				json loc_obj = json::array();
				for (UINT index = 0; index < arraylength; index++)
				{
					loc_obj[index] = pLogBuf[index];
				}
				obj->push_back({ szElementname.substr(szElementname.find('.') + 1,szElementname.length()), loc_obj });
			}
		}

	}
}

std::string LogUtils::getJSONString(char** pLogBuf)
{
   uiMaxNoOfParameters = 0;
   uiMyParameterNumber = 0;
   uiaclParameterNumber = 0;
   nlohmann::ordered_json message = nlohmann::ordered_json::object();
   json loc_obj = json::array();
   nlohmann::ordered_json obj = nlohmann::ordered_json::object();
   string aclName = "";
	uiMaxNoOfParameters = (UINT)typesList.size();
	while (uiMyParameterNumber < uiMaxNoOfParameters)
	{
		if (typesList[uiMyParameterNumber]->getStorageType() == "CLASSARRAY")
		{
			UINT uiNoItrations;
			memcpy(&uiNoItrations, (*pLogBuf)+offset, sizeof(UINT));
			UINT NoofParameters = messgeParamList[uiMyParameterNumber]->getChildParamValue();
			offset += typesList[uiMyParameterNumber]->getLength();
			uiMyParameterNumber++;
			for (UINT it = 0; it < uiNoItrations; it++)
			{				
				for (UINT parameterit = 0; parameterit < NoofParameters; parameterit++)
				{
					CheckParameterType(messgeParamList[uiMyParameterNumber + parameterit], typesList[uiMyParameterNumber + parameterit],
						enumsList[uiMyParameterNumber + parameterit], (*pLogBuf), &obj);
				}
				if (obj.size() != 0)
				{
               jsonarray_obj.push_back(obj);
					obj.clear();
				}
			}
         aclName = messgeParamList[uiMyParameterNumber - 1]->getElementname();
         message.push_back({ aclName.substr(aclName.find('.') + 1,aclName.length()),jsonarray_obj });
			if (uiaclParameterNumber == 0)
			{
				uiMyParameterNumber += NoofParameters;
			}
			else
			{
				uiMyParameterNumber += NoofParameters + uiaclParameterNumber;
			}
		}
		else if (typesList[uiMyParameterNumber]->getStorageType() != "CLASS")
		{
			CheckParameterType(messgeParamList[uiMyParameterNumber], typesList[uiMyParameterNumber],
				enumsList[uiMyParameterNumber], (*pLogBuf), &obj);
         message.update(obj);
			obj.clear();
			uiMyParameterNumber++;
		}
		else
		{
			uiMyParameterNumber++;
		}
		
	}
	return message.dump(-1,' ',false,json::error_handler_t::replace);
}

void LogUtils::CheckParameterType(CMessageParams* messageparam, CTypes* types, CEnums* enums, CHAR* pLogBuf, nlohmann::ordered_json* obj)
{
	std::string szElementname = messageparam->getElementname();
	switch (types->getStorageTypeEnum())
	{
	case SIMPLE:
	case ENUM:
		CopyParameterToJson(szElementname, types, enums, ((pLogBuf) + offset), obj);
		offset += types->getLength();
		break;
	case FIXEDARRAY:
		CopyParameterToJson(szElementname, types, enums, ((pLogBuf)+offset), obj);
		offset += types->getLength()*types->getArrayLength();
		break;
	case VARARRAY:
		UINT vArrayLength;
		memcpy(&vArrayLength, ((pLogBuf) + offset), sizeof(UINT));
		CopyParameterToJson(szElementname, types, enums, ((pLogBuf)+offset), obj);
		offset += vArrayLength * types->getLength();
		break;
	case STRING:
	{
		string szValue = "";
		for (UINT szSize = 0; szSize < types->getArrayLength(); szSize++)
		{
			CHAR* cbyte = (CHAR*)(pLogBuf + offset);

			if ((*cbyte) != '\0')
			{
				szValue += (*cbyte);
				offset += types->getLength();
			}
			else
			{
				offset += 4 - (szSize % 4);
				(*obj).push_back({ szElementname.substr(szElementname.find('.') + 1,szElementname.length()),szValue });
				break;
			}

		}
		break;
	}
	case CLASSARRAY:
	{
		UINT uiNoItrations;
		memcpy(&uiNoItrations, pLogBuf+offset, sizeof(UINT));
		UINT NoofParameters = messgeParamList[uiMyParameterNumber]->getChildParamValue();
		offset += typesList[uiMyParameterNumber]->getLength();
		UINT parameter = uiMyParameterNumber + 1;
		for (UINT it = 0; it < uiNoItrations; it++)
		{
			for (UINT parameterit = 0; parameterit < NoofParameters; parameterit++)
			{
				CheckParameterType(messgeParamList[parameter + parameterit], typesList[parameter + parameterit],
					enumsList[parameter + parameterit], (pLogBuf), obj);
			}
			jsonarray_obj.push_back(*obj);
			(*obj).clear();
		}
		uiaclParameterNumber = NoofParameters;
	}
		break;
	default:
		break;
	}
}
