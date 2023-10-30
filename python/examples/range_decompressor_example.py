#!/usr/bin/env python3
############################################################################
#
# Copyright (c) 2021 NovAtel Inc.
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
############################################################################
#
#  DESCRIPTION:
#    An example program using the low-level EDIE components to filter for
#    and decompress rangecmp logs.
#
############################################################################
import argparse
import atexit
import os
import timeit

import novatel_edie as ne
from novatel_edie import Logging, LogLevel


def _configure_logging(logger):
    logger.set_level(LogLevel.DEBUG)
    Logging.add_console_logging(logger)
    Logging.add_rotating_file_logger(logger)


def main():
    logger = Logging().register_logger("range_decompressor")
    _configure_logging(logger)
    atexit.register(Logging.shutdown)

    logger.info(f"Decoder library information:\n{ne.pretty_version}")

    parser = argparse.ArgumentParser(description="Filter and decompress RANGECMP OEM logs.")
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
        logger.error(f'File "{args.input_file}" does not exist')
        exit(1)

    framer = ne.Framer()
    framer.set_frame_json(False)
    framer.set_payload_only(False)
    framer.set_report_unknown_bytes(True)

    header_decoder = ne.HeaderDecoder()
    message_decoder = ne.MessageDecoder()
    range_decompressor = ne.RangeDecompressor()
    encoder = ne.Encoder()

    _configure_logging(framer.logger)
    _configure_logging(header_decoder.logger)
    _configure_logging(message_decoder.logger)
    _configure_logging(encoder.logger)

    input_stream = open(args.input_file, "rb")
    output_stream = ne.OutputFileStream(f"{args.input_file}.DECOMPRESSED.{encode_format}")

    meta = ne.MetaData()
    start = timeit.default_timer()
    completed_messages = 0
    while True:
        # Get frame, null-terminate.
        status, read_bytes = framer.get_frame(meta)
        if status in [ne.STATUS.BUFFER_EMPTY, ne.STATUS.INCOMPLETE]:
            # Read from file, write to framer.
            read_bytes = input_stream.read(ne.MESSAGE_SIZE_MAX)
            if len(read_bytes) == 0:
                logger.info("Stream finished")
                break
            framer.write(read_bytes)
        if status != ne.STATUS.SUCCESS:
            logger.error(f"Failed to get a frame: {status}: {status.__doc__}")
            continue

        # Decode the header.  Get meta data here and populate the Intermediate header.
        status, header = header_decoder.decode(read_bytes, meta)
        if status != ne.STATUS.SUCCESS:
            logger.error(f"Failed to decode a header: {status}: {status.__doc__}")
            continue

        status, message_data = range_decompressor.decompress(read_bytes, meta, encode_format)
        if status != ne.STATUS.SUCCESS:
            logger.error(f"Failed to decompress a message: {status}: {status.__doc__}")
            continue

        if status == ne.STATUS.UNSUPPORTED:
            header.message_id = meta.message_id
            body = read_bytes[meta.headerlength:]
            status, message = message_decoder.decode(body, meta)
            if status == ne.STATUS.SUCCESS:
                # Encode our message now that we have everything we need.
                status, encoded_message = encoder.encode(header, message, message_data, meta, encode_format)
                if status == ne.STATUS.SUCCESS:
                    logger.info(f"Encoded: ({len(message_data.message)}) {message_data.message}")
        else:
            completed_messages += 1
            written_bytes = output_stream.write(message_data)
            if len(message_data) == written_bytes:
                logger.info(f"Decompressed: ({len(message_data)}) {message_data}")
            else:
                logger.error(f"Could only write {written_bytes}/{len(message_data)} bytes.")

    elapsed_seconds = timeit.default_timer() - start
    logger.info(
        f"Decoded {completed_messages} messages in {elapsed_seconds}s. ({completed_messages / elapsed_seconds:.1f} msg/s)"
    )


if __name__ == "__main__":
    main()
