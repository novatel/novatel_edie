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
# \file converter_parser.cpp
# \brief Demonstrate how to use the C++ source for converting OEM
# messages using the Parser.
########################################################################


import os
import sys
import timeit

import novatel_edie as ne
import spdlog as spd


def main():
    logger = spd.ConsoleLogger("CommandEncoder")
    logger.set_level(spd.LogLevel.DEBUG)

    # Get command line arguments
    logger.info(f"Decoder library information:\n{ne.get_pretty_version()}")

    encodeformat = "ASCII"
    if "-V" in sys.argv:
        exit(0)
    if len(sys.argv) - 1 < 3:
        logger.error("ERROR: Need to specify a JSON message definitions DB, an input file and an output format.")
        logger.error("Example: converter <path to Json DB> <path to input file> <output format>")
        exit(1)
    if len(sys.argv) - 1 == 4:
        encodeformat = sys.argv[3]

    # Check command line arguments
    jsondb = sys.argv[1]
    if not os.path.exists(jsondb):
        logger.error(f'File "{jsondb}" does not exist')
        exit(1)

    infilename = sys.argv[2]
    if not os.path.exists(infilename):
        logger.error(f'File "{infilename}" does not exist')
        exit(1)

    encode_format_str = sys.argv[1]
    encode_format = ne.string_to_encode_format(encode_format_str)
    if encode_format == ne.ENCODEFORMAT.UNSPECIFIED:
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

    parser = ne.Parser(json_db)
    parser.SetEncodeFormat(encode_format)
    parser.SetLoggerLevel(spd.LogLevel.DEBUG)
    Logger.AddConsoleLogging(parser.GetLogger())
    Logger.AddRotatingFileLogger(parser.GetLogger())

    filter = ne.Filter()
    filter.SetLoggerLevel(spd.LogLevel.DEBUG)
    Logger.AddConsoleLogging(filter.GetLogger())
    Logger.AddRotatingFileLogger(filter.GetLogger())

    parser.SetFilter(filter)

    # Initialize FS structures and buffers
    readstatus = ne.StreamReadStatus()
    readdata = ne.ReadDataStructure()

    # Setup filestreams
    ifs = ne.InputFileStream(infilename)
    convertedlogsofs = ne.OutputFileStream(f"{infilename}.{encodeformat}")
    ne.OutputFileStream(f"{infilename}.UNKNOWN")

    completemessages = 0
    counter = 0
    start = timeit.default_timer()
    loop = timeit.default_timer()
    while not readstatus.eos:
        readdata.data = ifsreadbuffer
        readstatus = ifs.ReadData(readdata)
        parser.Write(readdata)

        while True:
            status = parser.Read(messagedata, metadata)

            if status == ne.STATUS.SUCCESS:
                convertedlogsofs.WriteData(messagedata)
                messagedata.message[messagedata.messagelength] = "\0"
                logger.info(f"Encoded: ({messagedata.messagelength}) {messagedata.message}")
                completemessages += 1

            if timeit.default_timer() - loop > 1:
                counter += 1
                logger.info(f"{completemessages / counter} logs/s")
                loop = timeit.default_timer()
            if status == ne.STATUS.BUFFER_EMPTY:
                break
    logger.info(f"Converted {completemessages} logs in {timeit.default_timer() - start:.3f}s from {infilename}")

    Logger.Shutdown()


if __name__ == "__main__":
    main()
