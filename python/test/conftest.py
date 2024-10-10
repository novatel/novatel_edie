import os
from pathlib import Path

import novatel_edie as ne
import pytest


@pytest.fixture(scope="session", autouse=True)
def change_test_dir(tmpdir_factory):
    """
    Run all tests inside a temporary directory, so logs and other cruft gets cleaned up automatically.
    Pytest keeps the 3 last runs by default and the temp dir root can be overriden with a `--basetemp=` argument.
    """
    tmpdir = tmpdir_factory.mktemp("novatel_edie")
    os.chdir(tmpdir)


@pytest.fixture(scope="session")
def project_root():
    return Path(__file__).parent.parent.parent


@pytest.fixture(scope="session")
def stream_interface_test_resources(project_root):
    return project_root / "src" / "stream_interface" / "test" / "resources"


@pytest.fixture(scope="session")
def decoders_test_resources(project_root):
    return project_root / "src" / "decoders" / "oem" / "test" / "resources"


@pytest.fixture(scope="session")
def json_db_path():
    with ne.default_json_db_path() as json_db_path:
        return str(json_db_path)


@pytest.fixture(scope="session")
def json_db():
    return ne.get_default_database()


@pytest.fixture(scope="session")
def min_json_db():
    json = """
    {
      "enums": [
        {
          "name": "Responses",
          "_id": "0",
          "enumerators": []
        },
        {
          "name": "Commands",
          "_id": "0",
          "enumerators": []
        },
        {
          "name": "PortAddress",
          "_id": "0",
          "enumerators": []
        },
        {
          "name": "GPSTimeStatus",
          "_id": "0",
          "enumerators": []
        }
      ],
      "messages": []
    }
    """
    db = ne.JsonReader()
    db.parse_json(json)
    return db
