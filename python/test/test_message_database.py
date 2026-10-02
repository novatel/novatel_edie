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

import logging

import pytest

from novatel_edie import MessageDatabase, UnsupportedException
from novatel_edie.oem import Decoder, Parser, FileParser, Commander, RangeDecompressor, RxConfigHandler, get_builtin_database
from novatel_edie.oem.enums import Datum
from novatel_edie.oem.messages import ExtendedSolutionStatus
from novatel_edie import MessageDefinition, EnumFieldDefinition, EnumDefinition, EnumDataType
from novatel_edie import BitField, BitMask, BitMaskEntry, BitMaskDefinition
from novatel_edie import FieldDefinition, ArrayFieldDefinition, FieldArrayFieldDefinition, FIELD_TYPE, DATA_TYPE
from novatel_edie.oem import Header


def _status_bitmask_def(masks: dict = None) -> BitMaskDefinition:
    """A bitmask definition splitting a byte into two nibbles, or using `masks` if given."""
    if masks is None:
        masks = {"low": BitMaskEntry(BitMask.from_range(0, 4)), "high": BitMaskEntry(BitMask.from_range(4, 8))}
    return BitMaskDefinition(id="status_id", name="Status", masks=masks)


def _status_msg_def() -> MessageDefinition:
    """A message definition whose only field is a ULONG interpreted by the Status bitmask."""
    return MessageDefinition(
        id="status_msg", log_id=1, name="STATUSMSG", latest_message_crc=0,
        fields={0: [FieldDefinition(name="status", type=FIELD_TYPE.SIMPLE, data_type=DATA_TYPE.ULONG, bitmask_id="status_id")]})

