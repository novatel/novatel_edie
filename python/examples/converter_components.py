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
import atexit
from binascii import hexlify
import os

import novatel_edie as ne
from novatel_edie import Logging, LogLevel, STATUS


def configure_logging():
    root_logger = Logging().get("root")
    root_logger.set_level(LogLevel.INFO)
    Logging.add_console_logging(root_logger)
    Logging.add_rotating_file_logger(root_logger)
    atexit.register(Logging.shutdown)


def read_frames(input_file, framer):
    # Write unrecognized and incomplete messages to a separate file.
    unknown_bytes_stream = ne.OutputFileStream(f"{input_file}.UNKNOWN")
    with open(input_file, "rb") as input_stream:
        while read_data := input_stream.read(ne.MESSAGE_SIZE_MAX):
            framer.write(read_data)
            while True:
                status, frame, meta = framer.get_frame()
                if status in [STATUS.BUFFER_EMPTY, STATUS.INCOMPLETE]:
                    break
                if status == STATUS.UNKNOWN or meta.response:
                    unknown_bytes_stream.write(frame)
                    continue
                yield status, frame, meta
    unknown_bytes_stream.write(framer.flush())

def format_frame(frame, frame_format):
    if frame_format in [ne.HEADERFORMAT.BINARY, ne.HEADERFORMAT.SHORT_BINARY, ne.HEADERFORMAT.PROPRIETARY_BINARY,
                        ne.ENCODEFORMAT.BINARY, ne.ENCODEFORMAT.FLATTENED_BINARY]:
        return hexlify(frame, sep=" ").decode("ascii").upper()
    return frame

def main():
    configure_logging()
    logger = Logging().register_logger("converter")

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
    header_decoder = ne.HeaderDecoder()
    message_decoder = ne.MessageDecoder()
    encoder = ne.Encoder()
    filter = ne.Filter()

    converted_logs_stream = ne.OutputFileStream(f"{args.input_file}.{encode_format}")

    for framer_status, frame, meta in read_frames(args.input_file, framer):
        try:
            framer_status.raise_on_error("Framer.get_frame() failed")
            logger.info(f"Framed ({len(frame)}): {format_frame(frame, meta.format)}")

            # Decode the header.
            status, header = header_decoder.decode(frame, meta)
            status.raise_on_error("HeaderDecoder.decode() failed")

            # Filter the log, pass over it if we don't want it.
            if not filter.do_filtering(meta):
                continue

            # Decode the log body.
            body = frame[meta.header_length :]
            status, message = message_decoder.decode(body, meta)
            status.raise_on_error("MessageDecoder.decode() failed")

            # Re-encode the log and write it to the output file.
            status, encoded_message = encoder.encode(header, message, meta, encode_format)
            status.raise_on_error("Encoder.encode() failed")

            converted_logs_stream.write(encoded_message.message)
            logger.info(f"Encoded ({len(encoded_message.message)}): {format_frame(encoded_message.message, encode_format)}")
        except ne.DecoderException as e:
            logger.warn(str(e))


if __name__ == "__main__":
    main()
