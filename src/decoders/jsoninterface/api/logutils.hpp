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

/*! \file logutils.hpp
 *  \brief Class to do provide the log processing functionality
 *
 */

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef LOGUTILS_H
#define LOGUTILS_H

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include <lib/nlohmann/json.hpp>
#include <cstdlib>
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <stdio.h>
#include "loaddatafromjson.hpp"

//-----------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------
/*! Namespace std */
using namespace std;

/*! Variable to use json object from nlohmann libray */
using json = nlohmann::json;

/*! \class LogUtils
 *  \brief Class with methods to process the log using json definition file
 * 
 */
class LogUtils
{
public:
   /*! A constructor
    * \brief 
	* \param [in] dbData JSONFileReader object
	* \param [in] strMsgName Name of the message
	*/
   LogUtils(JSONFileReader* dbData, std::string strMsgName);
	
   /*! Default destructor */
   ~LogUtils();

   /*! \fn void LoadLogs(JSONFileReader* dbData, std::string strMsgName)
    * \brief 
	* \param [in] dbData JSONFileReader object
	* \param [in] strMsgName Name of the message
	*/
   void LoadLogs(JSONFileReader* dbData, std::string strMsgName);

   /*! \fn void flatten_binary_log(CHAR** pLogBuf, CHAR** pOutBuf)
    * \brief Convert binary log to flaten binary log
	* \param [in] pLogBuf Decoded log 
	* \param [in] pOutBuf Ouputed flatten binary buffer
	*/	
   void flatten_binary_log(CHAR** pLogBuf, CHAR** pOutBuf);

   /*! \fn INT getFlattenbinarylength(void)
    * \brief Method to get flatten binary log length
	* \return Length of the flatten binary log
	*/	
   UINT getFlattenbinarylength(void);

   /*! \fn std::string getJSONString(CHAR** pLogBuf)
    * \brief 
	* \param [in] pLogBuf
	* \return Json string of given log 
	*/
   std::string getJSONString(CHAR** pLogBuf);

private:
   /*! \var UINT uiMaxNoOfParameters
    * \brief 
	*/ 
   UINT uiMaxNoOfParameters;

   /*! \var UINT uiMyParameterNumber
    * \brief 
	*/    
   UINT uiMyParameterNumber;

   /*! \var UINT uiaclParameterNumber
    * \brief 
	*/   
   UINT uiaclParameterNumber;

   /*! \var UINT offset
    * \brief 
	*/    
   UINT offset;

   /*! UINT uiFlattenbinarylength
    * \brief Length of the flatten binary log length
	*/ 
   UINT uiFlattenbinarylength;

   /*! json jsonarray_obj
    * \brief Json object form nlohmann library
	*/ 
   nlohmann::ordered_json jsonarray_obj;
   
   /*! Vector of MessageParams class objects */
   vector<CMessageParams*> messgeParamList; 
   /*! Vector of MessageParams class objects */
   vector<CTypes*> typesList; 
   /*! Vector of Enums class objects */
   vector<CEnums*> enumsList; 
   /*! Vector of Message class objects */
   vector<CMessage*> messgeList;

   /*! \fn void CheckParameterType(CMessageParams* messageparam, CTypes* types, CEnums* enums, CHAR* pLogBuf, json* obj)
    * \brief
	* \param [in] messageparam
	* \param [in] types
	* \param [in] enums
	* \param [in] pLogBuf
	* \param [in] obj
	*/
   void CheckParameterType(CMessageParams* messageparam, CTypes* types, CEnums* enums, CHAR* pLogBuf, nlohmann::ordered_json* obj);

   /*! \fn void CopyParameterToJson(std::string szElementname, CTypes* types, CEnums* enums, CHAR* pLogBuf, json* obj)
    * \brief
	* \param [in] szElementname
	* \param [in] types
	* \param [in] enums
	* \param [in] pLogBuf
	* \param [in] obj
	*/
   void CopyParameterToJson(std::string szElementname, CTypes* types, CEnums* enums, CHAR* pLogBuf, nlohmann::ordered_json* obj);

   /*! \fn void CopyParameterToBuffer(CMessageParams* messageparam, CTypes* types, CHAR** pLogBuf, CHAR** pOutBuf)
    * \brief Copy all the Message parameters to buffer
	* \param [in] messageparam
	* \param [in] types
	* \param [in] pLogBuf
	* \param [in] pOutBuf
	*/
   void CopyParameterToBuffer(CMessageParams* messageparam, CTypes* types, CHAR** pLogBuf, CHAR** pOutBuf);
	
};
#endif // LOGUTILS_H
