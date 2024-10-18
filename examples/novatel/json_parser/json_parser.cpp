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
// ! \file json_parser.cpp
// ===============================================================================

#include <chrono>
#include <filesystem>

#include <novatel_edie/common/logger.hpp>
#include <novatel_edie/decoders/oem/parser.hpp>
#include <novatel_edie/version.h>

namespace fs = std::filesystem;

using namespace novatel::edie;
using namespace novatel::edie::oem;

int main(int argc, char* argv[])
{
    // This example uses the default logger config, but you can also pass a config file to InitLogger()
    // Example config file: logger\example_logger_config.toml
    Logger::InitLogger();
    std::shared_ptr<spdlog::logger> pclLogger = Logger::RegisterLogger("json_parser");
    pclLogger->set_level(spdlog::level::debug);
    Logger::AddConsoleLogging(pclLogger);
    Logger::AddRotatingFileLogger(pclLogger);

    // Get command line arguments
    pclLogger->info("Decoder library information:\n{}", caPrettyPrint);

    std::string sEncodeFormat = "ASCII";
    if ((argc == 2) && (strcmp(argv[1], "-V") == 0)) { return 0; }
    if (argc < 3)
    {
        pclLogger->error("ERROR: Need to specify a JSON message definitions DB, an input file and an output format.");
        pclLogger->error("Example: converter <path to Json DB> <path to input file> <output format>");
        return 1;
    }
    if (argc == 4 || argc == 5) { sEncodeFormat = argv[3]; }

    std::string sAppendMsg;
    if (argc == 5) { sAppendMsg = argv[4]; }

    // Check command line arguments
    const fs::path pathJsonDb = argv[1];
    if (!fs::exists(pathJsonDb))
    {
        pclLogger->error("File \"{}\" does not exist", pathJsonDb.string());
        return 1;
    }
    const fs::path pathInFilename = argv[2];
    if (!fs::exists(pathInFilename))
    {
        pclLogger->error("File \"{}\" does not exist", pathInFilename.string());
        return 1;
    }

    ENCODE_FORMAT eEncodeFormat = StringToEncodeFormat(sEncodeFormat);
    if (eEncodeFormat == ENCODE_FORMAT::UNSPECIFIED)
    {
        pclLogger->error("Unspecified output format.\n\tASCII\n\tBINARY\n\tFLATTENED_BINARY");
        return 1;
    }

    // Load the database
    auto clJsonDb = std::make_shared<JsonReader>();
    pclLogger->info("Loading Database...");
    auto tStart = std::chrono::high_resolution_clock::now();
    clJsonDb->LoadFile(pathJsonDb.string());
    pclLogger->info("Done in {}ms",
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tStart).count());

    pclLogger->info("Appending Message...");
    tStart = std::chrono::high_resolution_clock::now();
    clJsonDb->AppendMessages(sAppendMsg);
    pclLogger->info("Done in {}ms",
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tStart).count());

    // Set up timers
    auto tLoop = std::chrono::high_resolution_clock::now();

    Parser clParser(clJsonDb);
    clParser.SetEncodeFormat(eEncodeFormat);
    clParser.SetLoggerLevel(spdlog::level::debug);
    Logger::AddConsoleLogging(clParser.GetLogger());
    Logger::AddRotatingFileLogger(clParser.GetLogger());

    auto clFilter = std::make_shared<Filter>();
    clFilter->SetLoggerLevel(spdlog::level::debug);
    Logger::AddConsoleLogging(clFilter->GetLogger());
    Logger::AddRotatingFileLogger(clFilter->GetLogger());

    // Initialize structures
    MetaDataStruct stMetaData;
    MessageDataStruct stMessageData;

    clParser.SetFilter(clFilter);

    std::array<char, MAX_ASCII_MESSAGE_LENGTH> cData;

    // Set up file streams
    std::ifstream ifs(pathInFilename, std::ios::binary);
    std::ofstream convertedOfs(pathInFilename.string() + "." + sEncodeFormat, std::ios::binary);
    std::ofstream unknownOfs(pathInFilename.string() + "." + sEncodeFormat + ".UNKNOWN", std::ios::binary);

    uint32_t uiCompleteMessages = 0;
    uint32_t uiCounter = 0;

    tStart = std::chrono::high_resolution_clock::now();
    tLoop = std::chrono::high_resolution_clock::now();

    while (!ifs.eof())
    {
        ifs.read(cData.data(), cData.size());
        clParser.Write(reinterpret_cast<unsigned char*>(cData.data()), ifs.gcount());

        STATUS eStatus = clParser.Read(stMessageData, stMetaData);

        while (eStatus != STATUS::BUFFER_EMPTY)
        {
            if (eStatus == STATUS::SUCCESS)
            {
                convertedOfs.write(reinterpret_cast<char*>(stMessageData.pucMessage), stMessageData.uiMessageLength);
                stMessageData.pucMessage[stMessageData.uiMessageLength] = '\0';
                pclLogger->info("Encoded: ({}) {}", stMessageData.uiMessageLength, reinterpret_cast<char*>(stMessageData.pucMessage));
                uiCompleteMessages++;
            }

            if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tLoop).count() > 1000)
            {
                uiCounter++;
                pclLogger->info("{} logs/s", uiCompleteMessages / uiCounter);
                tLoop = std::chrono::high_resolution_clock::now();
            }

            eStatus = clParser.Read(stMessageData, stMetaData);
        }
    }

    pclLogger->info("Converted {} logs in {}ms from {}", uiCompleteMessages,
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tStart).count(),
                    pathInFilename.string().c_str());
    Logger::Shutdown();
    return 0;
}
