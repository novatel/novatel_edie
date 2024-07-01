// ===============================================================================
// |                                                                             |
// |  COPYRIGHT NovAtel Inc, 2022. All rights reserved.                          |
// |                                                                             |
// |  Permission is hereby granted, free of charge, to any person obtaining a    |
// |  copy of this software and associated documentation files (the "Software"), |
// |  to deal in the Software without restriction, including without limitation  |
// |  the rights to use, copy, modify, merge, publish, distribute, sublicense,   |
// |  and/or sell copies of the Software, and to permit persons to whom the      |
// |  Software is furnished to do so, subject to the following conditions:       |
// |                                                                             |
// |  The above copyright notice and this permission notice shall be included    |
// |  in all copies or substantial portions of the Software.                     |
// |                                                                             |
// |  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR |
// |  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   |
// |  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    |
// |  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER |
// |  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    |
// |  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        |
// |  DEALINGS IN THE SOFTWARE.                                                  |
// |                                                                             |
// ===============================================================================
// ! \file json_reader_unit_test.cpp
// ===============================================================================

#include <filesystem>

#include <gtest/gtest.h>

#include "novatel_edie/common/common.hpp"
#include "novatel_edie/common/json_reader.hpp"

using namespace novatel::edie;

class JsonReaderTest : public testing::Test
{
  public:
    void SetUp() override {}
    void TearDown() override {}
};

// -------------------------------------------------------------------------------------------------------
// JsonReader Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(JsonReaderTest, JsonReaderFailure)
{
    JsonReader clJson;
    ASSERT_THROW(clJson.LoadFile<std::string>(""), JsonReaderFailure);
}

TEST_F(JsonReaderTest, AppendEnumerations)
{
    const std::string strId = "008451a05e1e7aa32c75119df950d405265e0904";

    JsonReader clJson;
    clJson.AppendEnumerations(std::filesystem::path(std::getenv("TEST_DATABASE_PATH")).string());

    const EnumDefinition* pstEnumDef = clJson.GetEnumDefId(strId);
    ASSERT_NE(pstEnumDef, nullptr);
    ASSERT_EQ(pstEnumDef->name, "Datum");

    clJson.RemoveEnumeration("Datum", true);
    ASSERT_EQ(clJson.GetEnumDefId(strId), nullptr);
}

TEST_F(JsonReaderTest, AppendMessages)
{
    constexpr uint32_t uiMsgId = 690;

    JsonReader clJson;
    clJson.AppendMessages(std::filesystem::path(std::getenv("TEST_DATABASE_PATH")).string());

    const MessageDefinition* pstMsgDef = clJson.GetMsgDef(uiMsgId);
    ASSERT_NE(pstMsgDef, nullptr);
    ASSERT_EQ(pstMsgDef->name, "PASSAUX");

    clJson.RemoveMessage(uiMsgId, true);
    ASSERT_EQ(clJson.GetMsgDef(uiMsgId), nullptr);
}
