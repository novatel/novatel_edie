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
#                            DESCRIPTION
#
# \file rxconfig_example.cpp
# \brief Demonstrate how to use the C++ source for converting RXCONFIG
# messages.
########################################################################


import os
import sys
import timeit

import novatel_edie as ne
import spdlog as spd


def main():
    # This example uses the default logger config, but you can also pass a config file to the Logger() ctor
    # An example config file: doc\example_logger_config.toml
    logger = Logger().RegisterLogger("rxconfig_converter")
    logger.set_level(spd.LogLevel.DEBUG)
    Logger.AddConsoleLogging(logger)
    Logger.AddRotatingFileLogger(logger)

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

    # Setup filestreams
    ifs = ne.InputFileStream(infilename)
    convertedrxconfigofs = ne.OutputFileStream(f"{infilename}.{encodeformat}")
    strippedrxconfigofs = ne.OutputFileStream(f"{infilename}.STRIPPED.{encodeformat}")

    metadata = ne.MetaDataStruct()
    embeddedmetadata = ne.MetaDataStruct()
    messagedata = ne.MessageDataStruct()
    embeddedmessagedata = ne.MessageDataStruct()

    rxconfighandler = ne.RxConfigHandler(json_db)

    while True:
        readdata.data = ifsreadbuffer
        readstatus = ifs.ReadData(readdata)
        rxconfighandler.Write(readdata)

        while True:
            status = rxconfighandler.Convert(
                messagedata, metadata, embeddedmessagedata, embeddedmetadata, encode_format
            )
            if status == ne.STATUS.SUCCESS:
                messagedata.message[messagedata.messagelength] = "\0"
                logger.info(f"Encoded: ({messagedata.messagelength}) {messagedata.message}")
                convertedrxconfigofs.WriteData(messagedata)

                # Make the embedded message valid by flipping the CRC.
                if encode_format == ne.ENCODEFORMAT.ASCII:
                    # Flip the CRC at the end of the embedded message and add a CRLF so it becomes a valid command.
                    crcbegin = embeddedmessagedata[-ne.OEM4_ASCII_CRC_LENGTH :]
                    flippedcrc = strtoul(crcbegin, NULL, 16) ^ 0xFFFFFFFF
                    snprintf(crcbegin, OEM4_ASCII_CRC_LENGTH + 1, "%08x", flippedcrc)
                    strippedrxconfigofs.WriteData(embeddedmessagedata)
                    strippedrxconfigofs.WriteData(b"\r\n")
                elif encode_format == ne.ENCODEFORMAT.BINARY:
                    # Flip the CRC at the end of the embedded message so it becomes a valid command.
                    crcbegin = embeddedmessagedata[-ne.OEM4_BINARY_CRC_LENGTH :]
                    crcbegin ^= 0xFFFFFFFF
                    strippedrxconfigofs.WriteData(embeddedmessagedata)
                elif encode_format == ne.ENCODEFORMAT.JSON:
                    # Write in a comma and CRLF to make the files parse-able by JSON readers.
                    convertedrxconfigofs.WriteData(b",\r\n")
                    strippedrxconfigofs.WriteData(b",\r\n")
            if status == ne.STATUS.BUFFER_EMPTY:
                break

        if readstatus.eos:
            break

    Logger.Shutdown()


if __name__ == "__main__":
    main()
