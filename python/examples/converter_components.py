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

import argparse
import os
from binascii import hexlify

import novatel_edie as ne
from novatel_edie.messages import RANGE
from novatel_edie import STATUS, ENCODE_FORMAT


def read_frames(input_file, framer):
    # Write unrecognized and incomplete messages to a separate file.
    with open(input_file, "rb") as input_stream, open(f"{input_file}.UNKNOWN", "wb") as unknown_bytes_stream:
        while read_data := input_stream.read(ne.MESSAGE_SIZE_MAX):
            framer.write(read_data)
            while True:
                try:
                    frame, meta = framer.get_frame()
                except ne.BufferEmptyException:
                    break
                except ne.IncompleteException:
                    break
                except ne.UnknownException:
                    continue
                yield frame, meta
        unknown_bytes_stream.write(framer.flush())


def format_frame(frame, frame_format):
    if frame_format in [ne.HEADER_FORMAT.BINARY, ne.HEADER_FORMAT.SHORT_BINARY, ne.HEADER_FORMAT.PROPRIETARY_BINARY,
                        ne.ENCODE_FORMAT.BINARY, ne.ENCODE_FORMAT.FLATTENED_BINARY]:
        return hexlify(frame, sep=" ").decode("ascii").upper()
    return frame


def main():
    logger = ne.Logging.register_logger("converter")
    parser = argparse.ArgumentParser(description="Convert OEM log files using low-level components.")
    parser.add_argument("input_file", help="Input file")
    parser.add_argument(
        "output_format",
        nargs="?",
        choices=["ASCII", "ABBREV_ASCII", "BINARY", "FLATTENED_BINARY", "JSON"],
        help="Output format",
        default="ASCII",
    )
    parser.add_argument("-V", "--version", action="store_true")
    args = parser.parse_args()
    encode_format = ne.string_to_encode_format(args.output_format)

    logger.info(f"Decoder library information:\n{ne.pretty_version}")
    if args.version:
        exit(0)

    if not os.path.exists(args.input_file):
        logger.error(f'File "{args.input_file}" does not exist')
        exit(1)

    # Set up the EDIE components
    framer = ne.Framer()
    framer.set_report_unknown_bytes(True)
    framer.set_payload_only(False)
    framer.set_frame_json(False)
    decoder = ne.Decoder()
    filter = ne.Filter()

    with open(f"{args.input_file}.{encode_format}", "wb") as converted_logs_stream:
        for frame, meta in read_frames(args.input_file, framer):
            try:
                if meta.format == ne.HEADER_FORMAT.UNKNOWN:
                    continue
                logger.info(f"Framed ({len(frame)}): {format_frame(frame, meta.format)}")

                # Decode in oneshot
                message = decoder.decode(frame)


                # Decode piecewise
                header = decoder.decode_header(frame, meta)

                # Filter the log, pass over it if we don't want it.
                if not filter.do_filtering(meta):
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

                # Re-encode the log and write it to the output file.
                if isinstance(message, ne.CompleteMessage):
                    encoded_message = message.to_ascii()
                else:
                    pass
                # status.raise_on_error("Encoder.encode() failed")

            except Exception as e:
                logger.warn(str(e))
if __name__ == "__main__":
    main()
