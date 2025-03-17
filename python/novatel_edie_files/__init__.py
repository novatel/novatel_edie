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

from .bindings import (
    HEADER_FORMAT, ENCODE_FORMAT, STATUS, FIELD_TYPE, DATA_TYPE, TIME_STATUS, MEASUREMENT_SOURCE,
    MESSAGE_SIZE_MAX, MAX_ASCII_MESSAGE_LENGTH, MAX_SHORT_ASCII_MESSAGE_LENGTH, MAX_BINARY_MESSAGE_LENGTH, MAX_SHORT_BINARY_MESSAGE_LENGTH,
    MAX_ABB_ASCII_RESPONSE_LENGTH, MAX_NMEA_MESSAGE_LENGTH,
    NMEA_SYNC_LENGTH, NMEA_CRC_LENGTH,
    OEM4_BINARY_HEADER_LENGTH, OEM4_SHORT_BINARY_HEADER_LENGTH,
    OEM4_BINARY_CRC_LENGTH, OEM4_ASCII_CRC_LENGTH,
    OEM4_BINARY_SYNC_LENGTH, OEM4_SHORT_ASCII_SYNC_LENGTH, OEM4_ASCII_SYNC_LENGTH, OEM4_SHORT_BINARY_SYNC_LENGTH,
    OEM4_BINARY_SYNC1, OEM4_BINARY_SYNC2, OEM4_BINARY_SYNC3,
    NovatelEdieException, FailureException, UnknownException, IncompleteException, IncompleteMoreDataException,
    NullProvidedException, NoDatabaseException, NoDefinitionException, NoDefinitionEmbeddedException,
    BufferFullException, BufferEmptyException, StreamEmptyException, UnsupportedException,
    MalformedInputException, DecompressionFailureException, JsonDbReaderException,
    string_to_encode_format, pretty_version,
    UnknownBytes, Header, Field, UnknownMessage, Message,
    MessageDatabase, get_default_database,
    Oem4BinaryHeader, Oem4BinaryShortHeader, MetaData, MessageData, MessageDefinition, BaseField,
    Framer, Filter, Decoder, DecoderException, Commander, Parser, FileParser,
    RangeDecompressor, RxConfigHandler,
    Logging, LogLevel,
    messages, enums, throw_exception_from_status,
    EnumField, EnumDataType,
    _internal
)

def default_json_db_path():
    """Returns a context manager that yields the path to the default JSON database."""
    return importlib_resources.as_file(importlib_resources.files("novatel_edie").joinpath("database.json"))
