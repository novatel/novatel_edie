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
    range_id = 43
    range_def = json_db.get_msg_def(range_id)

    # Act
    new_db.append_messages([bestpos_def, range_def])

    # Assert
    assert new_db.get_msg_def(bestpos_id) == bestpos_def
    assert new_db.get_msg_type("BESTPOS") is not None
    assert new_db.get_msg_def(range_id) == range_def
    assert new_db.get_msg_type("RANGE") is not None

def test_remove_message(json_db: MessageDatabase):
    # Arrange
    new_db = MessageDatabase()
    bestpos_id = 42
    bestpos_def = json_db.get_msg_def(bestpos_id)
    range_id = 43
    range_def = json_db.get_msg_def(range_id)
    new_db.append_messages([bestpos_def, range_def])
    new_range_type = new_db.get_msg_type("RANGE")

    # Act
    new_db.remove_message(bestpos_id)

    # Assert
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
    other_msg_type = oem_minus_bestpos_db.get_msg_type(other_msg_name)
    oem_minus_bestpos_db.remove_message(bestpos_msg_def.log_id)

    # Act
    new_db_with_bestpos.merge(oem_minus_bestpos_db)

    # Assert
    assert new_db_with_bestpos.get_msg_def(bestpos_name) == bestpos_msg_def
    assert new_db_with_bestpos.get_msg_type(bestpos_name) is bestpos_msg_type
    assert new_db_with_bestpos.get_msg_def(other_msg_name) == other_msg_def
    assert new_db_with_bestpos.get_msg_type(other_msg_name) is not None
    assert new_db_with_bestpos.get_msg_type(other_msg_name) != other_msg_type

def test_builtin_database_is_fixed(json_db: MessageDatabase):
    """The built-in database should be fixed and reject mutations."""
    bestpos_def = json_db.get_msg_def(42)

    with pytest.raises(FailureException, match="fixed"):
        json_db.merge(MessageDatabase())

    with pytest.raises(FailureException, match="fixed"):
        json_db.append_messages([bestpos_def])

    with pytest.raises(FailureException, match="fixed"):
        json_db.append_enumerations([])

    with pytest.raises(FailureException, match="fixed"):
        json_db.remove_message(42)

    with pytest.raises(FailureException, match="fixed"):
        json_db.remove_enumeration("Datum")

    with pytest.raises(FailureException, match="fixed"):
        json_db.message_family = "OEM"


def test_database_fixed_after_passing_to_decoder():
    """A database passed to a Decoder should become fixed."""
    db = MessageDatabase()
    assert not db.is_fixed
    Decoder(db)
    assert db.is_fixed

    with pytest.raises(FailureException, match="fixed"):
        db.append_messages([])

    with pytest.raises(FailureException, match="fixed"):
        db.merge(MessageDatabase())


def test_clone_unfixes_database(json_db: MessageDatabase):
    """Cloning a fixed database should produce an unfixed copy."""
    cloned = json_db.clone()
    assert not cloned.is_fixed
    # Should not raise
    cloned.remove_message(42)
    assert cloned.get_msg_def(42) is None
    # Original should be unaffected
    assert json_db.get_msg_def(42) is not None
