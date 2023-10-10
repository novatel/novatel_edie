import importlib.resources

from .decoders_bindings import *
from .stream_interface_bindings import *


def default_json_db_path():
    """Returns a context manager that yields the path to the default JSON database."""
    return importlib.resources.path("novatel_edie", "messages_public.json")


def load_message_database(path=None):
    if path:
        return JsonReader(str(path))
    with default_json_db_path() as db_path:
        return JsonReader(str(db_path))
