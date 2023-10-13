import importlib_resources

from .bindings import *


def default_json_db_path():
    """Returns a context manager that yields the path to the default JSON database."""
    return importlib_resources.as_file(importlib_resources.files("novatel_edie").joinpath("messages_public.json"))
