import pathlib

from .stream_interface_bindings import *
from .decoders_bindings import *

JSON_DB_PATH = pathlib.Path(__file__).parent / "messages_public.json"

def load_message_database(path=None):
    if path:
        return JsonReader(str(path))
    return JsonReader(str(JSON_DB_PATH))
