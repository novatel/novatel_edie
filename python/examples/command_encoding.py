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

import sys
from pathlib import Path

import novatel_edie as ne
from novatel_edie import Logger, LogLevel


def main():
    logger = Logger().register_logger("CommandEncoder")
    logger.set_level(LogLevel.DEBUG)
    Logger.add_console_logging(logger)
    Logger.add_rotating_file_logger(logger)

    if len(sys.argv) < 3:
        logger.error("Format: command_encoding <output format> <abbreviated ascii command>\n")
        logger.error('Example: command_encoding ASCII "RTKTIMEOUT 30"\n')
        exit(1)

    encode_format = ne.string_to_encode_format(sys.argv[1])
    if encode_format == ne.ENCODE_FORMAT.UNSPECIFIED:
        logger.error("Unsupported output format. Choose from:\n\tASCII\n\tBINARY")
        exit(1)

    command = sys.argv[2].encode()
    logger.info(f'Converting "{command}" to {encode_format}')
    commander = ne.Commander()
    status, encoded_command = commander.encode(command, encode_format)
    out_file = Path(f"COMMAND.{encode_format}")
    out_file.write_bytes(encoded_command)


if __name__ == "__main__":
    main()
