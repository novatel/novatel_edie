"""Tests for the in-Python message API on ENUM fields, exercised against
synthetic MessageDefinitions built in-test (no reliance on the shipped OEM
database for the message types under test).

Scope:
 - EnumFieldDefinition: assignment by int and by IntEnum member, retrieval as
   IntEnum (including the EDIE_UNKNOWN fallback for values without an
   enumerator), defaults, and validation.

Every test runs against each enum in ENUM_SPECS, so the behaviour is exercised
against more than one enumerator layout (different names, values, and gaps).

ENUM values are stored as int32 in the FieldContainer; retrieval resolves them
to members of the IntEnum type generated for the bound EnumDefinition.
"""

from dataclasses import dataclass
from typing import List, Tuple

import pytest

from novatel_edie import (
    MessageDefinition,
    EnumFieldDefinition,
    EnumDefinition,
    EnumDataType,
    FIELD_TYPE,
    DATA_TYPE,
    MessageDatabase,
)


@dataclass(frozen=True)
class EnumSpec:
    """An enum definition under test: a name, its (enumerator-name, value) pairs,
    and the expected outcomes for values with no enumerator.
    """
    name: str
    members: Tuple[Tuple[str, int], ...]
    unknown_value: int
    non_member_values: Tuple[int, ...]

    @property
    def enum_id(self) -> str:
        return f'{self.name.lower()}_enum'

    @property
    def msg_id(self) -> str:
        return f'{self.name.lower()}_msg'


ENUM_SPECS = [
    EnumSpec('Color', (('RED', 1), ('GREEN', 2), ('BLUE', 4)),
             unknown_value=5, non_member_values=(0, 3, 5, 99, -1)),
    EnumSpec('Direction', (('NORTH', 1), ('EAST', 2), ('SOUTH', 3), ('WEST', 4)),
             unknown_value=5, non_member_values=(0, 5, 99, -1)),
]


def _member_cases(specs: List[EnumSpec]) -> list:
    """Build (spec, name, value) params, one per enumerator of every spec."""
    return [pytest.param(spec, name, value, id=f'{spec.name}-{name}')
            for spec in specs for name, value in spec.members]


def _name_cases(specs: List[EnumSpec]) -> list:
    """Build (spec, name) params, one per enumerator of every spec."""
    return [pytest.param(spec, name, id=f'{spec.name}-{name}')
            for spec in specs for name, _ in spec.members]


def _value_cases(specs: List[EnumSpec]) -> list:
    """Build (spec, value) params, one per enumerator value of every spec."""
    return [pytest.param(spec, value, id=f'{spec.name}-{value}')
            for spec in specs for _, value in spec.members]


def _spec_cases(specs: List[EnumSpec]) -> list:
    """Build (spec,) params, one per spec."""
    return [pytest.param(spec, id=spec.name) for spec in specs]


def _unknown_cases(specs: List[EnumSpec]) -> list:
    """Build (spec, value) params over each spec's values that have no enumerator."""
    return [pytest.param(spec, value, id=f'{spec.name}-{value}')
            for spec in specs for value in spec.non_member_values]


