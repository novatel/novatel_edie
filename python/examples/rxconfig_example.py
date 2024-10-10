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
# \file rxconfig_example.py
# \brief Demonstrate how to use the Python API for converting RXCONFIG
# messages.
########################################################################
import argparse
import atexit
import os

import novatel_edie as ne
from novatel_edie import Logging, LogLevel, ENCODE_FORMAT


def _configure_logging(logger):
    logger.set_level(LogLevel.DEBUG)
    Logging.add_console_logging(logger)
    Logging.add_rotating_file_logger(logger)


def main():
    # This example uses the default logger config, but you can also pass a config file to the Logging() ctor
    # An example config file: doc\example_logger_config.toml
    logger = Logging().register_logger("rxconfig_converter")
    _configure_logging(logger)
    atexit.register(Logging.shutdown)

    logger.info(f"Decoder library information:\n{ne.pretty_version}")

    parser = argparse.ArgumentParser(description="Convert OEM RXCONFIG messages.")
    parser.add_argument("input_file", help="Input file")
    parser.add_argument("output_format", nargs="?",
                        choices=["ASCII", "ABBREV_ASCII", "BINARY", "FLATTENED_BINARY", "JSON"],
                        help="Output format", default="ASCII")
    parser.add_argument("-V", "--version", action="store_true")
    args = parser.parse_args()
    encode_format = ne.string_to_encode_format(args.output_format)

    if args.version:
        exit(0)

    if not os.path.exists(args.input_file):
        logger.error(f'File "{args.input_file}" does not exist')
        exit(1)

    # Setup file streams
    input_stream = open(args.input_file, "rb")
    converted_rxconfig_ofs = ne.OutputFileStream(f"{args.input_file}.{encode_format}")
    stripped_rxconfig_ofs = ne.OutputFileStream(f"{args.input_file}.STRIPPED.{encode_format}")

    rx_config_handler = ne.RxConfigHandler()

    while read_data := input_stream.read(ne.MESSAGE_SIZE_MAX):
        rx_config_handler.write(read_data)

        status = None
        while status != ne.STATUS.BUFFER_EMPTY:
            status, message, metadata, embedded_message, embedded_metadata = rx_config_handler.convert(encode_format)
            if status not in [ne.STATUS.SUCCESS, ne.STATUS.BUFFER_EMPTY]:
                logger.error(f"Error converting RXCONFIG message: {status}")
                continue
            if len(message.message) == 0:
                break
            logger.info(f"Encoded: ({len(message.message)}) {message.message}")
            converted_rxconfig_ofs.write(message.message)

            # Make the embedded message valid by flipping the CRC.
            if encode_format == ne.ENCODE_FORMAT.ASCII:
                # Flip the CRC at the end of the embedded message and add a CRLF so it becomes a valid command.
                msg = embedded_message.message
                msg = msg[:-ne.OEM4_ASCII_CRC_LENGTH] + msg[-ne.OEM4_ASCII_CRC_LENGTH:][::-1]
                stripped_rxconfig_ofs.write(msg)
                stripped_rxconfig_ofs.write(b"\r\n")
            elif encode_format == ne.ENCODE_FORMAT.BINARY:
                # Flip the CRC at the end of the embedded message so it becomes a valid command.
                for i in range(ne.OEM4_BINARY_CRC_LENGTH):
                    embedded_message.message[-ne.OEM4_BINARY_CRC_LENGTH + i] ^= 0xFF
                stripped_rxconfig_ofs.write(embedded_message)
            elif encode_format == ne.ENCODE_FORMAT.JSON:
                # Write in a comma and CRLF to make the files parse-able by JSON readers.
                converted_rxconfig_ofs.write(b",\r\n")
                stripped_rxconfig_ofs.write(b",\r\n")


if __name__ == "__main__":
    main()
