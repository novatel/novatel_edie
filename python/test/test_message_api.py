"""Tests for the in-Python message construction / mutation API, exercised
against synthetic MessageDefinitions built in-test (no reliance on the
shipped OEM database for the message types under test).

Scope:
 - StandardArray (ArrayFieldDefinition): valid setters + validation
 - FieldArray (FieldArrayFieldDefinition): setattr rejection
 - General validation (unknown kwarg, unknown setattr, type mismatch)
"""

import pytest

from novatel_edie import (
    MessageDefinition,
    FieldDefinition,
    ArrayFieldDefinition,
    FieldArrayFieldDefinition,
    FIELD_TYPE,
    DATA_TYPE,
    MessageDatabase,
)


def make_message_type(name: str, fields: list) -> type:
    """Build a fresh MessageDatabase holding one MessageDefinition and return its message type."""
    db = MessageDatabase()
    msg_def = MessageDefinition(
        id=name, log_id=0, name=name, latest_message_crc=0,
        fields={0: list(fields)},
    )
    db.append_messages([msg_def])
    return db.get_msg_type(name)


@pytest.fixture(scope='module')
def standard_array_message_type():
    """Message with a single variable-length UCHAR array field (auto-generates payload_length)."""
    return make_message_type('standard_array_msg', [
        ArrayFieldDefinition(
            'payload',
            type=FIELD_TYPE.VARIABLE_LENGTH_ARRAY,
            conversion=r'%s',
            data_type=DATA_TYPE.UCHAR,
            array_length=64,
        ),
    ])


class TestStandardArray:
    """Tests for ArrayFieldDefinition-backed arrays (FIXED_LENGTH and VARIABLE_LENGTH)."""

    @pytest.mark.parametrize('value', [
        b'my str', b'AB12'
    ])
    class TestSetters:
        """Set a standard array via bytes, str, and list inputs."""

        def test_bytes_setter(self, standard_array_message_type: type, value: bytes):
            # Arrange
            m = standard_array_message_type()
            set_value = value
            exp_value = value.decode('ascii')
            # Act
            m.payload = set_value
            # Assert
            assert m.payload == exp_value

        def test_string_setter(self, standard_array_message_type: type, value: bytes):
            # Arrange
            m = standard_array_message_type()
            set_value = value.decode('ascii')
            exp_value = value.decode('ascii')
            # Act
            m.payload = set_value
            # Assert
            assert m.payload == exp_value

        def test_list_setter(self, standard_array_message_type: type, value: bytes):
            # Arrange
            m = standard_array_message_type()
            set_value = list(value)
            exp_value = value.decode('ascii')
            # Act
            m.payload = set_value
            # Assert
            assert m.payload == exp_value

    class TestValidation:
        """Validation rejections specific to standard arrays."""

        def test_length_field_write_rejected(self, standard_array_message_type: type):
            m = standard_array_message_type()
            with pytest.raises(AttributeError, match='Length cannot be set directly'):
                m.payload_length = 5

        def test_oversize_bytes_rejected(self, standard_array_message_type: type):
            m = standard_array_message_type()
            with pytest.raises(ValueError, match='does not fit within fixed array size'):
                m.payload = b'x' * 65  # array_length is 64

        def test_oversize_list_rejected(self, standard_array_message_type: type):
            m = standard_array_message_type()
            with pytest.raises(ValueError, match='too many elements for fixed array'):
                m.payload = [0] * 65  # array_length is 64


class TestFieldArray:
    """Tests for FieldArrayFieldDefinition-backed arrays of nested records."""

    @pytest.fixture(scope='class')
    def message_type(self):
        """Message with a FieldArray of two-INT nested records."""
        return make_message_type('field_array_msg', [
            FieldArrayFieldDefinition(
                name='nested',
                array_length=2,
                fields=[
                    FieldDefinition(name='x', type=FIELD_TYPE.SIMPLE, data_type=DATA_TYPE.INT),
                    FieldDefinition(name='y', type=FIELD_TYPE.SIMPLE, data_type=DATA_TYPE.INT),
                ],
            ),
        ])

    def test_field_array_setattr_rejected(self, message_type: type):
        m = message_type()
        with pytest.raises(TypeError, match='FieldArray'):
            m.nested = [1, 2, 3]


class TestGeneralValidation:
    """Message-shape-agnostic rejections on a synthetic message definition."""

    @pytest.fixture(scope='class')
    def message_type(self):
        """Message with a single INT scalar field."""
        return make_message_type('general_validation_msg', [
            FieldDefinition(
                name='value',
                type=FIELD_TYPE.SIMPLE,
                conversion=r'%d',
                data_type=DATA_TYPE.INT,
            ),
        ])

    def test_unknown_kwarg_rejected(self, message_type: type):
        with pytest.raises(AttributeError, match='this_is_not_a_field'):
            message_type(this_is_not_a_field=1)

    def test_unknown_setattr_rejected(self, message_type: type):
        m = message_type()
        with pytest.raises(AttributeError, match='does_not_exist'):
            m.does_not_exist = 1

    def test_setattr_type_mismatch_raises(self, message_type: type):
        m = message_type()
        with pytest.raises(TypeError):
            m.value = 'not a number'
