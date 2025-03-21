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
# \file converter_components.py
# \brief Demonstrate how to use the Python API for converting OEM
# messages using the low-level components.
########################################################################

import logging
from binascii import hexlify

from novatel_edie import HEADER_FORMAT, ENCODE_FORMAT, pretty_version, Framer, Decoder, Filter, Message, MESSAGE_SIZE_MAX
from novatel_edie.messages import RANGE

from common_setup import setup_example_logging, handle_args


def format_frame(frame, frame_format):
    """Format the frame into a human-readable string."""
    if frame_format in [HEADER_FORMAT.BINARY, HEADER_FORMAT.SHORT_BINARY,
                        HEADER_FORMAT.PROPRIETARY_BINARY,
                        ENCODE_FORMAT.BINARY, ENCODE_FORMAT.FLATTENED_BINARY]:
        return hexlify(frame, sep=" ").decode("ascii").upper()
    return frame


def main():
    """Example framer usage."""
    # Setup logging
    setup_example_logging(logging.WARNING)
    logger = logging.getLogger(__name__)
    logger.info(f"Decoder library information:\n{pretty_version}")

    # Handle CLI arguments
    input_file, encode_format = handle_args(logger)

    # Set up the EDIE components
    framer = Framer()
    framer.set_report_unknown_bytes(True)
    framer.set_payload_only(False)
    framer.set_frame_json(False)
    decoder = Decoder()
    my_filter = Filter()

    with open(input_file, "rb") as input_stream:
        while read_data := input_stream.read(MESSAGE_SIZE_MAX):
            framer.write(read_data)
            for frame, meta in framer:
                if meta.format == HEADER_FORMAT.UNKNOWN:
                    continue
                logger.info(f"Framed ({len(frame)}): {format_frame(frame, meta.format)}")

                # Decode the log header.
                header = decoder.decode_header(frame, meta)

                # Filter the log, pass over it if we don't want it.
                if not my_filter.do_filtering(meta):
                    continue

                # Decode the log body.
                body = frame[meta.header_length:]
                message = decoder.decode_message(body, header, meta)

                # Get info from the log.
                if isinstance(message, RANGE):
                    obs = message.obs
                    for ob in obs:
                        value = ob.psr
                        pass

                # Re-encode the log
                if isinstance(message, Message):
                    encoded_message = message.to_ascii()

if __name__ == "__main__":
    main()
