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

import logging
import argparse
import atexit
import os
import timeit
import pandas

import novatel_edie as ne
from novatel_edie.messages import BESTPOS
from common import setup_example_logging


def main():
    setup_example_logging(logging.WARNING)
    logger = logging.getLogger(__name__)    \
    # ne.disable_internal_logging()

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
        exit(1)
    file_parser = ne.FileParser(args.input_file)
    filter = ne.Filter()
    file_parser.filter = filter

    messages = 0
    start = timeit.default_timer()
    for message in file_parser:
        if isinstance(message, ne.Message):
            messages += 1
        if messages > 100000:
            break

    elapsed_seconds = timeit.default_timer() - start
    logger.warning(f"Converted {messages} logs in {elapsed_seconds:.3f}s from {args.input_file}")


if __name__ == "__main__":
    main()
