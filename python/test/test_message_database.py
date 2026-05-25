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
from novatel_edie import MessageDefinition, EnumFieldDefinition, EnumDefinition, EnumDataType
from novatel_edie import FieldDefinition, ArrayFieldDefinition, FieldArrayFieldDefinition, FIELD_TYPE, DATA_TYPE
from novatel_edie.oem import Header

class TestDatabaseDefinitionObjects:
    """Tests that verify the interface for definition objects."""

    @pytest.mark.parametrize("values", [
        {},
        {"value": 2, "name": "val", "description": "a value of 2"},
        {"value": 0, "name": "error", "description": "an error msg"}])
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

    class TestFieldDefinition:
        """Tests for FieldDefinition (BaseField)."""

        @pytest.mark.parametrize("values", [
            {},
            {"name": "field1", "type": FIELD_TYPE.SIMPLE, "conversion": "%d", "data_type": DATA_TYPE.INT},
            {"name": "field2", "type": FIELD_TYPE.SIMPLE, "conversion": "%.3f", "data_type": DATA_TYPE.DOUBLE}])
        class TestValues:
            """Tests that values are set correctly."""
            defaults = {
                "name": "",
                "type": FIELD_TYPE.UNKNOWN,
                "conversion": "",
                "data_type": DATA_TYPE.UNKNOWN
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
        {"name": "field1", "type": FIELD_TYPE.SIMPLE, "conversion": "%d", "data_type": DATA_TYPE.ULONG, "enum_id": "42", "enumerators": [EnumDataType(1, "A", "first"), EnumDataType(2, "B", "second")]},
        {"name": "field2"}])
    class TestEnumFieldDefinition:
        """Tests for EnumFieldDefinition."""
        defaults = {
            "name": "",
            "type": FIELD_TYPE.ENUM,
            "conversion": "",
            "data_type": DATA_TYPE.UNKNOWN,
            "enum_id": "",
            "enumerators": [],
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
            EnumFieldDefinition(name="kind", enum_id="42", enumerators=[EnumDataType(1, "A", "first")]),
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
        {"id": "1", "log_id": 42, "name": "BESTPOS", "description": "best position log", "latest_message_crc": 1234},
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
