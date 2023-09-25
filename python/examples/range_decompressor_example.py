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

import os
import sys
import timeit

import novatel_edie as ne
import spdlog as spd


def main():
    logger = Logger().RegisterLogger("range_decompressor")
    logger.set_level(spd.LogLevel.DEBUG)
    Logger.AddConsoleLogging(logger)
    Logger.AddRotatingFileLogger(logger)

    if "-V" in sys.argv:
        logger.info(f"Decoder library information:\n{ne.get_pretty_version()}")
        exit(0)

    encodeformat = "ASCII"

    if len(sys.argv) - 1 < 3:
        logger.info("ERROR: Need to specify a JSON message definitions DB, an input file and an output format.\n")
        logger.info("Example: converter <path to Json DB> <path to input file> <output format>\n")
        return -1
    if len(sys.argv) - 1 == 4:
        encodeformat = sys.argv[3]

    if not os.path.exists(sys.argv[1]):
        logger.error(f'File "{sys.argv[1]}" does not exist')
        exit(1)
    if not os.path.exists(sys.argv[2]):
        logger.error(f'File "{sys.argv[2]}" does not exist')
        exit(1)

    json_db = sys.argv[1]
    infilename = sys.argv[2]

    encode_format_str = sys.argv[1]
    encode_format = ne.string_to_encode_format(encode_format_str)
    if encode_format == ne.ENCODEFORMAT.UNSPECIFIED:
        logger.error("Unspecified output format.\n\tASCII\n\tBINARY\n\tFLATTENED_BINARY")
        return -1

    logger.info(f"Decoder library information:\n{ne.get_pretty_version()}")

    jsondb = ne.JsonReader()
    logger.info("Loading Database... ")
    start = timeit.default_timer()
    jsondb.LoadFile(jsondb)
    logger.info(f"DONE ({timeit.default_timer() - start:.0f} ms)")

    framer = ne.Framer()
    headerdecoder = ne.HeaderDecoder(json_db)
    messagedecoder = ne.MessageDecoder(json_db)
    rangedecompressor = ne.RangeDecompressor(json_db)
    encoder = ne.Encoder(json_db)

    framer.SetLoggerLevel(spd.LogLevel.DEBUG)
    headerdecoder.SetLoggerLevel(spd.LogLevel.DEBUG)
    messagedecoder.SetLoggerLevel(spd.LogLevel.DEBUG)
    encoder.SetLoggerLevel(spd.LogLevel.DEBUG)
    Logger.AddConsoleLogging(framer.GetLogger())
    Logger.AddConsoleLogging(headerdecoder.GetLogger())
    Logger.AddConsoleLogging(messagedecoder.GetLogger())
    Logger.AddRotatingFileLogger(framer.GetLogger())
    Logger.AddRotatingFileLogger(headerdecoder.GetLogger())
    Logger.AddRotatingFileLogger(messagedecoder.GetLogger())

    framer.SetFrameJson(False)
    framer.SetPayloadOnly(False)
    framer.SetReportUnknownBytes(True)

    ifs = ne.InputFileStream(infilename)
    ofs = ne.OutputFileStream(f"{infilename}.DECOMPRESSED.{encodeformat}")

    header = ne.IntermediateHeader()
    message = ne.IntermediateMessage()

    metadata = ne.MetaDataStruct()
    messagedata = ne.MessageDataStruct()

    start = timeit.default_timer()
    completedmessages = 0
    while True:
        readbuffer = framebuffer

        # Get frame, null-terminate.
        status = framer.GetFrame(readbuffer, metadata)
        if status == ne.STATUS.SUCCESS:
            # Decode the header.  Get meta data here and populate the Intermediate header.
            status = headerdecoder.Decode(readbuffer, header, metadata)
            if status == ne.STATUS.SUCCESS:
                status = rangedecompressor.Decompress(readbuffer, metadata, encode_format)
                if status == ne.STATUS.SUCCESS:
                    completedmessages += 1
                    byteswritten = ofs.WriteData(readbuffer, metadata.length)
                    if metadata.length == byteswritten:
                        readbuffer[metadata.length] = "\0"
                        logger.info(f"Decompressed: ({metadata.length}) {readbuffer}")
                    else:
                        logger.error(f"Could only write {byteswritten}/{messagedata.messagelength} bytes.")
                elif status == ne.STATUS.UNSUPPORTED:
                    if status == ne.STATUS.SUCCESS:
                        header.messageid = metadata.messageid
                        status = messagedecoder.Decode((readbuffer + metadata.headerlength), message, metadata)
                        if status == ne.STATUS.SUCCESS:
                            # Encode our message now that we have everything we need.
                            encodedmessagebuffer, status = encoder.Encode(
                                header,
                                message,
                                messagedata,
                                metadata,
                                encode_format,
                            )
                            if status == ne.STATUS.SUCCESS:
                                messagedata.message[messagedata.messagelength] = "\0"
                                logger.info(f"Encoded: ({messagedata.messagelength}) {messagedata.message}")
        elif (status == ne.STATUS.BUFFER_EMPTY) or (status == ne.STATUS.INCOMPLETE):
            # Read from file, write to framer.
            readstatus = ifs.ReadData(readdata)
            if readstatus.currentstreamread == 0:
                logger.info("Stream finished")
                break

            framer.Write(readdata)

    elapsed_seconds = timeit.default_timer() - start
    logger.info(
        f"Decoded {completedmessages} messages in {elapsed_seconds}s. ({completedmessages / elapsed_seconds:.1f} msg/s)"
    )

    Logger.Shutdown()


if __name__ == "__main__":
    main()
