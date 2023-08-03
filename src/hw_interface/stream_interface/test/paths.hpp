#pragma once

#include <string_view>

extern const std::string* TEST_RESOURCE_PATH;

#ifdef WIN32
#define _TEXT(quote) L##quote
#define _tcout std::wcout
#else
#define _TEXT(quote) u8##quote
#define _tcout std::cout
#endif
