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

#include <stdio.h>
#include <stdlib.h>

#include "src/decoders/dynamic_library/api/common_jsonreader.hpp"
#include "src/decoders/dynamic_library/api/novatel_encoder.hpp"
#include "src/decoders/dynamic_library/api/novatel_filter.hpp"
#include "src/decoders/dynamic_library/api/novatel_framer.hpp"
#include "src/decoders/dynamic_library/api/novatel_header_decoder.hpp"
#include "src/decoders/dynamic_library/api/novatel_message_decoder.hpp"
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

    // Setup the EDIE components
    Framer* pclFramer = novatel_framer_init();
    novatel_framer_set_logger_level(pclFramer, static_cast<uint32_t>(spdlog::level::debug));
    novatel_framer_frame_json(pclFramer, false);
    novatel_framer_payload_only(pclFramer, false);
    novatel_framer_report_unknown_bytes(pclFramer, true);

    HeaderDecoder* pclHeaderDecoder = novatel_header_decoder_init(pclJsonDb);
    novatel_header_decoder_set_logger_level(pclHeaderDecoder, static_cast<uint32_t>(spdlog::level::debug));

    MessageDecoder* pclMessageDecoder = novatel_message_decoder_init(pclJsonDb);
    novatel_message_decoder_set_logger_level(pclMessageDecoder, static_cast<uint32_t>(spdlog::level::debug));

    Encoder* pclEncoder = novatel_encoder_init(pclJsonDb);
    novatel_encoder_set_logger_level(pclEncoder, static_cast<uint32_t>(spdlog::level::debug));

    Filter* pclFilter = novatel_filter_init();
    novatel_filter_set_logger_level(pclFilter, static_cast<uint32_t>(spdlog::level::debug));

    // Setup buffers
    unsigned char acFrameBuffer[MAX_ASCII_MESSAGE_LENGTH];
    unsigned char* pucFrameBuffer = acFrameBuffer;
    unsigned char acEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];
    unsigned char* pucEncodedMessageBuffer = acEncodeBuffer;

    // Initialize structures and error codes
    STATUS eFramerStatus = STATUS::UNKNOWN;
    STATUS eDecoderStatus = STATUS::UNKNOWN;
    STATUS eEncoderStatus = STATUS::UNKNOWN;

    IntermediateHeader stHeader;
    IntermediateMessage stMessage;

    MetaDataStruct stMetaData;
    MessageDataStruct stMessageData;

    // Initialize FS structures and buffers
    StreamReadStatus stReadStatus;
    ReadDataStructure stReadData;
    unsigned char* acIFSReadBuffer[MAX_ASCII_MESSAGE_LENGTH];
    stReadData.cData = reinterpret_cast<char*>(acIFSReadBuffer);
    stReadData.uiDataSize = sizeof(acIFSReadBuffer);

    // Setup filestreams
    InputFileStream clIFS(sInFilename.c_str());
    OutputFileStream clConvertedLogsOFS(sInFilename.append(".").append(sEncodeFormat).c_str());
    OutputFileStream clUnknownBytesOFS(sInFilename.append(".UNKNOWN").c_str());

    tStart = chrono::high_resolution_clock::now();
    tLoop = chrono::high_resolution_clock::now();

    while (!stReadStatus.bEOS)
    {
        stReadStatus = clIFS.ReadData(stReadData);
        novatel_framer_write(pclFramer, reinterpret_cast<unsigned char*>(stReadData.cData), stReadStatus.uiCurrentStreamRead);
        // Clearing INCOMPLETE status when internal buffer needs more bytes.
        eFramerStatus = STATUS::INCOMPLETE_MORE_DATA;

        while (eFramerStatus != STATUS::BUFFER_EMPTY && eFramerStatus != STATUS::INCOMPLETE)
        {
            pucFrameBuffer = acFrameBuffer;
            eFramerStatus = novatel_framer_read(pclFramer, pucFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, &stMetaData);

            if (eFramerStatus == STATUS::SUCCESS)
            {
                if (stMetaData.bResponse)
                {
                    clUnknownBytesOFS.WriteData(reinterpret_cast<char*>(pucFrameBuffer), stMetaData.uiLength);
                    continue;
                }

                pucFrameBuffer[stMetaData.uiLength] = '\0';
                pclLogger->info("Framed: {}", reinterpret_cast<char*>(pucFrameBuffer));

                // Decode the header.  Get meta data here and populate the Intermediate header.
                eDecoderStatus = novatel_header_decoder_decode(pclHeaderDecoder, pucFrameBuffer, &stHeader, &stMetaData);

                if (eDecoderStatus == STATUS::SUCCESS)
                {
                    // Filter the log, pass over this log if we don't want it.
                    if (!novatel_filter_do_filtering(pclFilter, &stMetaData)) { continue; }

                    pucFrameBuffer += stMetaData.uiHeaderLength;
                    // Decode the Log, pass the meta data and populate the intermediate log.
                    eDecoderStatus = novatel_message_decoder_decode(pclMessageDecoder, pucFrameBuffer, &stMessage, &stMetaData);

                    if (eDecoderStatus == STATUS::SUCCESS)
                    {
                        eEncoderStatus = novatel_encoder_encode(pclEncoder, pucEncodedMessageBuffer, MAX_ASCII_MESSAGE_LENGTH, &stHeader, &stMessage,
                                                                &stMessageData, &stMetaData, eEncodeFormat);

                        if (eEncoderStatus == STATUS::SUCCESS)
                        {
                            clConvertedLogsOFS.WriteData(reinterpret_cast<char*>(stMessageData.pucMessage), stMessageData.uiMessageLength);
                            stMessageData.pucMessage[stMessageData.uiMessageLength] = '\0';
                            pclLogger->info("Encoded: ({}) {}\n", stMessageData.uiMessageLength, reinterpret_cast<char*>(stMessageData.pucMessage));
                        }
                        else
                        {
                            clUnknownBytesOFS.WriteData(reinterpret_cast<char*>(pucFrameBuffer), stMetaData.uiLength);
                            pclLogger->warn("Encoder returned with status code {}", static_cast<int32_t>(eEncoderStatus));
                        }
                    }
                    else
                    {
                        clUnknownBytesOFS.WriteData(reinterpret_cast<char*>(pucFrameBuffer), stMetaData.uiLength);
                        pclLogger->warn("MessageDecoder returned with status code {}", static_cast<int32_t>(eDecoderStatus));
                    }
                }
                else
                {
                    clUnknownBytesOFS.WriteData(reinterpret_cast<char*>(pucFrameBuffer), stMetaData.uiLength);
                    pclLogger->warn("HeaderDecoder returned with status code {}", static_cast<int32_t>(eDecoderStatus));
                }
            }
            else if (eFramerStatus == STATUS::UNKNOWN) { clUnknownBytesOFS.WriteData(reinterpret_cast<char*>(pucFrameBuffer), stMetaData.uiLength); }
            else { pclLogger->warn("Framer returned with status code {}", static_cast<int32_t>(eFramerStatus)); }
        }
    }

    // Clean up
    pucFrameBuffer = acFrameBuffer;

    novatel_framer_shutdown_logger(pclFramer);
    novatel_header_decoder_shutdown_logger(pclHeaderDecoder);
    novatel_message_decoder_shutdown_logger(pclMessageDecoder);
    novatel_encoder_shutdown_logger(pclEncoder);
    novatel_filter_shutdown_logger(pclFilter);
    Logger::Shutdown();

    novatel_framer_delete(pclFramer);
    novatel_header_decoder_delete(pclHeaderDecoder);
    novatel_message_decoder_delete(pclMessageDecoder);
    novatel_encoder_delete(pclEncoder);
    novatel_filter_delete(pclFilter);
    return 0;
}
