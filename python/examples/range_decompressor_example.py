#!/usr/bin/env python3
############################################################################
#
# Copyright (c) 2021 NovAtel Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
############################################################################
#
#  DESCRIPTION:
#    An example program using the low-level EDIE components to filter for
#    and decompress rangecmp logs.
#
############################################################################
import argparse
import os
import timeit

import novatel_edie as ne
from novatel_edie import Logger, LogLevel


def main():
    logger = Logger().register_logger("range_decompressor")
    logger.set_level(LogLevel.DEBUG)
    Logger.add_console_logging(logger)
    Logger.add_rotating_file_logger(logger)

    logger.info(f"Decoder library information:\n{ne.pretty_version}")

    parser = argparse.ArgumentParser(description="Convert OEM log files using FileParser.")
    parser.add_argument("input_file", help="Input file")
    parser.add_argument("output_format", nargs="?", choices=["ASCII", "BINARY", "FLATTENED_BINARY"],
                        help="Output format", default="ASCII")
    parser.add_argument("-V", "--version", action="store_true")
    args = parser.parse_args()
    encode_format = ne.string_to_encode_format(args.output_format)

    if args.version:
        exit(0)

    if not os.path.exists(args.input_file):
        logger.error(f'File "{args.input_file}" does not exist')
        exit(1)

    framer = ne.Framer()
    header_decoder = ne.HeaderDecoder()
    message_decoder = ne.MessageDecoder()
    rangedecompressor = ne.RangeDecompressor()
    encoder = ne.Encoder()

    framer.logger.set_level(LogLevel.DEBUG)
    header_decoder.logger.set_level(LogLevel.DEBUG)
    message_decoder.logger.set_level(LogLevel.DEBUG)
    encoder.logger.set_level(LogLevel.DEBUG)
    Logger.add_console_logging(framer.logger)
    Logger.add_console_logging(header_decoder.logger)
    Logger.add_console_logging(message_decoder.logger)
    Logger.add_rotating_file_logger(framer.logger)
    Logger.add_rotating_file_logger(header_decoder.logger)
    Logger.add_rotating_file_logger(message_decoder.logger)

    framer.SetFrameJson(False)
    framer.SetPayloadOnly(False)
    framer.SetReportUnknownBytes(True)

    ifs = ne.InputFileStream(args.input_file)
    ofs = ne.OutputFileStream(f"{args.input_file}.DECOMPRESSED.{encode_format}")

    header = ne.IntermediateHeader()
    message = ne.IntermediateMessage()

    metadata = ne.MetaDataStruct()
    message_data = ne.MessageDataStruct()

    start = timeit.default_timer()
    completedmessages = 0
    while True:
        # Get frame, null-terminate.
        status = framer.GetFrame(readbuffer, metadata)
        if status == ne.STATUS.SUCCESS:
            # Decode the header.  Get meta data here and populate the Intermediate header.
            status = header_decoder.Decode(readbuffer, header, metadata)
            if status == ne.STATUS.SUCCESS:
                status = rangedecompressor.Decompress(readbuffer, metadata, encode_format)
                if status == ne.STATUS.SUCCESS:
                    completedmessages += 1
                    byteswritten = ofs.WriteData(readbuffer, metadata.length)
                    if metadata.length == byteswritten:
                        readbuffer[metadata.length] = "\0"
                        logger.info(f"Decompressed: ({metadata.length}) {readbuffer}")
                    else:
                        logger.error(f"Could only write {byteswritten}/{message_data.messagelength} bytes.")
                elif status == ne.STATUS.UNSUPPORTED:
                    if status == ne.STATUS.SUCCESS:
                        header.messageid = metadata.messageid
                        status = message_decoder.Decode((readbuffer + metadata.headerlength), message, metadata)
                        if status == ne.STATUS.SUCCESS:
                            # Encode our message now that we have everything we need.
                            encodedmessagebuffer, status = encoder.Encode(
                                header,
                                message,
                                message_data,
                                metadata,
                                encode_format,
                            )
                            if status == ne.STATUS.SUCCESS:
                                message_data.message[message_data.messagelength] = "\0"
                                logger.info(f"Encoded: ({message_data.messagelength}) {message_data.message}")
        elif (status == ne.STATUS.BUFFER_EMPTY) or (status == ne.STATUS.INCOMPLETE):
            # Read from file, write to framer.
            readstatus = ifs.ReadData(readdata)
            if readstatus.currentstreamread == 0:
                logger.info("Stream finished")
                break

            framer.Write(readdata)

    elapsed_seconds = timeit.default_timer() - start
    logger.info(
        f"Decoded {completedmessages} messages in {elapsed_seconds}s. ({completedmessages / elapsed_seconds:.1f} msg/s)"
    )

    Logger.shutdown()


if __name__ == "__main__":
    main()
