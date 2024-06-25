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

#include <string>

#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);

    if (argc != 2) { throw std::invalid_argument("1 argument required.\nUsage: <project root>"); }

    std::string strResourceVar = "TEST_RESOURCE_PATH=" + std::string(argv[1]) + "/src/hw_interface/stream_interface/test/resources/";

    if (putenv(const_cast<char*>(strResourceVar.c_str())) != 0) { throw std::runtime_error("Failed to set resource path."); }

    return RUN_ALL_TESTS();
}
