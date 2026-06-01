"""Tests for the in-Python message construction / mutation API on standard
arrays, exercised against synthetic MessageDefinitions built in-test (no
reliance on the shipped OEM database for the message types under test).
"""

from dataclasses import dataclass
from typing import Callable, List

import pytest

from novatel_edie import (
    MessageDefinition,
    ArrayFieldDefinition,
    FIELD_TYPE,
    DATA_TYPE,
    MessageDatabase,
)


ARRAY_LENGTH = 64
OVERSIZE = ARRAY_LENGTH + 1


@dataclass(frozen=True)
class ArrayFieldSpec:
    """A standard-array field's name, FIELD_TYPE, printf-style conversion, element DATA_TYPE, and capacity."""
    name: str
    field_type: FIELD_TYPE
    conversion: str
    data_type: DATA_TYPE
    array_length: int = ARRAY_LENGTH


# --------------------------------------------------------------------------- #
# Field catalogues, grouped by how their values round-trip through the API.
# --------------------------------------------------------------------------- #

# %s arrays behave as strings in Python; fixed and variable are identical.
STRING_FIELDS = [
    ArrayFieldSpec('fixed_str', FIELD_TYPE.FIXED_LENGTH_ARRAY,    r'%s', DATA_TYPE.UCHAR),
    ArrayFieldSpec('var_str',   FIELD_TYPE.VARIABLE_LENGTH_ARRAY, r'%s', DATA_TYPE.UCHAR),
]

# %P UCHAR buffer: accepts str/bytes/list, returns a list of ints.
PRINTABLE_FIELDS = [
    ArrayFieldSpec('printable_buffer', FIELD_TYPE.VARIABLE_LENGTH_ARRAY, r'%P', DATA_TYPE.UCHAR),
]

# Numeric fixed-length arrays: accept a list of ints, return zero-padded to array_length.
FIXED_NUMERIC_FIELDS = [
    ArrayFieldSpec('char_fixed_arr', FIELD_TYPE.FIXED_LENGTH_ARRAY, r'%u',  DATA_TYPE.UCHAR),
    ArrayFieldSpec('fixed_arr',      FIELD_TYPE.FIXED_LENGTH_ARRAY, r'%hu', DATA_TYPE.USHORT),
]

# Numeric variable-length arrays: accept a list of ints, return the list as given.
VARIABLE_NUMERIC_FIELDS = [
    ArrayFieldSpec('var_arr', FIELD_TYPE.VARIABLE_LENGTH_ARRAY, r'%d', DATA_TYPE.UINT),
]

NUMERIC_FIELDS = FIXED_NUMERIC_FIELDS + VARIABLE_NUMERIC_FIELDS
ALL_FIELDS = STRING_FIELDS + PRINTABLE_FIELDS + FIXED_NUMERIC_FIELDS + VARIABLE_NUMERIC_FIELDS
# Variable-length fields auto-generate a `<name>_length` companion field.
VARIABLE_FIELDS = [s for s in ALL_FIELDS if s.field_type == FIELD_TYPE.VARIABLE_LENGTH_ARRAY]


def _names(specs: List[ArrayFieldSpec]) -> List[str]:
    """Field names of the given specs, for use as parametrize values/ids."""
    return [spec.name for spec in specs]


def _spec_params(specs: List[ArrayFieldSpec]) -> list:
    """Build (spec,) params, one per spec, ided by field name."""
    return [pytest.param(spec, id=spec.name) for spec in specs]


# --------------------------------------------------------------------------- #
# Input representations. Every test value is authored as a list of ints; these
# convert that list into the bytes/str/list forms a field accepts.
# --------------------------------------------------------------------------- #

def _to_bytes(ints: List[int]) -> bytes:
    return bytes(ints)


def _to_str(ints: List[int]) -> str:
    return bytes(ints).decode('ascii')


def _to_list(ints: List[int]) -> List[int]:
    return list(ints)


# Converters for every representation a string-like array accepts.
INPUT_CONVERTERS = [
    pytest.param(_to_bytes, id='bytes'),
    pytest.param(_to_str, id='str'),
    pytest.param(_to_list, id='list'),
]

# Converters for the representations a numeric array rejects (a list is valid input, so it is excluded).
STR_BYTES_CONVERTERS = [
    pytest.param(_to_bytes, id='bytes'),
    pytest.param(_to_str, id='str'),
]


# --------------------------------------------------------------------------- #
# Value catalogues — all lists of ints; string content is sourced from b'...'.
# --------------------------------------------------------------------------- #

STRING_VALUES = [
    pytest.param([], id='empty'),
    pytest.param(list(b'my str'), id='string'),
    pytest.param(list(b'AB12'), id='alphanumeric'),
    pytest.param(list(b'a' * ARRAY_LENGTH), id='max-len'),
    pytest.param(list(range(1, ARRAY_LENGTH + 1)), id='raw-bytes'),
]

FIXED_NUMERIC_VALUES = [
    pytest.param([], id='empty'),
    pytest.param([0, 1, 2, 3], id='small-ints'),
    pytest.param(list(range(ARRAY_LENGTH)), id='max-len'),
    pytest.param([0xDE, 0xAD, 0xBE, 0xEF], id='raw-bytes'),
]

VARIABLE_NUMERIC_VALUES = [
    pytest.param([0, 1, 2, 3], id='small-ints'),
    pytest.param(list(range(ARRAY_LENGTH)), id='max-len'),
    pytest.param([0xDE, 0xAD, 0xBE, 0xEF], id='raw-bytes'),
]


