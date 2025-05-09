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
# \brief Demonstrate how to use the Python API for OEM command
# encoding from Abbreviated ASCII to ASCII/BINARY.
########################################################################

import logging

import novatel_edie as ne
from novatel_edie import CPP_PRETTY_VERSION

from common_setup import setup_example_logging


def main():
    # Setup logging
    setup_example_logging(logging.WARNING)
    logger = logging.getLogger(__name__)
    logger.info(f"Decoder library information:\n{CPP_PRETTY_VERSION}")

    # Set value to encode
    encode_format = ne.ENCODE_FORMAT.ASCII
    command = b"CONFIGCODE ERASE_TABLE \"WJ4HDW\" \"GM5Z99\" \"T2M7DP\" \"KG2T8T\" \"KF7GKR\" \"TABLECLEAR\""

    # Encode the command
    logger.info(f'Converting "{command}" to {encode_format}')
    commander = ne.Commander()
    encoded_command = commander.encode(command, encode_format)
    print(encoded_command)

if __name__ == "__main__":
    main()
