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
# \file converter_fileparser.cpp
# \brief Demonstrate how to use the C++ source for converting OEM
# messages using the FileParser.
########################################################################


import os
import sys
import timeit

import novatel_edie as ne
from novatel_edie import Logger, LogLevel


def main():
    logger = Logger().register_logger("converter")
    logger.set_level(LogLevel.DEBUG)
    Logger.add_console_logging(logger)
    Logger.add_rotating_file_logger(logger)

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

    infilename = sys.argv[1]
    if not os.path.exists(infilename):
        logger.error(f'File "{infilename}" does not exist')
        exit(1)

    if encode_format == ne.ENCODE_FORMAT.UNSPECIFIED:
        logger.error("Unspecified output format.\n\tASCII\n\tBINARY\n\tFLATTENED_BINARY")
        exit(1)

    fileparser = ne.FileParser()
    fileparser.logger.set_level(LogLevel.DEBUG)
    logger.add_console_logging(fileparser.logger)
    Logger.add_rotating_file_logger(fileparser.logger)

    filter = ne.Filter()
    filter.logger.set_level(LogLevel.DEBUG)
    Logger.add_console_logging(filter.logger)
    Logger.add_rotating_file_logger(filter.logger)

    # Initialize structures and error codes
    status = ne.STATUS.UNKNOWN

    metadata = ne.MetaData()

    fileparser.filter = filter
    fileparser.encode_format = encode_format

    # Setup filestreams
    ifs = ne.InputFileStream(infilename)
    convertedlogsofs = ne.OutputFileStream(f"{infilename}.{encode_format}")
    ne.OutputFileStream(f"{infilename}.UNKNOWN")

    if not fileparser.SetStream(ifs):
        logger.error("Input stream could not be set.  The stream is either unavailable or exhausted.")
        exit(-1)

    complete_messages = 0
    counter = 0
    start = timeit.default_timer()
    loop = timeit.default_timer()

    while status != ne.STATUS.STREAM_EMPTY:
        try:
            status = fileparser.Read(message_data, metadata)
            if status == ne.STATUS.SUCCESS:
                convertedlogsofs.WriteData(message_data)
                message_data.message[message_data.messagelength] = "\0"
                logger.info(f"Encoded: ({message_data.messagelength}) {message_data.message}")
                complete_messages += 1
        except Exception as e:
            logger.error(f"Exception thrown:  {__DATE__}, {__TIME__} \n{e}\n")
            exit(-1)

        if timeit.default_timer() - loop > 1:
            counter += 1
            logger.info(f"{fileparser.GetPercentRead()}% {complete_messages / counter} logs/s")
            loop = timeit.default_timer()
    logger.info(f"Converted {complete_messages} logs in {timeit.default_timer() - start:.3f}s from {infilename}")

    Logger.shutdown()


if __name__ == "__main__":
    main()
