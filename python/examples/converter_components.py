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
import timeit

import novatel_edie as ne
from novatel_edie import Logger, LogLevel


def _configure_logging(logger):
    logger.set_level(LogLevel.DEBUG)
    Logger.add_console_logging(logger)
    Logger.add_rotating_file_logger(logger)


def main():
    logger = Logger().register_logger("converter")
    _configure_logging(logger)

    # Get command line arguments
    logger.info(f"Decoder library information:\n{ne.pretty_version}")

    encode_format_str = "ASCII"
    if "-V" in sys.argv:
        exit(0)
    if len(sys.argv) < 3:
        logger.error("ERROR: Need to specify an input file and an output format.")
        logger.error("Example: converter <path to input file> <output format>")
        exit(1)
    if len(sys.argv) == 3:
        encode_format_str = sys.argv[2]

    in_filename = sys.argv[1]
    if not os.path.exists(in_filename):
        logger.error(f'File "{in_filename}" does not exist')
        exit(1)

    encode_format = ne.string_to_encode_format(encode_format_str)
    if encode_format == ne.ENCODE_FORMAT.UNSPECIFIED:
        logger.error("Unspecified output format.\n\tASCII\n\tBINARY\n\tFLATTENED_BINARY")
        exit(1)

    # Load the database
    logger.info("Loading Database... ")
    t0 = timeit.default_timer()
    json_db = ne.load_message_database()
    t1 = timeit.default_timer()
    logger.info(f"Done in {(t1 - t0) * 1e3:.0f} ms")

    # Setup timers
    loop = timeit.default_timer()

    # Setup the EDIE components
    framer = ne.Framer()
    framer.set_report_unknown_bytes(True)
    framer.set_payload_only(False)
    framer.set_frame_json(False)
    _configure_logging(framer.logger)

    header_decoder = ne.HeaderDecoder(json_db)
    _configure_logging(header_decoder.logger)

    message_decoder = ne.MessageDecoder(json_db)
    _configure_logging(message_decoder.logger)

    encoder = ne.Encoder(json_db)
    _configure_logging(encoder.logger)

    filter = ne.Filter()
    _configure_logging(filter.logger)

    # Setup filestreams
    ifs = ne.InputFileStream(in_filename)
    converted_logs_ofs = ne.OutputFileStream(f"{in_filename}.{encode_format}")
    unknown_bytes_ofs = ne.OutputFileStream(f"{in_filename}.UNKNOWN")

    read_status = ne.StreamReadStatus()
    while not read_status.eos:
        read_data, read_status = ifs.read()
        framer.write(read_data)
        # Clearing INCOMPLETE status when internal buffer needs more bytes.
        framer_status = ne.STATUS.INCOMPLETE_MORE_DATA

        while framer_status != ne.STATUS.BUFFER_EMPTY and framer_status != ne.STATUS.INCOMPLETE:
            _, frame_buffer, metadata = framer.get_frame()

            if framer_status == ne.STATUS.SUCCESS:
                if metadata.response:
                    unknown_bytes_ofs.write(frame_buffer, metadata.length)
                    continue

                logger.info(f"Framed: {frame_buffer}")

                # Decode the header.  Get meta data here and populate the Intermediate header.
                decoder_status, intermediate_header, metadata = header_decoder.decode(frame_buffer, metadata)

                if decoder_status == ne.STATUS.SUCCESS:
                    # Filter the log, pass over this log if we don't want it.
                    if not filter.do_filtering(metadata):
                        continue

                    frame_buffer += metadata.header_length
                    # Decode the Log, pass the meta data and populate the intermediate log.
                    decoder_status, intermediate_message, metadata = message_decoder.decode(frame_buffer, metadata)

                    if decoder_status == ne.STATUS.SUCCESS:
                        encoder_status, encoded_message_buffer, message_data = encoder.encode(
                            header, message, metadata, encode_format
                        )

                        if encoder_status == ne.STATUS.SUCCESS:
                            converted_logs_ofs.WriteData(message_data)
                            message_data.message[message_data.messagelength] = "\0"
                            logger.info(f"Encoded: ({len(message_data.message)}) {encoded_message_buffer}")
                        else:
                            unknown_bytes_ofs.write(frame_buffer, metadata.length)
                            logger.warn(f"Encoder returned with status code {int(encoder_status)}")
                    else:
                        unknown_bytes_ofs.write(frame_buffer, metadata.length)
                        logger.warn(f"MessageDecoder returned with status code {int(decoder_status)}")
                else:
                    unknown_bytes_ofs.write(frame_buffer, metadata.length)
                    logger.warn(f"HeaderDecoder returned with status code {int(decoder_status)}")
            elif framer_status == ne.STATUS.UNKNOWN:
                unknown_bytes_ofs.write(frame_buffer, metadata.length)
            else:
                logger.warn(f"Framer returned with status code {int(framer_status)}")

    # Clean up
    frame_buffer = framer.flush(True)
    unknown_bytes_ofs.write(frame_buffer)

    Logger.shutdown()


if __name__ == "__main__":
    main()
