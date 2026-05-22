################################################################################
#
# COPYRIGHT NovAtel Inc, 2024. All rights reserved.
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
################################################################################=

import pytest

from novatel_edie import MessageDatabase, FailureException
from novatel_edie.oem import Decoder
from novatel_edie.oem.enums import Datum

def test_message_db_enums(json_db):
    # Act
    datum_enum = json_db.get_enum_type_by_name("Datum")
    # Assert
    assert datum_enum.WGS84 == 61
    assert datum_enum.WGS84.name == "WGS84"
    assert datum_enum.WGS84 == Datum.WGS84

def test_append_messages(json_db: MessageDatabase):
    # Arrange
    new_db = MessageDatabase()
    bestpos_id = 42
    bestpos_def = json_db.get_msg_def(bestpos_id)
    bestpos_type = json_db.get_msg_type("BESTPOS")
    range_id = 43
    range_def = json_db.get_msg_def(range_id)
    range_type = json_db.get_msg_type("RANGE")

    # Act
    new_db.append_messages([bestpos_def, range_def])

    # Assert
    assert json_db.get_msg_def(bestpos_id) == bestpos_def
    assert json_db.get_msg_type("BESTPOS") is bestpos_type
    assert json_db.get_msg_def(range_id) == range_def
    assert json_db.get_msg_type("RANGE") is range_type

    assert new_db.get_msg_def(bestpos_id) == bestpos_def
    assert new_db.get_msg_type("BESTPOS") is not bestpos_type
    assert new_db.get_msg_type("BESTPOS") is not None
    assert new_db.get_msg_def(range_id) == range_def
    assert new_db.get_msg_type("RANGE") is not range_type
    assert new_db.get_msg_type("RANGE") is not None

def test_remove_message(json_db: MessageDatabase):
    # Arrange
    new_db = MessageDatabase()
    bestpos_id = 42
    bestpos_def = json_db.get_msg_def(bestpos_id)
    bestpos_type = json_db.get_msg_type("BESTPOS")
    range_id = 43
    range_def = json_db.get_msg_def(range_id)
    new_db.append_messages([bestpos_def, range_def])
    new_range_type = new_db.get_msg_type("RANGE")

    # Act
    new_db.remove_message(bestpos_id)

    # Assert
    assert json_db.get_msg_def(bestpos_id) == bestpos_def
    assert json_db.get_msg_type("BESTPOS") is bestpos_type

    assert new_db.get_msg_def(bestpos_id) is None
    assert new_db.get_msg_type("BESTPOS") is None
    assert new_db.get_msg_def(range_id) == range_def
    assert new_db.get_msg_type("RANGE") is new_range_type

def test_merge(json_db: MessageDatabase):
    """Tests that one databases messages can be merged into another."""
    # Arrange
    oem_minus_bestpos_db = json_db.clone()
    new_db_with_bestpos = MessageDatabase()
    bestpos_name = "BESTPOS"
    bestpos_msg_def = oem_minus_bestpos_db.get_msg_def(bestpos_name)
    new_db_with_bestpos.append_messages([bestpos_msg_def])
    bestpos_msg_type = new_db_with_bestpos.get_msg_type(bestpos_name)
    other_msg_name = "RANGE"
    other_msg_def = oem_minus_bestpos_db.get_msg_def(other_msg_name)
    oem_minus_bestpos_db.remove_message(bestpos_msg_def.log_id)
    other_msg_type = oem_minus_bestpos_db.get_msg_type(other_msg_name)

    # Act
    new_db_with_bestpos.merge(oem_minus_bestpos_db)

    # Assert
    assert new_db_with_bestpos.get_msg_def(bestpos_name) == bestpos_msg_def
    assert new_db_with_bestpos.get_msg_type(bestpos_name) is bestpos_msg_type
    assert new_db_with_bestpos.get_msg_def(other_msg_name) == other_msg_def
    assert new_db_with_bestpos.get_msg_type(other_msg_name) is not None
    assert new_db_with_bestpos.get_msg_type(other_msg_name) != other_msg_type

    # The source database must be unaffected by the merge.
    assert oem_minus_bestpos_db.get_msg_def(bestpos_name) is None
    assert oem_minus_bestpos_db.get_msg_type(bestpos_name) is None
    assert oem_minus_bestpos_db.get_msg_def(other_msg_name) == other_msg_def
    assert oem_minus_bestpos_db.get_msg_type(other_msg_name) is other_msg_type

def test_clone(json_db: MessageDatabase):
    """Tests that a cloned database exposes the same messages and types as the original."""
    # Arrange
    bestpos_name = "BESTPOS"
    bestpos_def = json_db.get_msg_def(bestpos_name)
    bestpos_type = json_db.get_msg_type(bestpos_name)
    range_name = "RANGE"
    range_def = json_db.get_msg_def(range_name)
    range_type = json_db.get_msg_type(range_name)

    # Act
    cloned_db = json_db.clone()

    # Assert
    assert cloned_db is not json_db
    assert cloned_db.get_msg_def(bestpos_name) == bestpos_def
    assert cloned_db.get_msg_type(bestpos_name) is bestpos_type
    assert cloned_db.get_msg_def(range_name) == range_def
    assert cloned_db.get_msg_type(range_name) is range_type

    assert json_db.get_msg_def(bestpos_name) == bestpos_def
    assert json_db.get_msg_type(bestpos_name) is bestpos_type
    assert json_db.get_msg_def(range_name) == range_def
    assert json_db.get_msg_type(range_name) is range_type
