#!/usr/bin/env python3
########################################################################
#
# COPYRIGHT NovAtel Inc, 2022. All rights reserved.
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
########################################################################
#                            DESCRIPTION
#
# \file converter_fileparser.py
# \brief Demonstrate how to use the Python API for converting OEM
# messages using the FileParser.
########################################################################
import argparse
import os
import timeit

import novatel_edie as ne
from novatel_edie import Logging, LogLevel


def _configure_logging(logger):
    logger.set_level(LogLevel.DEBUG)
    Logging.add_console_logging(logger)
    Logging.add_rotating_file_logger(logger)


def main():
    logger = Logging().register_logger("converter")
    _configure_logging(logger)

    logger.info(f"Decoder library information:\n{ne.pretty_version}")

    parser = argparse.ArgumentParser(description="Convert OEM log files using FileParser.")
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

    file_parser = ne.FileParser()
    file_parser.filter = ne.Filter()
    file_parser.encode_format = encode_format
    _configure_logging(file_parser.logger)
    _configure_logging(file_parser.filter.logger)

    # Set up file streams
    input_stream = ne.InputFileStream(args.input_file)
    converted_logs_stream = ne.OutputFileStream(f"{args.input_file}.{encode_format}")

    if not file_parser.set_stream(input_stream):
        logger.error("Input stream could not be set.  The stream is either unavailable or exhausted.")
        exit(-1)

    meta = ne.MetaData()
    complete_messages = 0
    counter = 0
    start = timeit.default_timer()
    loop = timeit.default_timer()
    status = ne.STATUS.UNKNOWN
    while status != ne.STATUS.STREAM_EMPTY:
        status, message_data = file_parser.read(meta)
        if status != ne.STATUS.SUCCESS:
            logger.error(f"Failed to read a message: {status}: {status.__doc__}")
            continue
        converted_logs_stream.write(message_data.message)
        logger.info(f"Encoded: ({len(message_data.message)}) {message_data.message}")
        complete_messages += 1

        if timeit.default_timer() - loop > 1:
            counter += 1
            logger.info(f"{file_parser.percent_read}% {complete_messages / counter} logs/s")
            loop = timeit.default_timer()

    elapsed_seconds = timeit.default_timer() - start
    logger.info(f"Converted {complete_messages} logs in {elapsed_seconds:.3f}s from {args.input_file}")
    Logging.shutdown()


if __name__ == "__main__":
    main()
