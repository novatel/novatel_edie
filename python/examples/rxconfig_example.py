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
#                            DESCRIPTION
#
# \file rxconfig_example.cpp
# \brief Demonstrate how to use the C++ source for converting RXCONFIG
# messages.
########################################################################
import argparse
import os

import novatel_edie as ne
from novatel_edie import Logger, LogLevel, ENCODEFORMAT


def main():
    # This example uses the default logger config, but you can also pass a config file to the Logger() ctor
    # An example config file: doc\example_logger_config.toml
    logger = Logger().register_logger("rxconfig_converter")
    logger.set_level(LogLevel.DEBUG)
    Logger.add_console_logging(logger)
    Logger.add_rotating_file_logger(logger)

    logger.info(f"Decoder library information:\n{ne.pretty_version}")

    parser = argparse.ArgumentParser(description="Convert OEM log files using FileParser.")
    parser.add_argument("input_file", help="Input file")
    parser.add_argument("output_format", nargs="?", help="Output format",
                        default=ENCODEFORMAT.ASCII, type=ne.string_to_encode_format,
                        choices=[ENCODEFORMAT.ASCII, ENCODEFORMAT.BINARY, ENCODEFORMAT.FLATTENED_BINARY])
    parser.add_argument("-V", "--version", action="store_true")
    args = parser.parse_args()

    if args.version:
        exit(0)

    args.output_format = ne.string_to_encode_format(args.output_format)
    if args.output_format == ne.ENCODEFORMAT.UNSPECIFIED:
        logger.error("Unspecified output format.\n\tASCII\n\tBINARY\n\tFLATTENED_BINARY")
        exit(1)

    if not os.path.exists(args.input_file):
        logger.error(f'File "{args.input_file}" does not exist')
        exit(1)

    # Setup file streams
    ifs = ne.InputFileStream(args.input_file)
    convertedrxconfigofs = ne.OutputFileStream(f"{args.input_file}.{args.output_format}")
    strippedrxconfigofs = ne.OutputFileStream(f"{args.input_file}.STRIPPED.{args.output_format}")

    metadata = ne.MetaDataStruct()
    embeddedmetadata = ne.MetaDataStruct()
    message_data = ne.MessageDataStruct()
    embeddedmesage_data = ne.MessageDataStruct()

    rxconfighandler = ne.RxConfigHandler()

    while True:
        readdata.data = ifsreadbuffer
        readstatus = ifs.ReadData(readdata)
        rxconfighandler.Write(readdata)

        while True:
            status = rxconfighandler.Convert(
                message_data, metadata, embeddedmesage_data, embeddedmetadata, args.output_format
            )
            if status == ne.STATUS.SUCCESS:
                message_data.message[message_data.messagelength] = "\0"
                logger.info(f"Encoded: ({message_data.messagelength}) {message_data.message}")
                convertedrxconfigofs.WriteData(message_data)

                # Make the embedded message valid by flipping the CRC.
                if args.output_format == ne.ENCODE_FORMAT.ASCII:
                    # Flip the CRC at the end of the embedded message and add a CRLF so it becomes a valid command.
                    crcbegin = embeddedmesage_data[-ne.OEM4_ASCII_CRC_LENGTH:]
                    flippedcrc = strtoul(crcbegin, NULL, 16) ^ 0xFFFFFFFF
                    snprintf(crcbegin, ne.OEM4_ASCII_CRC_LENGTH + 1, "%08x", flippedcrc)
                    strippedrxconfigofs.WriteData(embeddedmesage_data)
                    strippedrxconfigofs.WriteData(b"\r\n")
                elif args.output_format == ne.ENCODE_FORMAT.BINARY:
                    # Flip the CRC at the end of the embedded message so it becomes a valid command.
                    crcbegin = embeddedmesage_data[-ne.OEM4_BINARY_CRC_LENGTH:]
                    crcbegin ^= 0xFFFFFFFF
                    strippedrxconfigofs.WriteData(embeddedmesage_data)
                elif args.output_format == ne.ENCODE_FORMAT.JSON:
                    # Write in a comma and CRLF to make the files parse-able by JSON readers.
                    convertedrxconfigofs.WriteData(b",\r\n")
                    strippedrxconfigofs.WriteData(b",\r\n")

            if status == ne.STATUS.BUFFER_EMPTY:
                break

        if readstatus.eos:
            break

    Logger.shutdown()


if __name__ == "__main__":
    main()
