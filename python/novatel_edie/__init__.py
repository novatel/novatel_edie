import importlib_resources

from .bindings import (
    messages, enums,
    MESSAGE_SIZE_MAX, HEADER_FORMAT, ENCODE_FORMAT, STATUS, FIELD_TYPE, DATA_TYPE,
    TIME_STATUS, MEASUREMENT_SOURCE, OEM4_BINARY_HEADER_LENGTH, MAX_ASCII_MESSAGE_LENGTH,
    NMEA_SYNC_LENGTH, NMEA_CRC_LENGTH, OEM4_SHORT_BINARY_SYNC_LENGTH, OEM4_SHORT_BINARY_HEADER_LENGTH,
    OEM4_BINARY_CRC_LENGTH, OEM4_SHORT_ASCII_SYNC_LENGTH, OEM4_ASCII_CRC_LENGTH, MAX_SHORT_ASCII_MESSAGE_LENGTH,
    OEM4_BINARY_SYNC_LENGTH, MAX_BINARY_MESSAGE_LENGTH, OEM4_ASCII_SYNC_LENGTH, OEM4_BINARY_SYNC1, OEM4_BINARY_SYNC2,
    OEM4_BINARY_SYNC3,
    string_to_encode_format, pretty_version, get_default_database,
    Oem4BinaryHeader,
    MetaData, MessageData, Commander, JsonDbReader, BaseField, RangeDecompressor,
    Framer, HeaderDecoder, MessageDecoder, Encoder, Filter, DecoderException, Logging, LogLevel,
    Parser, RxConfigHandler, FileParser)

def default_json_db_path():
    """Returns a context manager that yields the path to the default JSON database."""
    return importlib_resources.as_file(importlib_resources.files("novatel_edie").joinpath("messages_public.json"))