class TestDatabaseObjects:
    """Tests that verify the interface for definition objects."""

    @pytest.mark.parametrize("values", [
        {},
        {"value": 2, "name": "val", "description": "a value of 2"},
        {"value": 0, "name": "error"}])
    class TestEnumDataType:
        """Tests for EnumDataType."""
        defaults = {
            "value": 0,
            "name": "",
            "description": ""
        }
        def test_construct(self, values: dict):
            # Act
            data = EnumDataType(**values)
            # Assert
            for attr, default in self.defaults.items():
                assert getattr(data, attr) == values.get(attr, default)

        def test_set_direct(self, values: dict):
            # Arrange
            data = EnumDataType()
            # Act
            for attr, value in values.items():
                setattr(data, attr, value)
            # Assert
            for attr, default in self.defaults.items():
                assert getattr(data, attr) == values.get(attr, default)

    @pytest.mark.parametrize("values", [
        {},
        {"id": "1", "name": "Datum", "enumerators": [EnumDataType(61, "WGS84", "WGS84 datum")]},
        {"id": "0", "name": "empty", "enumerators": []}])
    class TestEnumDefinition:
        """Tests for EnumDefinition."""
        defaults = {
            "id": "",
            "name": "",
            "enumerators": []
        }
        def test_construct(self, values: dict):
            # Act
            enum_def = EnumDefinition(**values)
            # Assert
            for attr, default in self.defaults.items():
                assert getattr(enum_def, attr) == values.get(attr, default)

        def test_set_direct(self, values: dict):
            # Arrange
            enum_def = EnumDefinition()
            # Act
            for attr, value in values.items():
                setattr(enum_def, attr, value)
            # Assert
            for attr, default in self.defaults.items():
                assert getattr(enum_def, attr) == values.get(attr, default)

    class TestBitMask:
        """Tests for BitMask."""
        def test_construct(self):
            # Act
            bitmask = BitMask(offset=4, width=3)
            # Assert
            assert bitmask.offset == 4
            assert bitmask.width == 3
            assert bitmask.mask == 0b1110000

        def test_from_range(self):
            # Act
            bitmask = BitMask.from_range(4, 7)
            # Assert
            assert bitmask == BitMask(offset=4, width=3)

        @pytest.mark.parametrize("offset, width", [(0, 0), (30, 3), (32, 1)])
        def test_invalid_construct_raises_value_error(self, offset: int, width: int):
            # Act / Assert
            with pytest.raises(ValueError):
                BitMask(offset, width)

        @pytest.mark.parametrize("start, end", [(4, 4), (5, 4), (0, 33)])
        def test_invalid_range_raises_value_error(self, start: int, end: int):
            # Act / Assert
            with pytest.raises(ValueError):
                BitMask.from_range(start, end)

    class TestBitMaskEntry:
        """Tests for BitMaskEntry."""
        def test_construct(self):
            # Arrange
            enum_def = EnumDefinition(id="1", name="Mode")
            # Act
            plain_entry = BitMaskEntry(BitMask(0, 1))
            enum_entry = BitMaskEntry(BitMask(1, 3), enum_def)
            # Assert
            assert plain_entry.bitmask == BitMask(0, 1)
            assert plain_entry.enum_def is None
            assert enum_entry.bitmask == BitMask(1, 3)
            assert enum_entry.enum_def.id == enum_def.id

        def test_set_direct(self):
            # Arrange
            entry = BitMaskEntry(BitMask(0, 1))
            # Act
            entry.bitmask = BitMask(2, 2)
            entry.enum_def = EnumDefinition(id="1", name="Mode")
            # Assert
            assert entry.bitmask == BitMask(2, 2)
            assert entry.enum_def.name == "Mode"
            # Act
            entry.enum_def = None
            # Assert
            assert entry.enum_def is None

    @pytest.mark.parametrize("values", [
        {},
        {"id": "7", "name": "Status", "masks": {"low": BitMaskEntry(BitMask(0, 4)), "high": BitMaskEntry(BitMask(4, 4))}},
        {"id": "0", "name": "empty", "masks": {}}])
    class TestBitMaskDefinition:
        """Tests for BitMaskDefinition."""
        defaults = {
            "id": "",
            "name": "",
            "masks": {}
        }
        def test_construct(self, values: dict):
            # Act
            bitmask_def = BitMaskDefinition(**values)
            # Assert
            for attr, default in self.defaults.items():
                assert getattr(bitmask_def, attr) == values.get(attr, default)

        def test_set_direct(self, values: dict):
            # Arrange
            bitmask_def = BitMaskDefinition()
            # Act
            for attr, value in values.items():
                setattr(bitmask_def, attr, value)
            # Assert
            for attr, default in self.defaults.items():
                assert getattr(bitmask_def, attr) == values.get(attr, default)

    class TestFieldDefinition:
        """Tests for FieldDefinition (BaseField)."""
        @pytest.mark.parametrize("values", [
            {},
            {"name": "field1", "type": FIELD_TYPE.SIMPLE, "conversion": "%d", "data_type": DATA_TYPE.INT},
            {"name": "field2", "type": FIELD_TYPE.SIMPLE, "conversion": "%.3f", "data_type": DATA_TYPE.DOUBLE},
            {"name": "field3", "type": FIELD_TYPE.SIMPLE, "data_type": DATA_TYPE.ULONG, "bitmask_id": "status_id"}])
        class TestValues:
            """Tests that values are set correctly."""
            defaults = {
                "name": "",
                "type": FIELD_TYPE.UNKNOWN,
                "conversion": "",
                "data_type": DATA_TYPE.UNKNOWN,
                "bitmask_id": ""
            }
            def test_construct(self, values: dict):
                # Act
                field = FieldDefinition(**values)
                # Assert
                for attr, default in self.defaults.items():
                    assert getattr(field, attr) == values.get(attr, default)

            def test_set_direct(self, values: dict):
                # Arrange
                field = FieldDefinition()
                # Act
                for attr, value in values.items():
                    setattr(field, attr, value)
                # Assert
                for attr, default in self.defaults.items():
                    assert getattr(field, attr) == values.get(attr, default)

        def test_bitmask_def_unresolved_outside_database(self):
            # Act
            field = FieldDefinition(name="status", bitmask_id="status_id")
            # Assert
            assert field.bitmask_def is None

        @pytest.mark.parametrize("invalid_conversion", [
            "",
            "d",
            "%q!",
            "%5.2"])
        def test_invalid_conversion_raises_attribute_error(self, invalid_conversion: str):
            # Arrange
            field = FieldDefinition()
            # Act / Assert
            with pytest.raises(AttributeError):
                field.conversion = invalid_conversion

    @pytest.mark.parametrize("values", [
        {},
        {"name": "field1", "type": FIELD_TYPE.SIMPLE, "conversion": "%d", "data_type": DATA_TYPE.ULONG, "enum_id": "42"},
        {"name": "field2"}])
    class TestEnumFieldDefinition:
        """Tests for EnumFieldDefinition."""
        defaults = {
            "name": "",
            "type": FIELD_TYPE.ENUM,
            "conversion": "",
            "data_type": DATA_TYPE.UNKNOWN,
            "enum_id": ""
        }
        def test_construct(self, values: dict):
            # Act
            field = EnumFieldDefinition(**values)
            # Assert
            for attr, default in self.defaults.items():
                assert getattr(field, attr) == values.get(attr, default)

        def test_set_direct(self, values: dict):
            # Arrange
            field = EnumFieldDefinition()
            # Act
            for attr, value in values.items():
                setattr(field, attr, value)
            # Assert
            for attr, default in self.defaults.items():
                assert getattr(field, attr) == values.get(attr, default)

    @pytest.mark.parametrize("values", [
        {},
        {"name": "arr1", "type": FIELD_TYPE.FIXED_LENGTH_ARRAY, "conversion": "%s", "data_type": DATA_TYPE.UCHAR, "array_length": 4},
        {"name": "arr2", "array_length": 0}])
    class TestArrayFieldDefinition:
        """Tests for ArrayFieldDefinition."""
        defaults = {
            "name": "",
            "type": FIELD_TYPE.UNKNOWN,
            "conversion": "",
            "data_type": DATA_TYPE.UNKNOWN,
            "array_length": 0,
        }
        def test_construct(self, values: dict):
            # Act
            field = ArrayFieldDefinition(**values)
            # Assert
            for attr, default in self.defaults.items():
                assert getattr(field, attr) == values.get(attr, default)

        def test_set_direct(self, values: dict):
            # Arrange
            field = ArrayFieldDefinition()
            # Act
            for attr, value in values.items():
                setattr(field, attr, value)
            # Assert
            for attr, default in self.defaults.items():
                assert getattr(field, attr) == values.get(attr, default)

    @pytest.mark.parametrize("values", [
        {},
        {"name": "fa1", "array_length": 4},
        {"name": "fa2", "array_length": 0, "type": FIELD_TYPE.SIMPLE, "conversion": "%s", "data_type": DATA_TYPE.UNKNOWN},
        {"name": "fa3", "array_length": 2, "fields": [
            FieldDefinition(name="x", type=FIELD_TYPE.SIMPLE, data_type=DATA_TYPE.INT),
            EnumFieldDefinition(name="kind", enum_id="42"),
        ]}])
    class TestFieldArrayFieldDefinition:
        """Tests for FieldArrayFieldDefinition."""
        defaults = {
            "name": "",
            "type": FIELD_TYPE.FIELD_ARRAY,
            "conversion": "",
            "data_type": DATA_TYPE.UNKNOWN,
            "array_length": 0,
            "fields": [],
        }
        def test_construct(self, values: dict):
            # Act
            field = FieldArrayFieldDefinition(**values)
            # Assert
            for attr, default in self.defaults.items():
                assert getattr(field, attr) == values.get(attr, default)

        def test_set_direct(self, values: dict):
            # Arrange
            field = FieldArrayFieldDefinition()
            # Act
            for attr, value in values.items():
                setattr(field, attr, value)
            # Assert
            for attr, default in self.defaults.items():
                assert getattr(field, attr) == values.get(attr, default)

    @pytest.mark.parametrize("values", [
        {},
        {"id": "1", "log_id": 42, "name": "BESTPOS", "description": "best position log", "latest_message_crc": 1234,
         "fields": {1234: [
             FieldDefinition(name="lat", type=FIELD_TYPE.SIMPLE, data_type=DATA_TYPE.DOUBLE),
             FieldDefinition(name="lon", type=FIELD_TYPE.SIMPLE, data_type=DATA_TYPE.DOUBLE),
         ]}},
        {"id": "0", "log_id": 0, "name": "", "description": "", "latest_message_crc": 0}])
    class TestMessageDefinition:
        """Tests for MessageDefinition."""
        defaults = {
            "id": "",
            "log_id": 0,
            "name": "",
            "description": "",
            "latest_message_crc": 0,
            "fields": {},
        }
        def test_construct(self, values: dict):
            # Act
            msg_def = MessageDefinition(**values)
            # Assert
            for attr, default in self.defaults.items():
                assert getattr(msg_def, attr) == values.get(attr, default)

        def test_set_direct(self, values: dict):
            # Arrange
            msg_def = MessageDefinition()
            # Act
            for attr, value in values.items():
                setattr(msg_def, attr, value)
            # Assert
            for attr, default in self.defaults.items():
                assert getattr(msg_def, attr) == values.get(attr, default)


