"""
MIT

Copyright (c) 2023 NovAtel Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"""

import importlib_resources

from .python_common import (
    _internal,  enable_internal_logging, disable_internal_logging,
    STATUS, TIME_STATUS, MEASUREMENT_SOURCE,
    NovatelEdieException, FailureException, UnknownException, IncompleteException, IncompleteMoreDataException,
    NullProvidedException, NoDatabaseException, NoDefinitionException, NoDefinitionEmbeddedException,
    BufferFullException, BufferEmptyException, StreamEmptyException, UnsupportedException,
    MalformedInputException, DecompressionFailureException, JsonDbReaderException,
    throw_exception_from_status,
    FIELD_TYPE, DATA_TYPE,
    MessageDatabase, get_builtin_database,
    MessageDefinition, FieldDefinition, ArrayFieldDefinition, FieldArrayFieldDefinition, EnumFieldDefinition,
    EnumDataType,
    ENCODE_FORMAT,
    MAX_MESSAGE_LENGTH, MAX_ASCII_MESSAGE_LENGTH, MAX_SHORT_ASCII_MESSAGE_LENGTH, MAX_BINARY_MESSAGE_LENGTH, MAX_SHORT_BINARY_MESSAGE_LENGTH,
    MAX_ABB_ASCII_RESPONSE_LENGTH, MAX_NMEA_MESSAGE_LENGTH,
    CPP_VERSION, CPP_PRETTY_VERSION, GIT_SHA, GIT_BRANCH, GIT_IS_DIRTY, BUILD_TIMESTAMP,
    calculate_crc, SatelliteId,
    UnknownBytes, Field, MessageData,
)

from .python_oem import (
    HEADER_FORMAT,
    NMEA_SYNC_LENGTH, NMEA_CRC_LENGTH,
    OEM4_BINARY_HEADER_LENGTH, OEM4_SHORT_BINARY_HEADER_LENGTH,
    OEM4_BINARY_CRC_LENGTH, OEM4_ASCII_CRC_LENGTH,
    OEM4_BINARY_SYNC_LENGTH, OEM4_SHORT_ASCII_SYNC_LENGTH, OEM4_ASCII_SYNC_LENGTH, OEM4_SHORT_BINARY_SYNC_LENGTH,
    OEM4_BINARY_SYNC1, OEM4_BINARY_SYNC2, OEM4_BINARY_SYNC3,
    Header, UnknownMessage, Message, Response, GpsTime,
    Oem4BinaryHeader, Oem4BinaryShortHeader,  MetaData,
    Framer, Filter, Decoder, Commander, Parser, FileParser,
    RangeDecompressor, RxConfigHandler,
    messages, enums, _internal as _novatel_internal
)

def default_json_db_path():
    """Returns a context manager that yields the path to the default JSON database."""
    return importlib_resources.as_file(importlib_resources.files("novatel_edie").joinpath("database.json"))
