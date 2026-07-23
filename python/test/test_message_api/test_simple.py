"""Tests for the in-Python message API on scalar fields, exercised against
synthetic MessageDefinitions built in-test (no reliance on the shipped OEM
database for the message types under test).
"""

import math
import sys
from dataclasses import dataclass
from typing import List

import pytest

from novatel_edie import (
    MessageDefinition,
    FieldDefinition,
    FIELD_TYPE,
    DATA_TYPE,
    MessageDatabase,
)


@dataclass(frozen=True)
class ScalarFieldSpec:
    """A SIMPLE scalar field's name, DATA_TYPE, and printf-style conversion string."""
    name: str
    data_type: DATA_TYPE
    conversion: str


@dataclass(frozen=True)
class IntFieldSpec(ScalarFieldSpec):
    """A SIMPLE integer field, with the inclusive [min, max] range of its underlying C++ type."""
    min: int
    max: int


@dataclass(frozen=True)
class FloatFieldSpec(ScalarFieldSpec):
    """A SIMPLE float field, with hand-picked round-trip values (precision matters more than range)
    and the finite extremes of its underlying C++ type."""
    values: tuple
    finite_max: float
    finite_min: float


# --------------------------------------------------------------------------- #
# Field catalogues, grouped by how the values round-trip through the API.
# --------------------------------------------------------------------------- #

SIGNED_INT_FIELDS = [
    IntFieldSpec('char_val',     DATA_TYPE.CHAR,     r'%c',   min=-(2**7),  max=2**7 - 1),
    IntFieldSpec('short_val',    DATA_TYPE.SHORT,    r'%hd',  min=-(2**15), max=2**15 - 1),
    IntFieldSpec('int_val',      DATA_TYPE.INT,      r'%d',   min=-(2**31), max=2**31 - 1),
    IntFieldSpec('long_val',     DATA_TYPE.LONG,     r'%ld',  min=-(2**31), max=2**31 - 1),
    IntFieldSpec('longlong_val', DATA_TYPE.LONGLONG, r'%lld', min=-(2**63), max=2**63 - 1),
]

UNSIGNED_INT_FIELDS = [
    IntFieldSpec('uchar_val',     DATA_TYPE.UCHAR,     r'%uc',  min=0, max=2**8 - 1),
    IntFieldSpec('ushort_val',    DATA_TYPE.USHORT,    r'%hu',  min=0, max=2**16 - 1),
    IntFieldSpec('uint_val',      DATA_TYPE.UINT,      r'%u',   min=0, max=2**32 - 1),
    IntFieldSpec('ulong_val',     DATA_TYPE.ULONG,     r'%lu',  min=0, max=2**32 - 1),
    IntFieldSpec('ulonglong_val', DATA_TYPE.ULONGLONG, r'%llu', min=0, max=2**64 - 1),
]

INT_FIELDS = SIGNED_INT_FIELDS + UNSIGNED_INT_FIELDS

# Largest finite IEEE-754 single-precision value; beyond this a float32 saturates to +inf.
FLOAT32_MAX = 3.4028234663852886e38

FLOAT_FIELDS = [
    FloatFieldSpec('float_val',  DATA_TYPE.FLOAT,  r'%f',  values=(0.0, 1.5, -2.25, 0.5, 256.0, 0.1, 3.14),
                   finite_max=FLOAT32_MAX, finite_min=-FLOAT32_MAX),
    FloatFieldSpec('double_val', DATA_TYPE.DOUBLE, r'%lf', values=(0.0, 1.5, -2.5, 3.141592653589793, 1e10, -1234.5),
                   finite_max=sys.float_info.max, finite_min=-sys.float_info.max),
]

BOOL_FIELD = ScalarFieldSpec('bool_val', DATA_TYPE.BOOL, r'%d')

# Scalar DATA_TYPEs that pass FieldContainer validation but are rejected by the
# attribute setter (get_simple_attribute has no case for them).
UNSETTABLE_FIELDS = [
    ScalarFieldSpec('hexbyte_val',   DATA_TYPE.HEXBYTE,     r'%Z'),
    ScalarFieldSpec('satellite_val', DATA_TYPE.SATELLITEID, r'%id'),
]

ALL_SIMPLE_FIELDS = INT_FIELDS + FLOAT_FIELDS + [BOOL_FIELD] + UNSETTABLE_FIELDS


def _names(specs: List[ScalarFieldSpec]) -> List[str]:
    """Field names of the given specs, for use as parametrize ids."""
    return [spec.name for spec in specs]


