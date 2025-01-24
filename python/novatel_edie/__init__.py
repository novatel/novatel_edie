import importlib_resources

from .bindings import messages, enums, MESSAGE_SIZE_MAX, HEADER_FORMAT, ENCODE_FORMAT, STATUS, string_to_encode_format, pretty_version, Framer, HeaderDecoder, MessageDecoder, Encoder, Filter, DecoderException, Logging

def default_json_db_path():
    """Returns a context manager that yields the path to the default JSON database."""
    return importlib_resources.as_file(importlib_resources.files("novatel_edie").joinpath("messages_public.json"))
