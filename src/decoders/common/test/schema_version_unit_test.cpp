// ===============================================================================
// |                                                                             |
// |  COPYRIGHT NovAtel Inc, 2026. All rights reserved.                          |
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
// ! \file schema_version_unit_test.cpp
// ===============================================================================

#include <filesystem>

#include <gtest/gtest.h>

#include "novatel_edie/decoders/common/json_db_reader.hpp"

using namespace novatel::edie;

// Each resources/schema_*.json is a minimal database in the shape nov_uidb generates for that schema version.
class SchemaVersionTest : public testing::Test
{
  protected:
    static std::filesystem::path ResourcePath(const char* pcFileName_)
    {
        return std::filesystem::path(std::getenv("TEST_RESOURCE_PATH")) / pcFileName_;
    }

    static void AssertLoads(const char* pcFileName_)
    {
        MessageDatabase::Ptr pclDb;
        ASSERT_NO_THROW(pclDb = LoadJsonDbFile(ResourcePath(pcFileName_)));
        ASSERT_NE(pclDb->GetMsgDef("SAMPLE"), nullptr);
        ASSERT_NE(pclDb->GetEnumDefName("SampleEnum"), nullptr);
    }

    static void AssertThrows(const char* pcFileName_) { ASSERT_THROW(LoadJsonDbFile(ResourcePath(pcFileName_)), JsonDbReaderFailure); }
};

// -------------------------------------------------------------------------------------------------------
// Database Schema Version Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(SchemaVersionTest, LoadsSchema_1_0_0) { AssertLoads("schema_1_0_0.json"); }

TEST_F(SchemaVersionTest, LoadsSchema_1_1_0) { AssertLoads("schema_1_1_0.json"); }

TEST_F(SchemaVersionTest, LoadsSchema_2_0_0) { AssertLoads("schema_2_0_0.json"); }

TEST_F(SchemaVersionTest, LoadsSchema_3_0_0) { AssertLoads("schema_3_0_0.json"); }

TEST_F(SchemaVersionTest, RejectsSchema_4_0_0) { AssertThrows("schema_4_0_0.json"); }
