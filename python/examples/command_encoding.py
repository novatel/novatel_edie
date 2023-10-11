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
# \file command_encoding.py
# \brief Demonstrate how to use the Python source for OEM command
# encoding from Abbreviated ASCII to ASCII/BINARY.
########################################################################

import argparse
from pathlib import Path

import novatel_edie as ne
from novatel_edie import Logging, LogLevel


def main():
    logger = Logging().register_logger("CommandEncoder")
    logger.set_level(LogLevel.DEBUG)
    Logging.add_console_logging(logger)
    Logging.add_rotating_file_logger(logger)

    parser = argparse.ArgumentParser(description="Encode a command from Abbreviated ASCII to ASCII/BINARY.")
    parser.add_argument("output_format", choices=["ASCII", "BINARY"], help="Output format")
    parser.add_argument("command", help="Abbreviated ASCII command")
    parser.add_argument("-V", "--version", action="store_true")
    args = parser.parse_args()
    encode_format = ne.string_to_encode_format(args.output_format)

    if args.version:
        logger.info(ne.pretty_version)
        exit(0)

    logger.info(f'Converting "{args.command}" to {encode_format}')
    commander = ne.Commander()
    status, encoded_command = commander.encode(args.command.encode(), encode_format)
    status.raise_on_error()
    out_file = Path(f"COMMAND.{encode_format}")
    out_file.write_bytes(encoded_command)


if __name__ == "__main__":
    main()