def _int_round_trip_cases(specs: List[IntFieldSpec]) -> list:
    """Build (field, value) params covering 0, 1, a midpoint, and both bounds."""
    cases = []
    for spec in specs:
        values = {0, 1, spec.min, spec.max, spec.max // 2}
        if spec.min < 0:
            values.update({-1, spec.min // 2})
        for value in sorted(values):
            cases.append(pytest.param(spec.name, value, id=f'{spec.name}={value}'))
    return cases


def _float_round_trip_cases(specs: List[FloatFieldSpec]) -> list:
    """Build (field, value) params from each spec's hand-picked values."""
    return [pytest.param(spec.name, value, id=f'{spec.name}={value}') for spec in specs for value in spec.values]


def _float_special_cases(specs: List[FloatFieldSpec]) -> list:
    """Build (field, value) params for each float type's finite extremes and the IEEE infinities."""
    cases = []
    for spec in specs:
        cases.append(pytest.param(spec.name, spec.finite_max, id=f'{spec.name}=max'))
        cases.append(pytest.param(spec.name, spec.finite_min, id=f'{spec.name}=min'))
        cases.append(pytest.param(spec.name, math.inf, id=f'{spec.name}=+inf'))
        cases.append(pytest.param(spec.name, -math.inf, id=f'{spec.name}=-inf'))
    return cases


def _int_out_of_range_cases(specs: List[IntFieldSpec]) -> list:
    """Build (field, value) params one past each end of every field's range."""
    cases = []
    for spec in specs:
        cases.append(pytest.param(spec.name, spec.max + 1, id=f'{spec.name}-over'))
        cases.append(pytest.param(spec.name, spec.min - 1, id=f'{spec.name}-under'))
    return cases


class TestSimpleField:
    """SIMPLE scalar fields: round-trip assignment, construction, defaults, and validation."""

    @pytest.fixture(scope='class')
    def simple_message_type(self) -> type:
        """Message exposing one SIMPLE field per supported (and a couple unsupported) scalar type."""
        fields = [
            FieldDefinition(name=spec.name, type=FIELD_TYPE.SIMPLE, conversion=spec.conversion, data_type=spec.data_type)
            for spec in ALL_SIMPLE_FIELDS
        ]
        msg_def = MessageDefinition(
            id='simple_msg', log_id=0, name='simple_msg', latest_message_crc=0,
            fields={0: fields},
        )
        db = MessageDatabase(message_family='OEM')
        db.append_messages([msg_def])
        return db.get_msg_type('simple_msg')

    class TestSignedIntegerFields:
        """Signed integer scalars accept and return Python ints across their full range."""

        @pytest.mark.parametrize('field,value', _int_round_trip_cases(SIGNED_INT_FIELDS))
        def test_setter(self, simple_message_type: type, field: str, value: int):
            # Arrange
            m = simple_message_type()
            # Act
            setattr(m, field, value)
            # Assert
            assert getattr(m, field) == value

        @pytest.mark.parametrize('field,value', _int_round_trip_cases(SIGNED_INT_FIELDS))
        def test_constructor(self, simple_message_type: type, field: str, value: int):
            # Act
            m = simple_message_type(**{field: value})
            # Assert
            assert getattr(m, field) == value

    class TestUnsignedIntegerFields:
        """Unsigned integer scalars accept and return Python ints across their full range."""

        @pytest.mark.parametrize('field,value', _int_round_trip_cases(UNSIGNED_INT_FIELDS))
        def test_setter(self, simple_message_type: type, field: str, value: int):
            # Arrange
            m = simple_message_type()
            # Act
            setattr(m, field, value)
            # Assert
            assert getattr(m, field) == value

        @pytest.mark.parametrize('field,value', _int_round_trip_cases(UNSIGNED_INT_FIELDS))
        def test_constructor(self, simple_message_type: type, field: str, value: int):
            # Act
            m = simple_message_type(**{field: value})
            # Assert
            assert getattr(m, field) == value

    class TestBooleanField:
        """The BOOL scalar round-trips Python bools."""
        field = BOOL_FIELD.name

        @pytest.mark.parametrize('value', [True, False])
        def test_setter(self, simple_message_type: type, value: bool):
            # Arrange
            m = simple_message_type()
            # Act
            setattr(m, self.field, value)
            # Assert
            assert getattr(m, self.field) == value

        @pytest.mark.parametrize('value', [True, False])
        def test_constructor(self, simple_message_type: type, value: bool):
            # Act
            m = simple_message_type(**{self.field: value})
            # Assert
            assert getattr(m, self.field) == value

    @pytest.mark.parametrize('field,value', _float_round_trip_cases(FLOAT_FIELDS))
    class TestFloatingPointFields:
        """Floating-point scalars round-trip within the precision of their storage type."""

        def test_setter(self, simple_message_type: type, field: str, value: float):
            # Arrange
            m = simple_message_type()
            # Act
            setattr(m, field, value)
            # Assert
            assert getattr(m, field) == pytest.approx(value, rel=1e-6)

        def test_constructor(self, simple_message_type: type, field: str, value: float):
            # Act
            m = simple_message_type(**{field: value})
            # Assert
            assert getattr(m, field) == pytest.approx(value, rel=1e-6)

    @pytest.mark.parametrize('field,value', _float_special_cases(FLOAT_FIELDS))
    class TestFloatingPointSpecialValues:
        """Floating-point scalars round-trip their type's finite extremes and +/-inf exactly."""

        def test_setter(self, simple_message_type: type, field: str, value: float):
            # Arrange
            m = simple_message_type()
            # Act
            setattr(m, field, value)
            # Assert
            assert getattr(m, field) == value

        def test_constructor(self, simple_message_type: type, field: str, value: float):
            # Act
            m = simple_message_type(**{field: value})
            # Assert
            assert getattr(m, field) == value

    class TestRetrievalTypes:
        """Retrieved scalars come back as the expected native Python type."""

        @pytest.mark.parametrize('field', _names(INT_FIELDS))
        def test_integer_fields_return_int(self, simple_message_type: type, field: str):
            # Arrange
            m = simple_message_type()
            # Act
            setattr(m, field, 1)
            # Assert
            assert isinstance(getattr(m, field), int)

        @pytest.mark.parametrize('field', _names(FLOAT_FIELDS))
        def test_float_fields_return_float(self, simple_message_type: type, field: str):
            # Arrange
            m = simple_message_type()
            # Act
            setattr(m, field, 1.0)
            # Assert
            assert isinstance(getattr(m, field), float)

        def test_bool_field_returns_bool(self, simple_message_type: type):
            # Arrange
            m = simple_message_type()
            # Act
            setattr(m, BOOL_FIELD.name, True)
            # Assert
            assert isinstance(getattr(m, BOOL_FIELD.name), bool)

    class TestDefaults:
        """A freshly constructed message zero-initializes its scalar fields."""

        @pytest.mark.parametrize('field', _names(INT_FIELDS))
        def test_integer_default_is_zero(self, simple_message_type: type, field: str):
            # Act
            m = simple_message_type()
            # Assert
            assert getattr(m, field) == 0

        @pytest.mark.parametrize('field', _names(FLOAT_FIELDS))
        def test_float_default_is_zero(self, simple_message_type: type, field: str):
            # Act
            m = simple_message_type()
            # Assert
            assert getattr(m, field) == 0.0

        def test_bool_default_is_false(self, simple_message_type: type):
            # Act
            m = simple_message_type()
            # Assert
            assert getattr(m, 'bool_val') is False

    class TestValidation:
        """Rejections raised when assigning incompatible values to scalar fields."""

        @pytest.mark.parametrize('field', _names(INT_FIELDS) + _names(FLOAT_FIELDS))
        @pytest.mark.parametrize('value', [
            pytest.param('not a number', id='str'),
            pytest.param(b'bytes', id='bytes'),
        ])
        def test_non_numeric_value_rejected(self, simple_message_type: type, field: str, value):
            m = simple_message_type()
            with pytest.raises(TypeError):
                setattr(m, field, value)

        @pytest.mark.parametrize('field,value', _int_out_of_range_cases(INT_FIELDS))
        def test_out_of_range_value_rejected(self, simple_message_type: type, field: str, value: int):
            m = simple_message_type()
            with pytest.raises((TypeError, ValueError, OverflowError)):
                setattr(m, field, value)

        @pytest.mark.skip
        @pytest.mark.parametrize('field', _names(UNSETTABLE_FIELDS))
        def test_unsupported_data_type_assignment_rejected(self, simple_message_type: type, field: str):
            m = simple_message_type()
            with pytest.raises((AttributeError, TypeError)):
                setattr(m, field, 1)


class TestGeneralValidation:
    """Message-shape-agnostic rejections on a synthetic message definition."""

    @pytest.fixture(scope='class')
    def message_type(self):
        """Message with a single INT scalar field."""
        msg_def = MessageDefinition(
            id='general_validation_msg', log_id=0, name='general_validation_msg', latest_message_crc=0,
            fields={0: [
                FieldDefinition(
                    name='value',
                    type=FIELD_TYPE.SIMPLE,
                    conversion=r'%d',
                    data_type=DATA_TYPE.INT,
                ),
            ]},
        )
        db = MessageDatabase(message_family='OEM')
        db.append_messages([msg_def])
        return db.get_msg_type('general_validation_msg')

    def test_unknown_kwarg_rejected(self, message_type: type):
        with pytest.raises(AttributeError, match='this_is_not_a_field'):
            message_type(this_is_not_a_field=1)

    def test_unknown_setattr_rejected(self, message_type: type):
        m = message_type()
        with pytest.raises(AttributeError, match='does_not_exist'):
            m.does_not_exist = 1
