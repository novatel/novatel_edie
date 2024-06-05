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
// ! \file dynamic_components.cpp
// ===============================================================================

#include <filesystem>

#include "src/decoders/dynamic_library/api/common_json_reader.hpp"
#include "src/decoders/dynamic_library/api/novatel_encoder.hpp"
#include "src/decoders/dynamic_library/api/novatel_filter.hpp"
#include "src/decoders/dynamic_library/api/novatel_framer.hpp"
#include "src/decoders/dynamic_library/api/novatel_header_decoder.hpp"
#include "src/decoders/dynamic_library/api/novatel_message_decoder.hpp"
#include "src/hw_interface/stream_interface/api/inputfilestream.hpp"
#include "src/hw_interface/stream_interface/api/outputfilestream.hpp"
#include "src/version.h"

namespace fs = std::filesystem;

using namespace novatel::edie;
using namespace novatel::edie::oem;

int main(int argc, char* argv[])
{
    // This example uses the default logger config, but you can also pass a config file to InitLogger()
    // Example config file: logger\example_logger_config.toml
    Logger::InitLogger();
    std::shared_ptr<spdlog::logger> pclLogger = Logger::RegisterLogger("dynamic_components");
    pclLogger->set_level(spdlog::level::debug);
    Logger::AddConsoleLogging(pclLogger);
    Logger::AddRotatingFileLogger(pclLogger);

    // Get command line arguments
    pclLogger->info("Decoder library information:\n{}", caPrettyPrint);

    std::string sEncodeFormat = "ASCII";
    if (argc == 2 && strcmp(argv[1], "-V") == 0) { return 0; }
    if (argc < 3)
    {
        pclLogger->error("ERROR: Need to specify a JSON message definitions DB, an input file and an output format.");
        pclLogger->error("Example: converter.exe <path to Json DB> <path to input file> <output format>");
        return 1;
    }
    if (argc == 4) { sEncodeFormat = argv[3]; }

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

    pclLogger->info("Loading Database...");
    auto tStart = std::chrono::high_resolution_clock::now();
    JsonReader* pclJsonDb = CommonJsonReaderInit();
    CommonJsonReaderLoadFile(pclJsonDb, pathJsonDb.string().c_str());
    pclLogger->info("Done in {}ms",
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tStart).count());

    // Set up the EDIE components
    Framer* pclFramer = NovatelFramerInit();
    NovatelFramerSetLoggerLevel(pclFramer, spdlog::level::debug);
    NovatelFramerFrameJson(pclFramer, false);
    NovatelFramerPayloadOnly(pclFramer, false);
    NovatelFramerReportUnknownBytes(pclFramer, true);

    HeaderDecoder* pclHeaderDecoder = NovatelHeaderDecoderInit(pclJsonDb);
    NovatelHeaderDecoderSetLoggerLevel(pclHeaderDecoder, spdlog::level::debug);

    MessageDecoder* pclMessageDecoder = NovatelMessageDecoderInit(pclJsonDb);
    NovatelMessageDecoderSetLoggerLevel(pclMessageDecoder, spdlog::level::debug);

    Encoder* pclEncoder = NovatelEncoderInit(pclJsonDb);
    NovatelEncoderSetLoggerLevel(pclEncoder, spdlog::level::debug);

    Filter* pclFilter = NovatelFilterInit();
    NovatelFilterSetLoggerLevel(pclFilter, spdlog::level::debug);

    // Set up buffers
    unsigned char acFrameBuffer[MAX_ASCII_MESSAGE_LENGTH];
    unsigned char acEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];
    unsigned char* pucEncodedMessageBuffer = acEncodeBuffer;

    // Initialize structures and error codes
    auto eFramerStatus = STATUS::UNKNOWN;
    auto eDecoderStatus = STATUS::UNKNOWN;
    auto eEncoderStatus = STATUS::UNKNOWN;

    IntermediateHeader stHeader;
    std::vector<FieldContainer> stMessage;

    MetaDataStruct stMetaData;
    MessageDataStruct stMessageData;

    // Initialize FS structures and buffers
    StreamReadStatus stReadStatus;
    ReadDataStructure stReadData;
    unsigned char* acIfsReadBuffer[MAX_ASCII_MESSAGE_LENGTH];
    stReadData.cData = reinterpret_cast<char*>(acIfsReadBuffer);
    stReadData.uiDataSize = sizeof(acIfsReadBuffer);

    // Set up file streams
    InputFileStream clIfs(pathInFilename.string().c_str());
    OutputFileStream clConvertedLogsOfs(pathInFilename.string().append(".").append(sEncodeFormat).c_str());
    OutputFileStream clUnknownBytesOfs(pathInFilename.string().append(".").append(sEncodeFormat).append(".UNKNOWN").c_str());

    tStart = std::chrono::high_resolution_clock::now();

    while (!stReadStatus.bEOS)
    {
        stReadStatus = clIfs.ReadData(stReadData);
        NovatelFramerWrite(pclFramer, reinterpret_cast<unsigned char*>(stReadData.cData), stReadStatus.uiCurrentStreamRead);
        // Clearing INCOMPLETE status when internal buffer needs more bytes.
        eFramerStatus = STATUS::INCOMPLETE_MORE_DATA;

        while (eFramerStatus != STATUS::BUFFER_EMPTY && eFramerStatus != STATUS::INCOMPLETE)
        {
            unsigned char* pucFrameBuffer = acFrameBuffer;
            eFramerStatus = NovatelFramerRead(pclFramer, pucFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, &stMetaData);

            if (eFramerStatus == STATUS::SUCCESS)
            {
                if (stMetaData.bResponse)
                {
                    clUnknownBytesOfs.WriteData(reinterpret_cast<char*>(pucFrameBuffer), stMetaData.uiLength);
                    continue;
                }

                pucFrameBuffer[stMetaData.uiLength] = '\0';
                pclLogger->info("Framed: {}", reinterpret_cast<char*>(pucFrameBuffer));

                // Decode the header.  Get metadata here and populate the Intermediate header.
                eDecoderStatus = NovatelHeaderDecoderDecode(pclHeaderDecoder, pucFrameBuffer, &stHeader, &stMetaData);

                if (eDecoderStatus == STATUS::SUCCESS)
                {
                    // Filter the log, pass over this log if we don't want it.
                    if (!NovatelFilterDoFiltering(pclFilter, &stMetaData)) { continue; }

                    pucFrameBuffer += stMetaData.uiHeaderLength;
                    // Decode the Log, pass the metadata and populate the intermediate log.
                    eDecoderStatus = NovatelMessageDecoderDecode(pclMessageDecoder, pucFrameBuffer, &stMessage, &stMetaData);

                    if (eDecoderStatus == STATUS::SUCCESS)
                    {
                        eEncoderStatus = NovatelEncoderEncode(pclEncoder, pucEncodedMessageBuffer, MAX_ASCII_MESSAGE_LENGTH, &stHeader, &stMessage,
                                                              &stMessageData, &stMetaData, eEncodeFormat);

                        if (eEncoderStatus == STATUS::SUCCESS)
                        {
                            clConvertedLogsOfs.WriteData(reinterpret_cast<char*>(stMessageData.pucMessage), stMessageData.uiMessageLength);
                            stMessageData.pucMessage[stMessageData.uiMessageLength] = '\0';
                            pclLogger->info("Encoded: ({}) {}\n", stMessageData.uiMessageLength, reinterpret_cast<char*>(stMessageData.pucMessage));
                        }
                        else
                        {
                            clUnknownBytesOfs.WriteData(reinterpret_cast<char*>(pucFrameBuffer), stMetaData.uiLength);
                            pclLogger->warn("Encoder returned with status code {}", static_cast<int32_t>(eEncoderStatus));
                        }
                    }
                    else
                    {
                        clUnknownBytesOfs.WriteData(reinterpret_cast<char*>(pucFrameBuffer), stMetaData.uiLength);
                        pclLogger->warn("MessageDecoder returned with status code {}", static_cast<int32_t>(eDecoderStatus));
                    }
                }
                else
                {
                    clUnknownBytesOfs.WriteData(reinterpret_cast<char*>(pucFrameBuffer), stMetaData.uiLength);
                    pclLogger->warn("HeaderDecoder returned with status code {}", static_cast<int32_t>(eDecoderStatus));
                }
            }
            else if (eFramerStatus == STATUS::UNKNOWN) { clUnknownBytesOfs.WriteData(reinterpret_cast<char*>(pucFrameBuffer), stMetaData.uiLength); }
            else { pclLogger->warn("Framer returned with status code {}", static_cast<int32_t>(eFramerStatus)); }
        }
    }

    // Clean up
    NovatelFramerShutdownLogger(pclFramer);
    NovatelHeaderDecoderShutdownLogger(pclHeaderDecoder);
    NovatelMessageDecoderShutdownLogger(pclMessageDecoder);
    NovatelEncoderShutdownLogger(pclEncoder);
    NovatelFilterShutdownLogger(pclFilter);
    Logger::Shutdown();

    NovatelFramerDelete(pclFramer);
    NovatelHeaderDecoderDelete(pclHeaderDecoder);
    NovatelMessageDecoderDelete(pclMessageDecoder);
    NovatelEncoderDelete(pclEncoder);
    NovatelFilterDelete(pclFilter);
    return 0;
}
