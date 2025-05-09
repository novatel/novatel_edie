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
import timeit

import novatel_edie as ne
import novatel_edie.messages as ne_msgs

from common_setup import setup_example_logging, handle_args

def main():
    """Example FileParser usage."""
    # Setup logging
    setup_example_logging(logging.WARNING)
    logger = logging.getLogger(__name__)
    logger.info(f"Decoder library information:\n{ne.CPP_PRETTY_VERSION}")

    # Handle CLI arguments
    input_file, encode_format = handle_args(logger)

    # Create a FileParser
    file_parser = ne.FileParser(input_file)

    # Setup a custom filter here
    my_filter = ne.Filter()
    file_parser.filter = my_filter

    # Iterate through the messages
    messages = 0
    msgs = []
    start = timeit.default_timer()
    observation_list = []
    for message in file_parser:
        # Handle messages that can be fully decoded
        if isinstance(message, ne.Message):
            # Encode the message into different formats
            encoded_msg = message.encode(encode_format)
            ascii_msg = message.to_ascii()
            binary_msg = message.to_binary()
            dict_msg = message.to_dict()
            messages += 1
            # Handle BESTPOS messages
            if isinstance(message, ne_msgs.BESTPOS):
                # Access specific fields
                lat = message.latitude
                lon = message.longitude
            elif isinstance(message, ne_msgs.RANGE):
                pass
                msgs.append(message.obs)
                if len(msgs) >= 10000:
                    break
        # Handle messages that did not match any known definitions
        elif isinstance(message, ne.UnknownMessage):
            unknown_id = message.header.message_id
            payload = message.payload
        # Handle bytes that could not be parsed into a message
        elif isinstance(message, ne.UnknownBytes):
            data = message.data
    pass

    elapsed_seconds = timeit.default_timer() - start
    logger.info(f"Converted {messages} logs in {elapsed_seconds:.3f}s from {input_file}")



if __name__ == "__main__":
    main()
