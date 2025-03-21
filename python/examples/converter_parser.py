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
# \file converter_parser.py
# \brief Demonstrate how to use the Python API for converting OEM
# messages using the Parser.
########################################################################

import argparse
import atexit
import timeit

import novatel_edie as ne
from novatel_edie import Logging, LogLevel
from novatel_edie.messages import BESTPOS


def _configure_logging(logger):
    logger.set_level(LogLevel.DEBUG)
    Logging.add_console_logging(logger)
    Logging.add_rotating_file_logger(logger)


def main():
    logger = Logging().register_logger("converter")
    _configure_logging(logger)
    atexit.register(Logging.shutdown)

    logger.info(f"Decoder library information:\n{ne.pretty_version}")

    parser = argparse.ArgumentParser(description="Convert OEM log files using Parser.")
    parser.add_argument("input_file", help="Input file")
    parser.add_argument("output_format", nargs="?",
                        choices=["ASCII", "ABBREV_ASCII", "BINARY", "FLATTENED_BINARY", "JSON"],
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
    parser.filter = ne.Filter()
    _configure_logging(parser.logger)
    _configure_logging(parser.filter.logger)

    with (open(args.input_file, "rb") as input_stream,
          open(f"{args.input_file}.{encode_format}", "wb") as converted_logs_stream):
        meta = ne.MetaData()
        messages = 0
        start = timeit.default_timer()
        while read_data := input_stream.read(ne.MESSAGE_SIZE_MAX):
            parser.write(read_data)
            for message in parser:
                if isinstance(message, ne.Message):
                    encoded_msg = message.encode(encode_format)
                    ascii_msg = message.to_ascii()
                    binary_msg = message.to_binary()
                    messages += 1
                    if isinstance(message, BESTPOS):
                        lat = message.latitude
                        lon = message.longitude
                elif isinstance(message, ne.UnknownMessage):
                    unknown_id = message.header.message_id
                    payload = message.payload
                elif isinstance(message, ne.UnknownBytes):
                    data = message.data

    elapsed_seconds = timeit.default_timer() - start
    logger.info(f"Converted {messages} logs in {elapsed_seconds:.3f}s from {args.input_file}")
    Logging.shutdown()


if __name__ == "__main__":
    main()
