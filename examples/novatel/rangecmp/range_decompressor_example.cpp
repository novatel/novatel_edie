////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2021 NovAtel Inc.
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
////////////////////////////////////////////////////////////////////////////////
//
//  DESCRIPTION:
//    An example program using the low-level EDIE components to filter for
//    and decompress rangecmp logs.
//
////////////////////////////////////////////////////////////////////////////////

#include <cstdio>
#include <cstdlib>

#include <chrono>

#include "src/decoders/common/api/message_decoder.hpp"
#include "src/decoders/novatel/api/encoder.hpp"
#include "src/decoders/novatel/api/filter.hpp"
#include "src/decoders/novatel/api/framer.hpp"
#include "src/decoders/novatel/api/header_decoder.hpp"
#include "src/decoders/novatel/api/rangecmp/range_decompressor.hpp"
#include "src/hw_interface/stream_interface/api/inputfilestream.hpp"
#include "src/hw_interface/stream_interface/api/outputfilestream.hpp"
#include "src/version.h"

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
    std::shared_ptr<spdlog::logger> pclLogger = Logger::RegisterLogger("range_decompressor");
    pclLogger->set_level(spdlog::level::debug);
    Logger::AddConsoleLogging(pclLogger);
    Logger::AddRotatingFileLogger(pclLogger);

    if (argc == 2 && strcmp(argv[1], "-V") == 0)
    {
        pclLogger->info("Decoder library information:\n{}", caPrettyPrint);
        return 0;
    }

    std::string sEncodeFormat = "ASCII";

    if (argc < 3)
    {
        pclLogger->info("ERROR: Need to specify a JSON message definitions DB, an input file and an output "
                        "format.\n");
        pclLogger->info("Example: converter.exe <path to Json DB> <path to input file> <output format>\n");
        return -1;
    }
    if (argc == 4) { sEncodeFormat = argv[3]; }

    if (!file_exists(argv[1]))
    {
        pclLogger->error("File \"{}\" does not exist", argv[1]);
        return 1;
    }
    if (!file_exists(argv[2]))
    {
        pclLogger->error("File \"{}\" does not exist", argv[2]);
        return 1;
    }

    std::string sJsonDB = argv[1];
    std::string sInFilename = argv[2];

    ENCODEFORMAT eEncodeFormat = StringToEncodeFormat(sEncodeFormat);
    if (eEncodeFormat == ENCODEFORMAT::UNSPECIFIED)
    {
        pclLogger->error("Unspecified output format.\n\tASCII\n\tBINARY\n\tFLATTENED_BINARY");
        return -1;
    }

    pclLogger->info("Decoder library information:\n{}", caPrettyPrint);

    JsonReader clJsonDb;
    pclLogger->info("Loading Database... ");
    auto tStart = std::chrono::high_resolution_clock::now();
    clJsonDb.LoadFile(sJsonDB);
    pclLogger->info("DONE ({}ms)", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tStart).count());

    Framer clFramer;
    HeaderDecoder clHeaderDecoder(&clJsonDb);
    MessageDecoder clMessageDecoder(&clJsonDb);
    RangeDecompressor clRangeDecompressor(&clJsonDb);
    Encoder clEncoder(&clJsonDb);

    clFramer.SetLoggerLevel(spdlog::level::debug);
    clHeaderDecoder.SetLoggerLevel(spdlog::level::debug);
    clMessageDecoder.SetLoggerLevel(spdlog::level::debug);
    clEncoder.SetLoggerLevel(spdlog::level::debug);
    Logger::AddConsoleLogging(clFramer.GetLogger());
    Logger::AddConsoleLogging(clHeaderDecoder.GetLogger());
    Logger::AddConsoleLogging(clMessageDecoder.GetLogger());
    Logger::AddRotatingFileLogger(clFramer.GetLogger());
    Logger::AddRotatingFileLogger(clHeaderDecoder.GetLogger());
    Logger::AddRotatingFileLogger(clMessageDecoder.GetLogger());

    clFramer.SetFrameJson(false);
    clFramer.SetPayloadOnly(false);
    clFramer.SetReportUnknownBytes(true);

    unsigned char acFileStreamBuffer[MAX_ASCII_MESSAGE_LENGTH];
    ReadDataStructure stReadData;
    stReadData.cData = reinterpret_cast<char*>(acFileStreamBuffer);
    stReadData.uiDataSize = sizeof(acFileStreamBuffer);

    unsigned char acFrameBuffer[MAX_ASCII_MESSAGE_LENGTH];
    [[maybe_unused]] unsigned char* pucReadBuffer = acFrameBuffer;
    unsigned char acEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];
    unsigned char* pucEncodedMessageBuffer = acEncodeBuffer;

    InputFileStream clIFS(sInFilename.c_str());
    OutputFileStream clOFS(sInFilename.append(".DECOMPRESSED.").append(sEncodeFormat).c_str());
    StreamReadStatus stReadStatus;

    STATUS eStatus = STATUS::UNKNOWN;

    IntermediateHeader stHeader;
    IntermediateMessage stMessage;

    MetaDataStruct stMetaData;
    MessageDataStruct stMessageData;

    auto start = std::chrono::system_clock::now();
    uint32_t uiCompletedMessages = 0;
    do {
        pucReadBuffer = acFrameBuffer;

        // Get frame, null-terminate.
        eStatus = clFramer.GetFrame(pucReadBuffer, MAX_ASCII_MESSAGE_LENGTH, stMetaData);
        if (eStatus == STATUS::SUCCESS)
        {
            // Decode the header.  Get meta data here and populate the Intermediate header.
            eStatus = clHeaderDecoder.Decode(pucReadBuffer, stHeader, stMetaData);
            if (eStatus == STATUS::SUCCESS)
            {
                eStatus = clRangeDecompressor.Decompress(pucReadBuffer, MAX_ASCII_MESSAGE_LENGTH, stMetaData, eEncodeFormat);
                if (eStatus == STATUS::SUCCESS)
                {
                    uiCompletedMessages++;
                    uint32_t uiBytesWritten = clOFS.WriteData(reinterpret_cast<char*>(pucReadBuffer), stMetaData.uiLength);
                    if (stMetaData.uiLength == uiBytesWritten)
                    {
                        pucReadBuffer[stMetaData.uiLength] = '\0';
                        pclLogger->info("Decompressed: ({}) {}", stMetaData.uiLength, reinterpret_cast<char*>(pucReadBuffer));
                    }
                    else { pclLogger->error("Could only write {}/{} bytes.", uiBytesWritten, stMessageData.uiMessageLength); }
                }
                else if (eStatus == STATUS::UNSUPPORTED)
                {
                    if (eStatus == STATUS::SUCCESS)
                    {
                        stHeader.usMessageID = stMetaData.usMessageID;
                        eStatus = clMessageDecoder.Decode((pucReadBuffer + stMetaData.uiHeaderLength), stMessage, stMetaData);
                        if (eStatus == STATUS::SUCCESS)
                        {
                            // Encode our message now that we have everything we need.
                            eStatus = clEncoder.Encode(&pucEncodedMessageBuffer, MAX_ASCII_MESSAGE_LENGTH, stHeader, stMessage, stMessageData,
                                                       stMetaData, eEncodeFormat);
                            if (eStatus == STATUS::SUCCESS)
                            {
                                stMessageData.pucMessage[stMessageData.uiMessageLength] = '\0';
                                pclLogger->info("Encoded: ({}) {}", stMessageData.uiMessageLength, reinterpret_cast<char*>(stMessageData.pucMessage));
                            }
                        }
                    }
                }
            }
        }
        else if ((eStatus == STATUS::BUFFER_EMPTY) || (eStatus == STATUS::INCOMPLETE))
        {
            // Read from file, write to framer.
            stReadStatus = clIFS.ReadData(stReadData);
            if (stReadStatus.uiCurrentStreamRead == 0)
            {
                pclLogger->info("Stream finished");
                break;
            }

            clFramer.Write(reinterpret_cast<unsigned char*>(stReadData.cData), stReadStatus.uiCurrentStreamRead);
        }
    } while (true);

    std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start;
    pclLogger->info("Decoded {} messages in {}s. ({}msg/s)", uiCompletedMessages, elapsed_seconds.count(),
                    uiCompletedMessages / elapsed_seconds.count());

    Logger::Shutdown();
    return 0;
}
