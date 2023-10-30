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
import os

import novatel_edie as ne
from novatel_edie import Logging, LogLevel, STATUS


def _configure_logging():
    root_logger = Logging().get("root")
    root_logger.set_level(LogLevel.INFO)
    Logging.add_console_logging(root_logger)
    Logging.add_rotating_file_logger(root_logger)
    atexit.register(Logging.shutdown)


def read_as_frames(input_file, framer):
    with open(input_file, "rb") as input_stream:
        while read_data := input_stream.read(ne.MAX_ASCII_MESSAGE_LENGTH):
            framer.write(read_data)
            while True:
                status, frame, metadata = framer.get_frame()
                if status in [STATUS.BUFFER_EMPTY, STATUS.INCOMPLETE]:
                    break
                yield status, frame, metadata


def main():
    _configure_logging()
    logger = Logging().register_logger("converter")

    logger.info(f"Decoder library information:\n{ne.pretty_version}")

    parser = argparse.ArgumentParser(
        description="Convert OEM log files using low-level components."
    )
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

    if args.version:
        exit(0)

    if not os.path.exists(args.input_file):
        logger.error(f'File "{args.input_file}" does not exist')
        exit(1)

    # Setup the EDIE components
    framer = ne.Framer()
    framer.set_report_unknown_bytes(True)
    framer.set_payload_only(False)
    framer.set_frame_json(False)

    header_decoder = ne.HeaderDecoder()
    message_decoder = ne.MessageDecoder()
    encoder = ne.Encoder()
    filter = ne.Filter()

    # Set up file streams
    converted_logs_stream = ne.OutputFileStream(f"{args.input_file}.{encode_format}")
    unknown_bytes_stream = ne.OutputFileStream(f"{args.input_file}.UNKNOWN")

    for framer_status, frame, meta in read_as_frames(args.input_file, framer):
        try:
            if framer_status == STATUS.UNKNOWN:
                unknown_bytes_stream.write(frame)
            else:
                framer_status.raise_on_error("Framer.get_frame() failed")
            if meta.response:
                unknown_bytes_stream.write(frame)
                continue
            logger.info(f"Framed: {frame}")

            # Decode the header.  Get meta data here and populate the Intermediate header.
            status, header = header_decoder.decode(frame, meta)
            status.raise_on_error("HeaderDecoder.decode() failed")

            # Filter the log, pass over this log if we don't want it.
            if not filter.do_filtering(meta):
                continue

            # Decode the Log, pass the meta data and populate the intermediate log.
            body = frame[meta.header_length :]
            status, message = message_decoder.decode(body, meta)
            status.raise_on_error("MessageDecoder.decode() failed")

            status, encoded_message = encoder.encode(header, message, meta, encode_format)
            status.raise_on_error("Encoder.encode() failed")

            converted_logs_stream.write(encoded_message.message)
            logger.info(f"Encoded: ({len(encoded_message.message)}) {encoded_message.message}")
        except ne.DecoderException as e:
            logger.warn(str(e))

    # Clean up
    unparsed_bytes = framer.flush()
    unknown_bytes_stream.write(unparsed_bytes)


if __name__ == "__main__":
    main()
