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

// Includes

#include "decoders/jsoninterface/api/loaddatafromjson.hpp"
#include <gtest/gtest.h>
#include <iostream>
#include <fstream>


#ifndef DATADIR
    #define DATADIR
#endif

class JsonInterFaceTest : public ::testing::Test {
public:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }

private:
};


TEST_F(JsonInterFaceTest, ReadMessageDefinition)
{
   std::list<INT> msgIdList;
   std::list<string> msgNameList; 
   std::string filename = std::string(DATADIR) + "dynamicdef.json";

   const char*	filepath = filename.c_str();
   loadDataFromJSON::initializeDatabaseObj();
   JSONFileReader* jsonReader = NULL;
   jsonReader = loadDataFromJSON::getDatabaseObj(filepath);

   unsigned int messageid = jsonReader->getMessageId("LOGLIST");
   ASSERT_DOUBLE_EQ(messageid, 5);

   std::string msgdefcrc = jsonReader->getMsgDefCRC(messageid);
   ASSERT_STREQ(msgdefcrc.c_str(), "c00c");

   std::string messagename = jsonReader->getMessageName(messageid, msgdefcrc);
   ASSERT_STREQ(messagename.c_str(), "LOGLIST");

   unsigned int paramcount = jsonReader->getNoOfParamas(messageid, msgdefcrc);
   ASSERT_DOUBLE_EQ(paramcount, 9);

   msgIdList = jsonReader->getMsgIdList();
   msgNameList = jsonReader->getMsgNameList();

   loadDataFromJSON::DestroyDatabaseObj();
}
