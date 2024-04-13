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
// ! \file dynamic_parser.cpp
// ===============================================================================

#include <stdio.h>
#include <stdlib.h>

#include <chrono>

#include "src/decoders/dynamic_library/api/common_jsonreader.hpp"
#include "src/decoders/dynamic_library/api/novatel_filter.hpp"
#include "src/decoders/dynamic_library/api/novatel_parser.hpp"
#include "src/hw_interface/stream_interface/api/inputfilestream.hpp"
#include "src/hw_interface/stream_interface/api/outputfilestream.hpp"
#include "src/version.h"

using namespace std;
using namespace novatel::edie;
using namespace novatel::edie::oem;

inline bool file_exists(const std::string& name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

int main(int argc, char* argv[])
{
    // This example uses the default logger config, but you can also pass a config file to InitLogger()
    // Example config file: logger\example_logger_config.toml
    Logger::InitLogger();
    std::shared_ptr<spdlog::logger> pclLogger = Logger::RegisterLogger("dynamic_parser");
    pclLogger->set_level(spdlog::level::debug);
    Logger::AddConsoleLogging(pclLogger);
    Logger::AddRotatingFileLogger(pclLogger);

    // Get command line arguments
    pclLogger->info("Decoder library information:\n{}", caPrettyPrint);

    std::string sEncodeFormat = "ASCII";
    if (argc == 2 && strcmp(argv[1], "-V") == 0) { return 0; }
    if (argc < 3)
    {
        pclLogger->error("ERROR: Need to specify a JSON message definitions DB, an input file and an output "
                         "format.");
        pclLogger->error("Example: converter.exe <path to Json DB> <path to input file> <output format>");
        return 1;
    }
    if (argc == 4) { sEncodeFormat = argv[3]; }

    // Check command line arguments
    std::string sJsonDB = argv[1];
    if (!file_exists(sJsonDB))
    {
        pclLogger->error("File \"{}\" does not exist", sJsonDB);
        return 1;
    }

    std::string sInFilename = argv[2];
    if (!file_exists(sInFilename))
    {
        pclLogger->error("File \"{}\" does not exist", sInFilename);
        return 1;
    }

    ENCODEFORMAT eEncodeFormat = StringToEncodeFormat(sEncodeFormat);
    if (eEncodeFormat == ENCODEFORMAT::UNSPECIFIED)
    {
        pclLogger->error("Unspecified output format.\n\tASCII\n\tBINARY\n\tFLATTENED_BINARY");
        return 1;
    }

    pclLogger->info("Loading Database...");
    auto tStart = chrono::high_resolution_clock::now();
    JsonReader* pclJsonDb = common_jsonreader_init();
    common_jsonreader_load_file(pclJsonDb, sJsonDB.c_str());
    pclLogger->info("Done in {}ms", chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - tStart).count());

    // Setup timers
    auto tLoop = chrono::high_resolution_clock::now();
    uint32_t uiCounter = 0;

    // Setup the EDIE components
    Parser* pclParser = novatel_parser_init(pclJsonDb);
    pclParser->SetEncodeFormat(eEncodeFormat);

    Filter* pclFilter = novatel_filter_init();
    novatel_filter_set_logger_level(pclFilter, static_cast<uint32_t>(spdlog::level::debug));

    // Initialize structures and error codes
    STATUS eStatus = STATUS::UNKNOWN;

    MetaDataStruct stMetaData;
    MessageDataStruct stMessageData;

    novatel_parser_set_filter(pclParser, pclFilter);

    // Initialize FS structures and buffers
    StreamReadStatus stReadStatus;
    ReadDataStructure stReadData;
    unsigned char acIFSReadBuffer[MAX_ASCII_MESSAGE_LENGTH];
    stReadData.cData = reinterpret_cast<char*>(acIFSReadBuffer);
    stReadData.uiDataSize = sizeof(acIFSReadBuffer);

    // Setup filestreams
    InputFileStream clIFS(sInFilename.c_str());
    OutputFileStream clConvertedLogsOFS(sInFilename.append(".").append(sEncodeFormat).c_str());
    OutputFileStream clUnknownBytesOFS(sInFilename.append(".UNKNOWN").c_str());

    uint32_t uiCompleteMessages = 0;
    tStart = chrono::high_resolution_clock::now();
    tLoop = chrono::high_resolution_clock::now();
    while (!stReadStatus.bEOS)
    {
        stReadData.cData = reinterpret_cast<char*>(acIFSReadBuffer);
        stReadStatus = clIFS.ReadData(stReadData);
        novatel_parser_write(pclParser, reinterpret_cast<unsigned char*>(stReadData.cData), stReadStatus.uiCurrentStreamRead);

        do {
            eStatus = novatel_parser_read(pclParser, &stMessageData, &stMetaData);

            if (eStatus == STATUS::SUCCESS)
            {
                clConvertedLogsOFS.WriteData(reinterpret_cast<char*>(stMessageData.pucMessage), stMessageData.uiMessageLength);
                stMessageData.pucMessage[stMessageData.uiMessageLength] = '\0';
                pclLogger->info("Encoded: ({}) {}", stMessageData.uiMessageLength, reinterpret_cast<char*>(stMessageData.pucMessage));
                uiCompleteMessages++;
            }

            if (chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - tLoop).count() > 1000)
            {
                uiCounter++;
                pclLogger->info("{} logs/s", uiCompleteMessages / uiCounter);
                tLoop = chrono::high_resolution_clock::now();
            }
        } while (eStatus != STATUS::BUFFER_EMPTY);
    }
    pclLogger->info("Converted {} logs in {}s from {}", uiCompleteMessages,
                    (chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - tStart).count() / 1000.0),
                    sInFilename.c_str());

    Logger::Shutdown();
    novatel_parser_delete(pclParser);
    return 0;
}
