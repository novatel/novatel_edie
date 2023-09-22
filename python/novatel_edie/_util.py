import sys
from ctypes import CDLL
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent.absolute()
JSON_DB_PATH = SCRIPT_DIR / "messages_public.json"


def load_shared_library(lib_name):
    if sys.platform == "win32":
        prefix, ext = "", "dll"
    elif sys.platform == "darwin":
        prefix, ext = "lib", "dylib"
    else:
        prefix, ext = "lib", "so"
    return CDLL(str(SCRIPT_DIR / f"{prefix}{lib_name}.{ext}"))
