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

import os
import sys

import novatel_edie as ne
from novatel_edie import Logger, LogLevel, STATUS


def _configure_logging(logger):
    logger.set_level(LogLevel.DEBUG)
    Logger.add_console_logging(logger)
    Logger.add_rotating_file_logger(logger)


def main():
    logger = Logger().register_logger("converter")
    _configure_logging(logger)

    # Get command line arguments
    logger.info(f"Decoder library information:\n{ne.pretty_version}")

    encode_format = ne.ENCODEFORMAT.ASCII
    if "-V" in sys.argv:
        exit(0)
    if len(sys.argv) < 3:
        logger.error("ERROR: Need to specify an input file and an output format.")
        logger.error("Example: converter <path to input file> <output format>")
        exit(1)
    if len(sys.argv) == 3:
        encode_format = ne.string_to_encode_format(sys.argv[2])

    in_filename = sys.argv[1]
    if not os.path.exists(in_filename):
        logger.error(f'File "{in_filename}" does not exist')
        exit(1)

    if encode_format == ne.ENCODE_FORMAT.UNSPECIFIED:
        logger.error("Unspecified output format.\n\tASCII\n\tBINARY\n\tFLATTENED_BINARY")
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
    ifs = ne.InputFileStream(in_filename)
    converted_logs_stream = ne.OutputFileStream(f"{in_filename}.{encode_format}")
    unknown_bytes_stream = ne.OutputFileStream(f"{in_filename}.UNKNOWN")

    read_status = ne.StreamReadStatus()
    while not read_status.eos:
        read_data, read_status = ifs.read(ne.MAX_ASCII_MESSAGE_LENGTH)
        framer.write(read_data)
        # Clearing INCOMPLETE status when internal buffer needs more bytes.
        framer_status = STATUS.INCOMPLETE_MORE_DATA

        while framer_status != STATUS.BUFFER_EMPTY and framer_status != STATUS.INCOMPLETE:
            framer_status, frame, metadata = framer.get_frame()
            if framer_status == STATUS.UNKNOWN:
                unknown_bytes_stream.write(frame)
            elif framer_status != STATUS.SUCCESS:
                logger.warn(f"Framer returned with status code {int(framer_status)}: {framer_status.__doc__}")
                continue
            if metadata.response:
                unknown_bytes_stream.write(frame)
                continue

            logger.info(f"Framed: {frame}")

            # Decode the header.  Get meta data here and populate the Intermediate header.
            status, header = header_decoder.decode(frame, metadata)
            if status != STATUS.SUCCESS:
                unknown_bytes_stream.write(frame)
                logger.warn(f"HeaderDecoder returned with status code {int(status)}: {status.__doc__}")
                continue

            # Filter the log, pass over this log if we don't want it.
            if not filter.do_filtering(metadata):
                continue

            raw_message = frame[metadata.header_length:]
            # Decode the Log, pass the meta data and populate the intermediate log.
            status, message = message_decoder.decode(raw_message, metadata)
            if status != STATUS.SUCCESS:
                unknown_bytes_stream.write(raw_message)
                logger.warn(f"MessageDecoder returned with status code {int(status)}: {status.__doc__}")
                continue

            status, encoded_message = encoder.encode(header, message, metadata, encode_format)
            if status != STATUS.SUCCESS:
                unknown_bytes_stream.write(raw_message)
                logger.warn(f"Encoder returned with status code {int(status)}: {status.__doc__}")
                continue

            converted_logs_stream.write(encoded_message.message)
            logger.info(f"Encoded: ({len(encoded_message.message)}) {encoded_message.message}")

    # Clean up
    unparsed_bytes = framer.flush()
    unknown_bytes_stream.write(unparsed_bytes)

    Logger.shutdown()


if __name__ == "__main__":
    main()
