"""Tests for the in-Python message construction / mutation API, exercised
against synthetic MessageDefinitions built in-test (no reliance on the
shipped OEM database for the message types under test).

Scope:
 - StandardArray (ArrayFieldDefinition): valid setters + validation
 - FieldArray (FieldArrayFieldDefinition): setattr rejection
 - General validation (unknown kwarg, unknown setattr, type mismatch)
"""

from typing import Callable

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
    db.message_family = 'OEM'
    msg_def = MessageDefinition(
        id=name, log_id=0, name=name, latest_message_crc=0,
        fields={0: list(fields)},
    )
    db.append_messages([msg_def])
    return db.get_msg_type(name)




class TestStandardArray:
    """Tests for ArrayFieldDefinition-backed arrays (FIXED_LENGTH and VARIABLE_LENGTH)."""
    @pytest.fixture(scope='class')
    def standard_array_message_type(self):
        """Message with a single variable-length UCHAR array field (auto-generates payload_length)."""
        return make_message_type('standard_array_msg', [
            ArrayFieldDefinition(
                'fixed_str',
                type=FIELD_TYPE.FIXED_LENGTH_ARRAY,
                conversion=r'%s',
                data_type=DATA_TYPE.UCHAR,
                array_length=64,
            ),
            ArrayFieldDefinition(
                'var_str',
                type=FIELD_TYPE.VARIABLE_LENGTH_ARRAY,
                conversion=r'%s',
                data_type=DATA_TYPE.UCHAR,
                array_length=64,
            ),
            ArrayFieldDefinition(
                'fixed_arr',
                type=FIELD_TYPE.FIXED_LENGTH_ARRAY,
                conversion=r'%u',
                data_type=DATA_TYPE.USHORT,
                array_length=64,
            ),
            ArrayFieldDefinition(
                'var_arr',
                type=FIELD_TYPE.VARIABLE_LENGTH_ARRAY,
                conversion=r'%u',
                data_type=DATA_TYPE.UINT,
                array_length=64,
            ),
        ])

    @pytest.mark.parametrize('value', [
        pytest.param(b'my str', id='string'),
        pytest.param(b'AB12', id='alphanumeric'),
        pytest.param(b'a' * 64, id='max-len'),
        pytest.param(bytes(range(1, 65)), id='raw-bytes'),
    ])
    @pytest.mark.parametrize('input_type', [
        pytest.param(lambda x: x, id='bytes'),
        pytest.param(lambda x: x.decode('ascii'), id='str'),
        pytest.param(list, id='list'),
    ])
    @pytest.mark.parametrize('field', ['fixed_str', 'var_str'])
    class TestStringArraySetters:
        """Set %s-conversion arrays; in Python they're just strings, so fixed and variable behave the same."""
        def test_setter(self, standard_array_message_type: type, value: bytes, field: str, input_type: Callable):
            # Arrange
            m = standard_array_message_type()
            set_value = input_type(value)
            exp_value = value.decode('ascii')
            # Act
            setattr(m, field, set_value)
            # Assert
            assert getattr(m, field) == exp_value

        def test_constructor(self, standard_array_message_type: type, value: bytes, field: str, input_type: Callable):
            # Arrange
            set_value = input_type(value)
            exp_value = value.decode('ascii')
            # Act
            m = standard_array_message_type(**{field: set_value})
            # Assert
            assert getattr(m, field) == exp_value

    @pytest.mark.parametrize('value', [
        pytest.param([0, 1, 2, 3], id='small-ints'),
        pytest.param(list(range(64)), id='max-len'),
        pytest.param([0xDE, 0xAD, 0xBE, 0xEF], id='raw-bytes'),
    ])
    class TestFixedRegularArraySetters:
        """Set the %u fixed-length array; shorter values are accepted and the return is zero-padded to the array length."""
        field = 'fixed_arr'
        array_length = 64

        def test_setter(self, standard_array_message_type: type, value: list):
            # Arrange
            m = standard_array_message_type()
            exp_value = value + [0] * (self.array_length - len(value))
            # Act
            setattr(m, self.field, value)
            # Assert
            assert getattr(m, self.field) == exp_value

        def test_constructor(self, standard_array_message_type: type, value: list):
            # Arrange
            exp_value = value + [0] * (self.array_length - len(value))
            # Act
            m = standard_array_message_type(**{self.field: value})
            # Assert
            assert getattr(m, self.field) == exp_value

    @pytest.mark.parametrize('value', [
        pytest.param([0, 1, 2, 3], id='small-ints'),
        pytest.param(list(range(64)), id='max-len'),
        pytest.param([0xDE, 0xAD, 0xBE, 0xEF], id='raw-bytes'),
    ])
    class TestVariableRegularArraySetters:
        """Set the %u variable-length array; value may be shorter than the array."""
        field = 'var_arr'

        def test_setter(self, standard_array_message_type: type, value: list):
            # Arrange
            m = standard_array_message_type()
            # Act
            setattr(m, self.field, value)
            # Assert
            assert getattr(m, self.field) == value

        def test_constructor(self, standard_array_message_type: type, value: list):
            # Act
            m = standard_array_message_type(**{self.field: value})
            # Assert
            assert getattr(m, self.field) == value

    class TestValidation:
        """Validation rejections specific to standard arrays."""
        @pytest.mark.parametrize('length_field', ['var_str_length', 'var_arr_length'])
        def test_length_field_write_rejected(self, standard_array_message_type: type, length_field: str):
            m = standard_array_message_type()
            with pytest.raises(AttributeError, match='Length cannot be set directly'):
                setattr(m, length_field, 5)

        @pytest.mark.parametrize('value', [b'x' * 65, 'a' * 65, [0] * 65])
        def test_oversize_value_rejected(self, standard_array_message_type: type, value):
            m = standard_array_message_type()
            with pytest.raises(ValueError, match='Value exceeds maximum array size'):
                m.fixed_str = value

        @pytest.mark.parametrize('field', ['fixed_arr', 'var_arr'])
        @pytest.mark.parametrize('value', [
            pytest.param(b'abc', id='bytes'),
            pytest.param('abc', id='str'),
        ])
        def test_string_value_rejected_for_regular_field(self, standard_array_message_type: type, field: str, value):
            m = standard_array_message_type()
            with pytest.raises(TypeError):
                setattr(m, field, value)

        @pytest.mark.parametrize('field', ['fixed_arr', 'var_arr'])
        @pytest.mark.parametrize('value', [
            pytest.param(b'abc', id='bytes'),
            pytest.param('abc', id='str'),
        ])
        def test_string_value_rejected_for_regular_field_constructor(self, standard_array_message_type: type, field: str, value):
            with pytest.raises(TypeError):
                standard_array_message_type(**{field: value})


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
