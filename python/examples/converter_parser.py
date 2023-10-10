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
# \file converter_parser.cpp
# \brief Demonstrate how to use the C++ source for converting OEM
# messages using the Parser.
########################################################################
import argparse
import timeit

import novatel_edie as ne
from novatel_edie import Logger, LogLevel


def main():
    logger = Logger().register_logger("converter")
    logger.set_level(LogLevel.DEBUG)
    Logger.add_console_logging(logger)
    Logger.add_rotating_file_logger(logger)

    logger.info(f"Decoder library information:\n{ne.pretty_version}")

    parser = argparse.ArgumentParser(description="Convert OEM log files using Parser.")
    parser.add_argument("input_file", help="Input file")
    parser.add_argument("output_format", nargs="?", choices=["ASCII", "BINARY", "FLATTENED_BINARY"],
                        help="Output format", default="ASCII")
    parser.add_argument("append_msg_types", nargs="?", help="Additional message definitions to append to the database")
    parser.add_argument("-V", "--version", action="store_true")
    args = parser.parse_args()
    encode_format = ne.string_to_encode_format(args.output_format)

    if args.version:
        exit(0)

    # Load the database
    logger.info("Loading Database... ")
    t0 = timeit.default_timer()
    json_db = ne.get_default_database()
    t1 = timeit.default_timer()
    logger.info(f"Done in {(t1 - t0) * 1e3:.0f} ms")

    if args.append_msg_types:
        logger.info("Appending Message...")
        start = timeit.default_timer()
        json_db.append_messages(args.append_msg_types)
        logger.info(f"Done in {timeit.default_timer() - start:.0f}ms")

    parser = ne.Parser(json_db)
    parser.SetEncodeFormat(encode_format)
    parser.logger.set_level(LogLevel.DEBUG)
    Logger.add_console_logging(parser.logger)
    Logger.add_rotating_file_logger(parser.logger)

    filter = ne.Filter()
    filter.logger.set_level(LogLevel.DEBUG)
    Logger.add_console_logging(filter.logger)
    Logger.add_rotating_file_logger(filter.logger)

    parser.SetFilter(filter)

    # Setup filestreams
    ifs = ne.InputFileStream(args.input_file)
    convertedlogsofs = ne.OutputFileStream(f"{args.input_file}.{encode_format}")

    completemessages = 0
    counter = 0
    start = timeit.default_timer()
    loop = timeit.default_timer()
    readstatus = ne.StreamReadStatus()
    while not readstatus.eos:
        readdata.data = ifsreadbuffer
        readstatus = ifs.ReadData(readdata)
        parser.Write(readdata)

        while True:
            status = parser.Read(message_data, metadata)

            if status == ne.STATUS.SUCCESS:
                convertedlogsofs.WriteData(message_data)
                logger.info(f"Encoded: ({message_data.messagelength}) {message_data.message}")
                completemessages += 1

            if timeit.default_timer() - loop > 1:
                counter += 1
                logger.info(f"{completemessages / counter} logs/s")
                loop = timeit.default_timer()

            if status == ne.STATUS.BUFFER_EMPTY:
                break

    logger.info(f"Converted {completemessages} logs in {timeit.default_timer() - start:.3f}s from {args.input_file}")

    Logger.shutdown()


if __name__ == "__main__":
    main()