class TestDatabaseActions:
    """Tests for the effect of preforming actions on a database."""
    class TestDatabaseLocking:
        """Tests for the database's locking mechanism.

        Any access to the database's immutable types or exposure of them to other
        C++ objects should permenantly lock the database from further mutation.
        """
        def test_lock(self, json_db: MessageDatabase):
            # Arrange
            db = MessageDatabase(message_family="OEM")
            # Act
            db.lock()

            # Assert
            assert db.is_locked
            with pytest.raises(UnsupportedException, match="locked"):
                db.append_messages(MessageDefinition())
            with pytest.raises(UnsupportedException, match="locked"):
                db.append_enumerations(EnumDefinition())
            with pytest.raises(UnsupportedException, match="locked"):
                db.remove_message(0)
            with pytest.raises(UnsupportedException, match="locked"):
                db.remove_enumeration("Datum")
            with pytest.raises(UnsupportedException, match="locked"):
                db.append_bitmasks(BitMaskDefinition())
            with pytest.raises(UnsupportedException, match="locked"):
                db.remove_bitmask("ExtendedSolutionStatus")
            with pytest.raises(UnsupportedException, match="locked"):
                db.merge(json_db)
            with pytest.raises(UnsupportedException, match="locked"):
                db.message_family = "OEM"

        def test_global_lock(self):
            assert get_builtin_database().is_locked

        def test_merge_locking(self):
            # Arrange
            old_db = MessageDatabase(message_family="OEM")
            new_db = MessageDatabase(message_family="OEM")
            # Act
            new_db.merge(old_db)
            # Assert
            assert old_db.is_locked
            assert not new_db.is_locked

        def test_fork_locking(self):
            # Arrange
            old_db = MessageDatabase()
            # Act
            new_db = old_db.fork()
            # Assert
            assert old_db.is_locked
            assert not new_db.is_locked

        def test_pytype_access_locking(self, json_db: MessageDatabase):
            # Arrange
            db = json_db.fork()
            # Act
            db.get_msg_type("BESTPOS")
            # Assert
            assert db.is_locked

        @pytest.mark.parametrize("consumer_cons", [Decoder, Parser, lambda x: FileParser("", x),
                                                   Commander, RangeDecompressor, RxConfigHandler])
        def test_consumer_locking(self, consumer_cons, json_db: MessageDatabase):
            # Arrange
            db = json_db.fork()
            # Act
            consumer_cons(db)
            # Assert
            assert db.is_locked

    def test_message_db_enums(self, json_db):
        # Act
        datum_enum = json_db.get_enum_type_by_name("Datum")
        # Assert
        assert datum_enum.WGS84 == 61
        assert datum_enum.WGS84.name == "WGS84"
        assert datum_enum.WGS84 == Datum.WGS84

    def test_message_db_bitfields(self, json_db: MessageDatabase):
        # Arrange
        bitmask_def = json_db.get_bitmask_def_by_name("ExtendedSolutionStatus")
        # Act
        by_name = json_db.get_bitfield_type_by_name("ExtendedSolutionStatus")
        by_id = json_db.get_bitfield_type_by_id(bitmask_def.id)
        # Assert
        assert by_name is ExtendedSolutionStatus
        assert by_id is ExtendedSolutionStatus
        assert issubclass(by_name, BitField)

    def test_get_bitmask_def(self, json_db: MessageDatabase):
        # Act
        by_name = json_db.get_bitmask_def_by_name("ExtendedSolutionStatus")
        by_id = json_db.get_bitmask_def_by_id(by_name.id)
        # Assert
        assert by_id == by_name
        entry = by_name.masks["pseudorange_inno_correction"]
        assert entry.bitmask == BitMask.from_range(1, 4)
        assert entry.enum_def.name == "PseudorangeInnoCorrection"
        assert by_name.masks["rtk_solution_verified"].enum_def is None

    def test_get_bitmask_def_missing(self, json_db: MessageDatabase):
        # Act / Assert
        assert json_db.get_bitmask_def_by_name("NotABitmask") is None
        assert json_db.get_bitmask_def_by_id("not_an_id") is None
        assert json_db.get_bitfield_type_by_name("NotABitmask") is None

    def test_append_bitmasks(self, json_db: MessageDatabase):
        # Arrange
        new_db = MessageDatabase(message_family="OEM")
        bitmask_def = json_db.get_bitmask_def_by_name("ExtendedSolutionStatus")

        # Act
        new_db.append_bitmasks([bitmask_def])

        # Assert
        assert new_db.get_bitmask_def_by_name("ExtendedSolutionStatus") == bitmask_def
        assert new_db.get_bitmask_def_by_id(bitmask_def.id) == bitmask_def
        new_type = new_db.get_bitfield_type_by_name("ExtendedSolutionStatus")
        assert new_type is not None
        assert new_type is not ExtendedSolutionStatus
        assert json_db.get_bitfield_type_by_name("ExtendedSolutionStatus") is ExtendedSolutionStatus

    def test_append_bitmasks_resolves_existing_message_fields(self):
        # Arrange
        db = MessageDatabase(message_family="OEM")
        db.append_messages([_status_msg_def()])
        assert db.get_msg_def("STATUSMSG").fields[0][0].bitmask_def is None

        # Act
        db.append_bitmasks([_status_bitmask_def()])

        # Assert
        assert db.get_msg_def("STATUSMSG").fields[0][0].bitmask_def == _status_bitmask_def()
        message = db.get_msg_type("STATUSMSG")(status=0xA5)
        assert isinstance(message.status, db.get_bitfield_type_by_name("Status"))
        assert message.status.low == 0x5
        assert message.status.high == 0xA

    def test_append_bitmasks_replaces_by_name(self):
        # Arrange
        db = MessageDatabase(message_family="OEM")
        db.append_bitmasks([_status_bitmask_def()])
        replacement = _status_bitmask_def({"all": BitMaskEntry(BitMask.from_range(0, 8))})

        # Act
        db.append_bitmasks([replacement])

        # Assert
        assert db.get_bitmask_def_by_name("Status") == replacement
        status = db.get_bitfield_type_by_name("Status")(0xA5)
        assert status.all == 0xA5
        with pytest.raises(AttributeError):
            status.low

    def test_remove_bitmask(self):
        # Arrange
        db = MessageDatabase(message_family="OEM")
        db.append_messages([_status_msg_def()])
        db.append_bitmasks([_status_bitmask_def()])

        # Act
        db.remove_bitmask("Status")

        # Assert
        assert db.get_bitmask_def_by_name("Status") is None
        assert db.get_bitmask_def_by_id("status_id") is None
        assert db.get_msg_def("STATUSMSG").fields[0][0].bitmask_def is None
        assert db.get_bitfield_type_by_name("Status") is None
        message = db.get_msg_type("STATUSMSG")(status=0xA5)
        assert type(message.status) is int
        assert message.status == 0xA5

    def test_merge_bitmasks(self):
        # Arrange
        source_db = MessageDatabase(message_family="OEM")
        source_db.append_messages([_status_msg_def()])
        source_db.append_bitmasks([_status_bitmask_def()])
        new_db = MessageDatabase(message_family="OEM")

        # Act
        new_db.merge(source_db)

        # Assert
        assert new_db.get_bitmask_def_by_name("Status") == _status_bitmask_def()
        new_type = new_db.get_bitfield_type_by_name("Status")
        assert new_type is not None
        assert new_type is not source_db.get_bitfield_type_by_name("Status")
        message = new_db.get_msg_type("STATUSMSG")(status=0xA5)
        assert isinstance(message.status, new_type)

    def test_append_messages(self, json_db: MessageDatabase):
        # Arrange
        new_db = MessageDatabase(message_family="OEM")
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

    def test_remove_message(self, json_db: MessageDatabase):
        # Arrange
        new_db = MessageDatabase(message_family="OEM")
        bestpos_id = 42
        bestpos_def = json_db.get_msg_def(bestpos_id)
        bestpos_type = json_db.get_msg_type("BESTPOS")
        range_id = 43
        range_def = json_db.get_msg_def(range_id)
        new_db.append_messages([bestpos_def, range_def])

        # Act
        new_db.remove_message(bestpos_id)

        # Assert
        assert json_db.get_msg_def(bestpos_id) == bestpos_def
        assert json_db.get_msg_type("BESTPOS") is bestpos_type

        assert new_db.get_msg_def(bestpos_id) is None
        assert new_db.get_msg_type("BESTPOS") is None
        assert new_db.get_msg_def(range_id) == range_def
        assert new_db.get_msg_type("RANGE") is not None

    def test_merge(self, json_db: MessageDatabase):
        """Tests that one databases messages can be merged into another."""
        # Arrange
        oem_minus_bestpos_db = json_db.fork()
        new_db_with_bestpos = MessageDatabase(message_family="OEM")
        bestpos_name = "BESTPOS"
        bestpos_msg_def = oem_minus_bestpos_db.get_msg_def(bestpos_name)
        new_db_with_bestpos.append_messages([bestpos_msg_def])
        other_msg_name = "RANGE"
        other_msg_def = oem_minus_bestpos_db.get_msg_def(other_msg_name)
        oem_minus_bestpos_db.remove_message(bestpos_msg_def.log_id)
        other_msg_type = oem_minus_bestpos_db.get_msg_type(other_msg_name)

        # Act
        new_db_with_bestpos.merge(oem_minus_bestpos_db)

        # Assert
        assert new_db_with_bestpos.get_msg_def(bestpos_name) == bestpos_msg_def
        assert new_db_with_bestpos.get_msg_type(bestpos_name) is not None
        assert new_db_with_bestpos.get_msg_def(other_msg_name) == other_msg_def
        assert new_db_with_bestpos.get_msg_type(other_msg_name) is not None
        assert new_db_with_bestpos.get_msg_type(other_msg_name) != other_msg_type

        # The source database must be unaffected by the merge.
        assert oem_minus_bestpos_db.get_msg_def(bestpos_name) is None
        assert oem_minus_bestpos_db.get_msg_type(bestpos_name) is None
        assert oem_minus_bestpos_db.get_msg_def(other_msg_name) == other_msg_def
        assert oem_minus_bestpos_db.get_msg_type(other_msg_name) is other_msg_type

    def test_fork(self, json_db: MessageDatabase):
        """Tests that a forked database exposes the same messages and types as the original."""
        # Arrange
        bestpos_name = "BESTPOS"
        bestpos_def = json_db.get_msg_def(bestpos_name)
        bestpos_type = json_db.get_msg_type(bestpos_name)
        range_name = "RANGE"
        range_def = json_db.get_msg_def(range_name)
        range_type = json_db.get_msg_type(range_name)

        # Act
        forked_db = json_db.fork()

        # Assert
        assert forked_db is not json_db
        assert forked_db.get_msg_def(bestpos_name) == bestpos_def
        assert forked_db.get_msg_type(bestpos_name) is bestpos_type
        assert forked_db.get_msg_def(range_name) == range_def
        assert forked_db.get_msg_type(range_name) is range_type
        assert forked_db.get_bitfield_type_by_name("ExtendedSolutionStatus") is ExtendedSolutionStatus

        assert json_db.get_msg_def(bestpos_name) == bestpos_def
        assert json_db.get_msg_type(bestpos_name) is bestpos_type
        assert json_db.get_msg_def(range_name) == range_def
        assert json_db.get_msg_type(range_name) is range_type

    def test_clone_deprecated(self, json_db: MessageDatabase, caplog):
        """Tests that the deprecated clone() alias warns but still behaves like fork()."""
        # Act
        with caplog.at_level(logging.WARNING, logger="novatel_edie.deprecation_warning"):
            cloned_db = json_db.clone()

        # Assert: clone() behaves like fork() (independent copy, source locked).
        assert cloned_db is not json_db
        assert json_db.is_locked
        assert not cloned_db.is_locked

        # Assert: a deprecation warning directing the user to fork() was logged.
        deprecation_records = [
            rec for rec in caplog.records if rec.name == "novatel_edie.deprecation_warning"
        ]
        assert len(deprecation_records) == 1
        assert deprecation_records[0].levelno == logging.WARNING
        assert "fork" in deprecation_records[0].message

    def test_append_message_recalculate_alignment(self, json_db: MessageDatabase):
        # Arrange
        oem_db = json_db.fork()
        generic_db = MessageDatabase(message_family="")
        test_msg = MessageDefinition(
            id='test_msg', log_id=0, name='test_msg', latest_message_crc=0,
            fields={0: [
                FieldDefinition(name='short', type=FIELD_TYPE.SIMPLE, data_type=DATA_TYPE.SHORT),
                FieldDefinition(name='int', type=FIELD_TYPE.SIMPLE, data_type=DATA_TYPE.INT)
            ]}
        )

        # Act
        oem_db.append_messages([test_msg])
        generic_db.append_messages([test_msg])

        # Assert
        retrieved_msg_oem = oem_db.get_msg_def('test_msg')
        assert retrieved_msg_oem is not None
        assert len(retrieved_msg_oem.fields[0]) == 2
        assert retrieved_msg_oem.fields[0][1].index == 4 # OEM alignment

        retrieved_msg_generic = generic_db.get_msg_def('test_msg')
        assert retrieved_msg_generic is not None
        assert len(retrieved_msg_generic.fields[0]) == 2
        assert retrieved_msg_generic.fields[0][1].index == 2 # No alignment
        
    def test_set_message_family_recalculate_alignment(self, json_db: MessageDatabase):
        # Arrange
        db = MessageDatabase()
        test_msg = MessageDefinition(
            id='test_msg', log_id=0, name='test_msg', latest_message_crc=0,
            fields={0: [
                FieldDefinition(name='short', type=FIELD_TYPE.SIMPLE, data_type=DATA_TYPE.SHORT),
                FieldDefinition(name='int', type=FIELD_TYPE.SIMPLE, data_type=DATA_TYPE.INT)
            ]}
        )

        # Act
        db.append_messages([test_msg])
        db.message_family = "OEM"

        # Assert
        retrieved_msg_oem = db.get_msg_def('test_msg')
        assert retrieved_msg_oem is not None
        assert test_msg.fields[0][1].index == 2 # Original message definition remains unchanged
        assert len(retrieved_msg_oem.fields[0]) == 2
        assert retrieved_msg_oem.fields[0][1].index == 4 # OEM alignment
        
    def test_merge_recalculate_alignment(self, json_db: MessageDatabase):
        # Arrange
        oem_db = json_db.fork()
        generic_db = MessageDatabase(message_family="")
        test_msg = MessageDefinition(
            id='test_msg', log_id=0, name='test_msg', latest_message_crc=0,
            fields={0: [
                FieldDefinition(name='short', type=FIELD_TYPE.SIMPLE, data_type=DATA_TYPE.SHORT),
                FieldDefinition(name='int', type=FIELD_TYPE.SIMPLE, data_type=DATA_TYPE.INT)
            ]}
        )
        generic_db.append_messages([test_msg])

        # Act
        oem_db.merge(generic_db)

        # Assert
        retrieved_msg_oem = oem_db.get_msg_def('test_msg')
        assert retrieved_msg_oem is not None
        assert test_msg.fields[0][1].index == 2 # Original message definition remains unchanged
        assert len(retrieved_msg_oem.fields[0]) == 2
        assert retrieved_msg_oem.fields[0][1].index == 4 # OEM alignment