class TestStandardArray:
    """Tests for ArrayFieldDefinition-backed arrays (FIXED_LENGTH and VARIABLE_LENGTH)."""

    @pytest.fixture(scope='class')
    def message_type(self) -> type:
        """Message exposing one array field per spec in ALL_FIELDS."""
        fields = [
            ArrayFieldDefinition(
                spec.name, type=spec.field_type, conversion=spec.conversion,
                data_type=spec.data_type, array_length=spec.array_length,
            )
            for spec in ALL_FIELDS
        ]
        msg_def = MessageDefinition(
            id='standard_array_msg', log_id=0, name='standard_array_msg', latest_message_crc=0,
            fields={0: fields},
        )
        db = MessageDatabase(message_family='OEM')
        db.append_messages([msg_def])
        return db.get_msg_type('standard_array_msg')

    @pytest.mark.parametrize('value', STRING_VALUES)
    @pytest.mark.parametrize('converter', INPUT_CONVERTERS)
    @pytest.mark.parametrize('field', _names(STRING_FIELDS))
    class TestStringArraySetters:
        """Set %s-conversion arrays; in Python they're just strings, so fixed and variable behave the same."""

        def test_setter(self, message_type: type, value: List[int], field: str, converter: Callable):
            # Arrange
            m = message_type()
            # Act
            setattr(m, field, converter(value))
            # Assert
            assert getattr(m, field) == _to_str(value)

        def test_constructor(self, message_type: type, value: List[int], field: str, converter: Callable):
            # Act
            m = message_type(**{field: converter(value)})
            # Assert
            assert getattr(m, field) == _to_str(value)

    @pytest.mark.parametrize('value', STRING_VALUES)
    @pytest.mark.parametrize('converter', INPUT_CONVERTERS)
    @pytest.mark.parametrize('field', _names(PRINTABLE_FIELDS))
    class TestPrintableByteArraySetters:
        """Set %P UCHAR arrays; accept str/bytes/list, return a list of ints."""

        def test_setter(self, message_type: type, value: List[int], field: str, converter: Callable):
            # Arrange
            m = message_type()
            # Act
            setattr(m, field, converter(value))
            # Assert
            assert getattr(m, field) == value

        def test_constructor(self, message_type: type, value: List[int], field: str, converter: Callable):
            # Act
            m = message_type(**{field: converter(value)})
            # Assert
            assert getattr(m, field) == value

    @pytest.mark.parametrize('value', FIXED_NUMERIC_VALUES)
    @pytest.mark.parametrize('spec', _spec_params(FIXED_NUMERIC_FIELDS))
    class TestFixedNumericArraySetters:
        """Set numeric fixed-length arrays; shorter values are accepted and the return is zero-padded to array_length."""

        def test_setter(self, message_type: type, spec: ArrayFieldSpec, value: list):
            # Arrange
            m = message_type()
            exp_value = value + [0] * (spec.array_length - len(value))
            # Act
            setattr(m, spec.name, value)
            # Assert
            assert getattr(m, spec.name) == exp_value

        def test_constructor(self, message_type: type, spec: ArrayFieldSpec, value: list):
            # Arrange
            exp_value = value + [0] * (spec.array_length - len(value))
            # Act
            m = message_type(**{spec.name: value})
            # Assert
            assert getattr(m, spec.name) == exp_value

    @pytest.mark.parametrize('spec', _spec_params(FIXED_NUMERIC_FIELDS))
    def test_fixed_array_default_construction_is_zero_padded(self, message_type: type, spec: ArrayFieldSpec):
        # Act
        m = message_type()
        # Assert
        assert getattr(m, spec.name) == [0] * spec.array_length

    @pytest.mark.parametrize('value', VARIABLE_NUMERIC_VALUES)
    @pytest.mark.parametrize('spec', _spec_params(VARIABLE_NUMERIC_FIELDS))
    class TestVariableNumericArraySetters:
        """Set numeric variable-length arrays; the value may be shorter than array_length and is returned as given."""

        def test_setter(self, message_type: type, spec: ArrayFieldSpec, value: list):
            # Arrange
            m = message_type()
            # Act
            setattr(m, spec.name, value)
            # Assert
            assert getattr(m, spec.name) == value

        def test_constructor(self, message_type: type, spec: ArrayFieldSpec, value: list):
            # Act
            m = message_type(**{spec.name: value})
            # Assert
            assert getattr(m, spec.name) == value

    class TestValidation:
        """Validation rejections specific to standard arrays."""

        @pytest.mark.parametrize('length_field', [f'{spec.name}_length' for spec in VARIABLE_FIELDS])
        def test_length_field_write_rejected(self, message_type: type, length_field: str):
            m = message_type()
            with pytest.raises(AttributeError, match='Length cannot be set directly'):
                setattr(m, length_field, 5)

        @pytest.mark.parametrize('converter', INPUT_CONVERTERS)
        @pytest.mark.parametrize('field', _names(STRING_FIELDS))
        def test_oversize_value_rejected(self, message_type: type, field: str, converter: Callable):
            m = message_type()
            with pytest.raises(ValueError, match='Value exceeds maximum array size'):
                setattr(m, field, converter(list(b'a' * OVERSIZE)))

        @pytest.mark.parametrize('converter', STR_BYTES_CONVERTERS)
        @pytest.mark.parametrize('field', _names(NUMERIC_FIELDS))
        class TestStringRejectedForNumericField:
            """Numeric arrays reject str/bytes input (a list is valid), via both setter and constructor."""

            def test_setter(self, message_type: type, field: str, converter: Callable):
                m = message_type()
                with pytest.raises(TypeError):
                    setattr(m, field, converter(list(b'abc')))

            def test_constructor(self, message_type: type, field: str, converter: Callable):
                with pytest.raises(TypeError):
                    message_type(**{field: converter(list(b'abc'))})
