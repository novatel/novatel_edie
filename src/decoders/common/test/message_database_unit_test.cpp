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
// ! \file message_database_unit_test.cpp
// ===============================================================================

#include <filesystem>

#include <gtest/gtest.h>

#include "novatel_edie/decoders/common/common.hpp"
#include "novatel_edie/decoders/common/message_database.hpp"

using namespace novatel::edie;

class MessageDatabaseTest : public testing::Test
{
  protected:
    std::shared_ptr<BaseField> f0;
    std::shared_ptr<BaseField> f1;
    static constexpr uint32_t kMsgCrc = 0x11223344U;

    void SetUp() override
    {
        MessageDatabase::RegisterAlignmentFunction("OEM", [](const size_t size, const uintptr_t start, const uintptr_t ptr) {
            size_t alignment = std::min(size_t{4}, size);
            size_t offset = (ptr - start) % alignment;
            return offset == 0 ? 0 : alignment - offset;
        });
        
        f0 = std::make_shared<BaseField>("short", FIELD_TYPE::SIMPLE, "%hu", DATA_TYPE::USHORT);
        f1 = std::make_shared<BaseField>("int", FIELD_TYPE::SIMPLE, "%d", DATA_TYPE::INT);
    }

    std::shared_ptr<MessageDefinition> CreateMessageDefinition(uint32_t logID, const std::string& name)
    {
        auto msgDef = std::make_shared<MessageDefinition>();
        msgDef->logID = logID;
        msgDef->name = name;
        const auto fieldInfo = BuildFieldInfo({f0, f1});
        msgDef->fieldInfo.emplace(kMsgCrc, fieldInfo);
        msgDef->latestMessageCrc = kMsgCrc;
        return msgDef;
    }
};

TEST_F(MessageDatabaseTest, NoAlignByDefault)
{
    auto msgDef = CreateMessageDefinition(123U, "TESTMSG");

    MessageDatabase db({msgDef}, {});
    auto retrievedMsgDef = db.GetMsgDef("TESTMSG");
    ASSERT_NE(retrievedMsgDef, nullptr);
    const auto retrievedFieldInfo = retrievedMsgDef->GetMsgDefFromCrc(kMsgCrc);
    ASSERT_EQ(retrievedFieldInfo.fixedFieldBytes, 6U);
    ASSERT_EQ(retrievedFieldInfo.varFieldCount, 0U);
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields.size(), 2U);
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[0]->name, "short");
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[1]->name, "int");
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[1]->index, 2U); // No alignment, int field starts immediately after the short field
}

TEST_F(MessageDatabaseTest, OEMAlign)
{
    auto msgDef = CreateMessageDefinition(123U, "TESTMSG");

    MessageDatabase db({msgDef}, {});
    db.SetMessageFamily("OEM");
    auto retrievedMsgDef = db.GetMsgDef("TESTMSG");
    ASSERT_NE(retrievedMsgDef, nullptr);
    const auto retrievedFieldInfo = retrievedMsgDef->GetMsgDefFromCrc(kMsgCrc);
    ASSERT_EQ(retrievedFieldInfo.fixedFieldBytes, 8U);
    ASSERT_EQ(retrievedFieldInfo.varFieldCount, 0U);
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields.size(), 2U);
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[0]->name, "short");
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[1]->name, "int");
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[1]->index, 4U); // OEM alignment
}

TEST_F(MessageDatabaseTest, AppendMessagesCopies)
{
    auto msgDef = CreateMessageDefinition(124U, "TESTMSG");

    MessageDatabase db({}, {});
    db.SetMessageFamily("OEM");
    db.AppendMessages({msgDef});

    auto retrievedMsgDef = db.GetMsgDef("TESTMSG");
    ASSERT_NE(retrievedMsgDef, nullptr);
    const auto retrievedFieldInfo = retrievedMsgDef->GetMsgDefFromCrc(kMsgCrc);
    ASSERT_EQ(retrievedFieldInfo.fixedFieldBytes, 8U);
    ASSERT_EQ(retrievedFieldInfo.varFieldCount, 0U);
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields.size(), 2U);
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[0]->name, "short");
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[1]->name, "int");
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[1]->index, 4U);          // OEM alignment
    ASSERT_EQ(msgDef->fieldInfo[kMsgCrc]->messageOrderedFields[1]->index, 2U); // Original message definition is unchanged
}

TEST_F(MessageDatabaseTest, MessageDefinitionsNotCopiedOnInit)
{
    auto msgDef = CreateMessageDefinition(124U, "TESTMSG");

    MessageDatabase db({msgDef}, {});

    auto retrievedMsgDef = db.GetMsgDef("TESTMSG");
    ASSERT_EQ(retrievedMsgDef, msgDef); // The message definition is not copied on init
}

TEST_F(MessageDatabaseTest, MessageDefinitionsCopiedOnMerge)
{
    auto msgDef1 = CreateMessageDefinition(123U, "TESTMSG1");
    auto msgDef2 = CreateMessageDefinition(124U, "TESTMSG2");

    MessageDatabase noAlignDb({msgDef1}, {});
    MessageDatabase oemDb({msgDef2}, {});
    oemDb.SetMessageFamily("OEM");
    oemDb.Merge(noAlignDb);
    auto retrievedMsgDef = noAlignDb.GetMsgDef("TESTMSG1");
    ASSERT_NE(retrievedMsgDef, nullptr);
    auto retrievedFieldInfo = retrievedMsgDef->GetMsgDefFromCrc(kMsgCrc);
    ASSERT_EQ(retrievedFieldInfo.fixedFieldBytes, 6U);
    ASSERT_EQ(retrievedFieldInfo.varFieldCount, 0U);
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields.size(), 2U);
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[0]->name, "short");
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[1]->name, "int");
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[1]->index, 2U); // No alignment, int field starts immediately after the short field

    retrievedMsgDef = oemDb.GetMsgDef("TESTMSG2");
    ASSERT_NE(retrievedMsgDef, nullptr);
    retrievedFieldInfo = retrievedMsgDef->GetMsgDefFromCrc(kMsgCrc);
    ASSERT_EQ(retrievedFieldInfo.fixedFieldBytes, 8U);
    ASSERT_EQ(retrievedFieldInfo.varFieldCount, 0U);
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields.size(), 2U);
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[0]->name, "short");
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[1]->name, "int");
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[1]->index, 4U);           // OEM alignment
    ASSERT_EQ(msgDef2->fieldInfo[kMsgCrc]->messageOrderedFields[1]->index, 2U); // Original message definition is unchanged
}
