////////////////////////////////////////////////////////////////////////
//
// COPYRIGHT NovAtel Inc, 2022. All rights reserved.
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
////////////////////////////////////////////////////////////////////////
//                            DESCRIPTION
//
//! \file main.cpp
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include <vector>
#include <filesystem>
#include <string>
#include <iostream>

#include "gtest/gtest.h"
#include "paths.hpp"

const std::string* TEST_DB_PATH;
const std::string* TEST_RESOURCE_PATH;

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    if (argc != 3)
    {
       throw::std::runtime_error("2 arguments required.\nUsage: <db path> <resource path>");
    }

    const std::vector<std::string> args(argv + 1, argv + argc);

    if (!std::filesystem::exists(args[0])){
       throw::std::runtime_error("\"" + args[0] + "\" does not exist");
    }
    if (!std::filesystem::is_directory(args[1])){
       throw::std::runtime_error("\"" + args[1] + "\" does not exist");
    }

    TEST_DB_PATH = &args[0];
    TEST_RESOURCE_PATH = &args[1];

    return RUN_ALL_TESTS();
}
