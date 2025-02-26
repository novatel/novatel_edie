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

#include "novatel_edie/decoders/common/common.hpp"
#include "novatel_edie/decoders/common/json_db_reader.hpp"

using namespace novatel::edie;

class JsonDbReaderTest : public testing::Test
{
  public:
    void SetUp() override {}
    void TearDown() override {}
};

// -------------------------------------------------------------------------------------------------------
// JsonDbReader Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(JsonDbReaderTest, JsonDbReaderFailure)
{
    MessageDatabase clJson;
    ASSERT_THROW(LoadJsonDbFile(""), JsonDbReaderFailure);
}

TEST_F(JsonDbReaderTest, AppendJsonDbEnumerations)
{
    const std::string strId = "008451a05e1e7aa32c75119df950d405265e0904";

    auto clJson = std::make_shared<MessageDatabase>();
    AppendJsonDbEnumerations(clJson, std::filesystem::path(std::getenv("TEST_DATABASE_PATH")));

    EnumDefinition::ConstPtr pstEnumDef = clJson->GetEnumDefId(strId);
    ASSERT_NE(pstEnumDef, nullptr);
    ASSERT_EQ(pstEnumDef->name, "Datum");

    clJson->RemoveEnumeration("Datum", true);
    ASSERT_EQ(clJson->GetEnumDefId(strId), nullptr);
}

TEST_F(JsonDbReaderTest, AppendJsonDbMessages)
{
    constexpr uint32_t uiMsgId = 690;

    auto clJson = std::make_shared<MessageDatabase>();
    AppendJsonDbMessages(clJson, std::filesystem::path(std::getenv("TEST_DATABASE_PATH")));

    MessageDefinition::ConstPtr pstMsgDef = clJson->GetMsgDef(uiMsgId);
    ASSERT_NE(pstMsgDef, nullptr);
    ASSERT_EQ(pstMsgDef->name, "PASSAUX");

    clJson->RemoveMessage(uiMsgId, true);
    ASSERT_EQ(clJson->GetMsgDef(uiMsgId), nullptr);
}