class TestEnumField:
    """ENUM fields: assignment by int and IntEnum member, retrieval as IntEnum, defaults, and validation."""

    @pytest.fixture(scope='class')
    def make_enum_db(self):
        """Cached factory: build (and memoize) a MessageDatabase for a given EnumSpec.

        The enum is appended before the message so MapMessageEnumFields can
        resolve the field's enum_id to its definition.
        """
        cache: dict = {}

        def build(spec: EnumSpec) -> MessageDatabase:
            if spec.enum_id not in cache:
                enum_def = EnumDefinition(
                    id=spec.enum_id, name=spec.name,
                    enumerators=[EnumDataType(value, name, f'{name}') for name, value in spec.members],
                )
                msg_def = MessageDefinition(
                    id=spec.msg_id, log_id=0, name=spec.msg_id, latest_message_crc=0,
                    fields={0: [
                        EnumFieldDefinition(name='enum_val', type=FIELD_TYPE.ENUM,
                                            data_type=DATA_TYPE.INT, enum_id=spec.enum_id),
                    ]},
                )
                db = MessageDatabase(message_family='OEM')
                db.append_enumerations([enum_def])
                db.append_messages([msg_def])
                cache[spec.enum_id] = db
            return cache[spec.enum_id]

        return build

    class TestAssignmentByInt:
        """Known enumerator values may be assigned as plain ints and read back as members."""

        @pytest.mark.parametrize('spec,name,value', _member_cases(ENUM_SPECS))
        def test_setter(self, make_enum_db, spec: EnumSpec, name: str, value: int):
            # Arrange
            m = make_enum_db(spec).get_msg_type(spec.msg_id)()
            # Act
            m.enum_val = value
            # Assert
            assert m.enum_val == value
            assert m.enum_val.name == name

        @pytest.mark.parametrize('spec,name,value', _member_cases(ENUM_SPECS))
        def test_constructor(self, make_enum_db, spec: EnumSpec, name: str, value: int):
            # Act
            m = make_enum_db(spec).get_msg_type(spec.msg_id)(enum_val=value)
            # Assert
            assert m.enum_val == value
            assert m.enum_val.name == name

    class TestAssignmentByMember:
        """Enumerator IntEnum members may be assigned directly."""

        @pytest.mark.parametrize('spec,name', _name_cases(ENUM_SPECS))
        def test_setter(self, make_enum_db, spec: EnumSpec, name: str):
            # Arrange
            db = make_enum_db(spec)
            m = db.get_msg_type(spec.msg_id)()
            member = getattr(db.get_enum_type_by_id(spec.enum_id), name)
            # Act
            m.enum_val = member
            # Assert
            assert m.enum_val == member
            assert m.enum_val.name == name

        @pytest.mark.parametrize('spec,name', _name_cases(ENUM_SPECS))
        def test_constructor(self, make_enum_db, spec: EnumSpec, name: str):
            # Arrange
            db = make_enum_db(spec)
            member = getattr(db.get_enum_type_by_id(spec.enum_id), name)
            # Act
            m = db.get_msg_type(spec.msg_id)(enum_val=member)
            # Assert
            assert m.enum_val == member
            assert m.enum_val.name == name

    class TestRetrieval:
        """Retrieval always yields a member of the bound IntEnum type."""

        @pytest.mark.parametrize('spec,value', _value_cases(ENUM_SPECS))
        def test_returns_enum_member(self, make_enum_db, spec: EnumSpec, value: int):
            # Arrange
            db = make_enum_db(spec)
            m = db.get_msg_type(spec.msg_id)()
            # Act
            m.enum_val = value
            # Assert
            assert isinstance(m.enum_val, db.get_enum_type_by_id(spec.enum_id))

        @pytest.mark.parametrize('spec,value', _unknown_cases(ENUM_SPECS))
        def test_unknown_value_maps_to_edie_unknown(self, make_enum_db, spec: EnumSpec, value: int):
            # Arrange
            m = make_enum_db(spec).get_msg_type(spec.msg_id)()
            # Act
            m.enum_val = value
            # Assert
            assert m.enum_val.name == 'EDIE_UNKNOWN'
            assert m.enum_val == spec.unknown_value

    class TestDefaults:
        """A freshly constructed ENUM field stores 0, which has no enumerator here and reads back as EDIE_UNKNOWN."""

        @pytest.mark.parametrize('spec', _spec_cases(ENUM_SPECS))
        def test_default_maps_to_edie_unknown(self, make_enum_db, spec: EnumSpec):
            # Act
            m = make_enum_db(spec).get_msg_type(spec.msg_id)()
            # Assert
            assert m.enum_val.name == 'EDIE_UNKNOWN'

    class TestValidation:
        """Rejections raised when assigning incompatible values to an ENUM field."""

        @pytest.mark.parametrize('spec', _spec_cases(ENUM_SPECS))
        @pytest.mark.parametrize('value', [
            pytest.param('RED', id='str'),
            pytest.param(b'RED', id='bytes'),
            pytest.param(1.5, id='float'),
        ])
        def test_non_integer_value_rejected(self, make_enum_db, spec: EnumSpec, value):
            m = make_enum_db(spec).get_msg_type(spec.msg_id)()
            with pytest.raises(TypeError):
                m.enum_val = value
