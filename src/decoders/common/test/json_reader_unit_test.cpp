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

TEST_F(JsonDbReaderTest, AppendEnumerations)
{
    const std::string strId = "2630f62f9ca0a9e4bcccef50ffbaba8f920c4439";

    auto clJson = std::make_shared<MessageDatabase>();
    clJson->AppendEnumerations(LoadJsonDbFile(std::filesystem::path(std::getenv("TEST_DATABASE_PATH")))->EnumDefinitions());

    EnumDefinition::ConstPtr pstEnumDef = clJson->GetEnumDefId(strId);
    ASSERT_NE(pstEnumDef, nullptr);
    ASSERT_EQ(pstEnumDef->name, "Adjust1PPSMode");

    clJson->RemoveEnumeration("Adjust1PPSMode");
    ASSERT_EQ(clJson->GetEnumDefId(strId), nullptr);
}

TEST_F(JsonDbReaderTest, AppendBitMasks)
{
    const std::string strId = "7977f11493b08295e9a5692b992646b1b1438b7d";
    const std::string strName = "ExtendedSolutionStatus";

    auto clJson = std::make_shared<MessageDatabase>();
    clJson->AppendBitMasks(LoadJsonDbFile(std::filesystem::path(std::getenv("TEST_DATABASE_PATH")))->BitMasks());

    BitMaskMap::ConstPtr pstBitMaskDef = clJson->GetBitMaskDefId(strId);
    ASSERT_NE(pstBitMaskDef, nullptr);
    ASSERT_EQ(pstBitMaskDef->name, strName);
    ASSERT_EQ(clJson->GetBitMaskDefName(strName), pstBitMaskDef);
    ASSERT_EQ(pstBitMaskDef->masks.at("pseudorange_inno_correction").bitfield, BitMask::fromRange(1, 4));

    clJson->RemoveBitMask(strName);
    ASSERT_EQ(clJson->GetBitMaskDefId(strId), nullptr);
    ASSERT_EQ(clJson->GetBitMaskDefName(strName), nullptr);
}

TEST_F(JsonDbReaderTest, BitMaskFieldResolution)
{
    const std::string strName = "ExtendedSolutionStatus";
    const auto getExtSolStat = [](const MessageDatabase& db_) {
        const auto pstMsgDef = db_.GetMsgDef("BESTPOS");
        return pstMsgDef->GetMsgDefFromCrc(pstMsgDef->latestMessageCrc).GetFieldDefByName("ext_sol_stat");
    };

    const auto clLoaded = LoadJsonDbFile(std::filesystem::path(std::getenv("TEST_DATABASE_PATH")));
    auto clJson = std::make_shared<MessageDatabase>();
    clJson->AppendMessages(clLoaded->MessageDefinitions());
    ASSERT_EQ(getExtSolStat(*clJson)->bitMasks, nullptr);

    clJson->AppendBitMasks(clLoaded->BitMasks());
    ASSERT_EQ(getExtSolStat(*clJson)->bitMasks, clJson->GetBitMaskDefName(strName));

    clJson->RemoveBitMask(strName);
    ASSERT_EQ(getExtSolStat(*clJson)->bitMasks, nullptr);
}

TEST_F(JsonDbReaderTest, BitMaskChangesLeaveEnumFieldsUnchanged)
{
    const auto getSolutionStatus = [](const MessageDatabase& db_) {
        const auto pstMsgDef = db_.GetMsgDef("BESTPOS");
        const auto pstField = pstMsgDef->GetMsgDefFromCrc(pstMsgDef->latestMessageCrc).GetFieldDefByName("solution_status");
        return std::dynamic_pointer_cast<const EnumField>(pstField);
    };

    const auto clLoaded = LoadJsonDbFile(std::filesystem::path(std::getenv("TEST_DATABASE_PATH")));
    auto clJson = std::make_shared<MessageDatabase>();
    clJson->AppendEnumerations(clLoaded->EnumDefinitions());
    clJson->AppendMessages(clLoaded->MessageDefinitions());
    const EnumDefinition::ConstPtr pstSolStatus = getSolutionStatus(*clJson)->enumDef;
    ASSERT_NE(pstSolStatus, nullptr);

    clJson->RemoveEnumeration("SolStatus");
    clJson->AppendBitMasks(clLoaded->BitMasks());
    ASSERT_EQ(getSolutionStatus(*clJson)->enumDef, pstSolStatus);

    clJson->RemoveBitMask("ExtendedSolutionStatus");
    ASSERT_EQ(getSolutionStatus(*clJson)->enumDef, pstSolStatus);
}

TEST_F(JsonDbReaderTest, MergeBitMasks)
{
    const std::string strName = "ExtendedSolutionStatus";

    MessageDatabase clJson;
    clJson.Merge(*LoadJsonDbFile(std::filesystem::path(std::getenv("TEST_DATABASE_PATH"))));

    const BitMaskMap::ConstPtr pstBitMaskDef = clJson.GetBitMaskDefName(strName);
    ASSERT_NE(pstBitMaskDef, nullptr);

    const auto pstMsgDef = clJson.GetMsgDef("BESTPOS");
    ASSERT_EQ(pstMsgDef->GetMsgDefFromCrc(pstMsgDef->latestMessageCrc).GetFieldDefByName("ext_sol_stat")->bitMasks, pstBitMaskDef);
}

TEST_F(JsonDbReaderTest, AppendMessages)
{
    constexpr uint32_t uiMsgId = 690;

    auto clJson = std::make_shared<MessageDatabase>();
    clJson->AppendMessages(LoadJsonDbFile(std::filesystem::path(std::getenv("TEST_DATABASE_PATH")))->MessageDefinitions());

    MessageDefinition::ConstPtr pstMsgDef = clJson->GetMsgDef(uiMsgId);
    ASSERT_NE(pstMsgDef, nullptr);
    ASSERT_EQ(pstMsgDef->name, "PASSAUX");

    clJson->RemoveMessage(uiMsgId);
    ASSERT_EQ(clJson->GetMsgDef(uiMsgId), nullptr);
}
