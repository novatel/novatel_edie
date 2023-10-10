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
# \file converter_components.cpp
# \brief Demonstrate how to use the C++ source for converting OEM
# messages using the low-level components.
########################################################################
import argparse
import os
from traceback import format_exc

import novatel_edie as ne
from novatel_edie import Logger, LogLevel, STATUS


def _configure_logging(logger):
    logger.set_level(LogLevel.DEBUG)
    Logger.add_console_logging(logger)
    Logger.add_rotating_file_logger(logger)


def read_as_frames(ifs, framer):
    read_status = ne.StreamReadStatus()
    while not read_status.eos:
        read_data, read_status = ifs.read(ne.MAX_ASCII_MESSAGE_LENGTH)
        framer.write(read_data)
        status = STATUS.INCOMPLETE_MORE_DATA
        while status != STATUS.BUFFER_EMPTY and status != STATUS.INCOMPLETE:
            status, frame, metadata = framer.get_frame()
            yield status, frame, metadata


def main():
    logger = Logger().register_logger("converter")
    _configure_logging(logger)

    logger.info(f"Decoder library information:\n{ne.pretty_version}")

    parser = argparse.ArgumentParser(description="Convert OEM log files using low-level components.")
    parser.add_argument("input_file", help="Input file")
    parser.add_argument("output_format", nargs="?", choices=["ASCII", "BINARY", "FLATTENED_BINARY"],
                        help="Output format", default="ASCII")
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

    _configure_logging(framer.logger)
    _configure_logging(header_decoder.logger)
    _configure_logging(message_decoder.logger)
    _configure_logging(encoder.logger)
    _configure_logging(filter.logger)

    # Setup file streams
    ifs = ne.InputFileStream(args.input_file)
    converted_logs_stream = ne.OutputFileStream(f"{args.input_file}.{encode_format}")
    unknown_bytes_stream = ne.OutputFileStream(f"{args.input_file}.UNKNOWN")

    for framer_status, frame, metadata in read_as_frames(ifs, framer):
        try:
            if framer_status == STATUS.UNKNOWN:
                unknown_bytes_stream.write(frame)
            framer_status.raise_on_error("Framer.get_frame() failed.")
            if metadata.response:
                unknown_bytes_stream.write(frame)
                continue
            logger.info(f"Framed: {frame}")

            # Decode the header.  Get meta data here and populate the Intermediate header.
            status, header = header_decoder.decode(frame, metadata)
            status.raise_on_error("HeaderDecoder.decode() failed.")

            # Filter the log, pass over this log if we don't want it.
            if not filter.do_filtering(metadata):
                continue

            # Decode the Log, pass the meta data and populate the intermediate log.
            body = frame[metadata.header_length:]
            status, message = message_decoder.decode(body, metadata)
            status.raise_on_error("MessageDecoder.decode() failed.")

            status, encoded_message = encoder.encode(header, message, metadata, encode_format)
            status.raise_on_error("Encoder.encode() failed.")

            converted_logs_stream.write(encoded_message.message)
            logger.info(f"Encoded: ({len(encoded_message.message)}) {encoded_message.message}")
        except ne.DecoderException:
            logger.warn(format_exc())

    # Clean up
    unparsed_bytes = framer.flush()
    unknown_bytes_stream.write(unparsed_bytes)

    Logger.shutdown()


if __name__ == "__main__":
    main()
