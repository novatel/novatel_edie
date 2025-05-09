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
// ! \file main.cpp
// ===============================================================================

#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

#include "novatel_edie/common/logger.hpp"
#include "novatel_edie/common/test_utils/get_repo_path.hpp"

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    CPPLoggerManager* pclMyLoggerManager = GetLoggerManager();
    pclMyLoggerManager->InitLogger();

    std::filesystem::path pathRepoDir = GetRepoBasePath(argc, argv);

    std::filesystem::path pathDatabaseFile = pathRepoDir / "database" / "database.json";

    std::string strDatabaseVar = pathDatabaseFile.string();

#ifdef _WIN32
    if (_putenv_s("TEST_DATABASE_PATH", strDatabaseVar.c_str()) != 0) { throw std::runtime_error("Failed to set db path."); }
#else
    if (setenv("TEST_DATABASE_PATH", strDatabaseVar.c_str(), 1) != 0) { throw std::runtime_error("Failed to set db path."); }
#endif

    return RUN_ALL_TESTS();
}
