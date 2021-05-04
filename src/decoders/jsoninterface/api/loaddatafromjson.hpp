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

/*! \file loaddatafromjson.hpp
 *  \brief Class to read the JSON message definition file
 *
 */

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef LOADDATAFROMJSON_H
#define LOADDATAFROMJSON_H

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "jsonfilereader.h"

/*! \class loadDataFromJSON
 *  \brief Class with intrerface methods to read the json file
 * 
 */
class loadDataFromJSON
{
public:
   /*! Default constructor */
   loadDataFromJSON();
   /*! Default destructor */
   ~loadDataFromJSON();

   /*! \var static JSONFileReader *pReadDatabaseObj
    * \brief A static variable to hold JSONFileReader object instance
    */ 
   static JSONFileReader *pReadDatabaseObj;

   /*! \fn static JSONFileReader *getDatabaseObj(std::string strJSONFilePath = "")
    * \brief Method to get JSONFileReader instance
    * \param [in] strJSONFilePath Full Json file path
    * \return JSONFileReader instance
    */ 
   static JSONFileReader *getDatabaseObj(std::string strJSONFilePath = "");
#ifdef WIDE_CHAR_SUPPORT
   /*! \fn static JSONFileReader *getDatabaseObj(std::string strJSONFilePath = "")
    * \brief Method to get JSONFileReader instance
    * \param [in] strJSONFilePath Full Json file path with wide characters
    * \return JSONFileReader instance
    */ 
   static JSONFileReader *getDatabaseObj(std::wstring strJSONFilePath);
#endif
   /*! \fn static void DestroyDatabaseObj()
    * \brief A static method to delete the instance of JSONFileReader
    */ 
   static void DestroyDatabaseObj();
   /*! \fn static void initializeDatabaseObj()
    * \brief A static method to intialize the instance of JSONFileReader with NULL
    */    
   static void initializeDatabaseObj();
private:
};
#endif //LOADDATAFROMJSON_H
