"""Tests for the in-Python message API on bitmask (BitField) fields, exercised
against synthetic databases built in-test from JSON (no reliance on the shipped
OEM database for the bitmasks under test).

Scope:
 - Concrete per-bitmask BitField subtypes: direct construction resolves the
   owning MessageDatabase from the class's ``_owner_db`` attribute and looks up
   the bitmask interpretation from the first class in its MRO that the database
   registered, mirroring the
   Field/Message ``__new__`` pattern. A directly-constructed subtype must expose
   the same named sub-masks (and typed IntEnum sub-masks) as the instance
   produced on the decode path (PyField::convert_field).
 - The base ``BitField``: the shared base for the generated per-bitmask
   subtypes, not constructible on its own (direct instantiation raises).
   Concrete subtypes behave like their underlying integer value.

Every case runs against two equivalent databases: one parsed from JSON with
``MessageDatabase.from_string``, and one assembled from ``EnumDefinition``,
``BitMaskCollectionDefinition``, ``BitMaskDefinition`` and ``MessageDefinition`` objects. A SIMPLE field carrying
a bitmask ID is exposed as its concrete BitField subtype; ``type(message.<field>)``
recovers that subtype for direct construction.

Sub-mask extraction follows ``BitMask::fromRange(start, end)`` — bits ``[start,
end)`` shifted down to bit 0 — and enum sub-masks resolve to the bound IntEnum
type, falling back to EDIE_UNKNOWN for values with no enumerator.
"""

import json
import operator
from dataclasses import dataclass
from typing import List, Optional, Tuple

import pytest
from novatel_edie import (
    DATA_TYPE,
    FIELD_TYPE,
    BitField,
    BitMaskCollectionDefinition,
    BitMaskDefinition,
    EnumDataType,
    EnumDefinition,
    FieldDefinition,
    MessageDatabase,
    MessageDefinition,
)


@dataclass(frozen=True)
class EnumSpec:
    """An enum referenced by an enum-typed sub-mask."""
    id: str
    name: str
    members: Tuple[Tuple[str, int], ...]


@dataclass(frozen=True)
class SubMaskSpec:
    """A named sub-mask covering bits [start, end); enum names the bound EnumSpec if any."""
    name: str
    start: int
    end: int
    enum: Optional[str] = None

    def extract(self, value: int) -> int:
        """The value of this sub-mask within `value`, shifted down to bit 0."""
        return (value >> self.start) & ((1 << (self.end - self.start)) - 1)


@dataclass(frozen=True)
class BitmaskSpec:
    """A bitmask definition under test, its sub-masks, and any referenced enums."""
    name: str
    submasks: Tuple[SubMaskSpec, ...]
    enums: Tuple[EnumSpec, ...] = ()

    @property
    def bitmask_id(self) -> str:
        return f'{self.name.lower()}_id'

    @property
    def msg_name(self) -> str:
        return f'{self.name.upper()}MSG'

    @property
    def field_name(self) -> str:
        return 'status'

    def submask(self, name: str) -> SubMaskSpec:
        return next(sm for sm in self.submasks if sm.name == name)

    def enum(self, enum_name: str) -> EnumSpec:
        return next(e for e in self.enums if e.name == enum_name)


MODE_ENUM = EnumSpec('mode_enum', 'Mode', (('OFF', 0), ('ON', 1), ('AUTO', 2)))

BITMASK_SPECS = [
    # Mix of plain-int and enum-typed sub-masks.
    BitmaskSpec('Status',
                submasks=(SubMaskSpec('low', 0, 4),
                          SubMaskSpec('high', 4, 8),
                          SubMaskSpec('mode', 8, 10, enum='Mode')),
                enums=(MODE_ENUM,)),
    # Plain-int sub-masks only, including single-bit flags.
    BitmaskSpec('Flags',
                submasks=(SubMaskSpec('a', 0, 1),
                          SubMaskSpec('b', 1, 2),
                          SubMaskSpec('rest', 2, 8))),
]

# 32-bit values exercised against sub-mask extraction.
VALUES = [0x00000000, 0xFFFFFFFF, 0xDEADBEEF, 0x0000FFFF, 0xA5A5A5A5]

# Operands for operator tests. Kept small so pow and shifts stay cheap.
OPERAND_PAIRS = [pytest.param(3, 3, id='equal'), pytest.param(11, 3, id='unequal')]
UNARY_OPERANDS = [pytest.param(0, id='zero'), pytest.param(11, id='nonzero')]
OPERAND = 11

BINARY_OPERATORS = [
    operator.eq, operator.ne, operator.lt, operator.le, operator.gt, operator.ge,
    operator.add, operator.sub, operator.mul, operator.truediv, operator.floordiv, operator.mod, divmod, operator.pow,
    operator.and_, operator.or_, operator.xor, operator.lshift, operator.rshift,
]

UNARY_OPERATORS = [operator.neg, operator.pos, abs, operator.invert]


def _build_db_from_json(spec: BitmaskSpec) -> MessageDatabase:
    """Build a MessageDatabase from JSON exposing a single message whose only
    field is a SIMPLE ULONG carrying `spec`'s bitmask.
    """
    db_dict = {
        'meta': {'messageFamily': 'OEM', 'version': '1.0.0'},
        'enums': [
            {'_id': e.id, 'name': e.name,
             'enumerators': [{'value': value, 'name': name, 'description': ''}
                             for name, value in e.members]}
            for e in spec.enums
        ],
        'bitmasks': [
            {'_id': spec.bitmask_id, 'name': spec.name,
             'masks': {
                 sm.name: {'start': sm.start, 'end': sm.end,
                           **({'enumID': spec.enum(sm.enum).id} if sm.enum else {})}
                 for sm in spec.submasks
             }},
        ],
        'messages': [
            {'_id': f'{spec.name.lower()}_msg', 'messageID': 1, 'name': spec.msg_name,
             'description': '', 'latestMsgDefCrc': '0',
             'fields': {'0': [
                 {'name': spec.field_name, 'type': 'SIMPLE', 'description': '',
                  'dataType': {'name': 'ULONG', 'length': 4, 'description': ''},
                  'bitmaskID': spec.bitmask_id},
             ]}},
        ],
    }
    return MessageDatabase.from_string(json.dumps(db_dict))


def _build_db_from_definitions(spec: BitmaskSpec) -> MessageDatabase:
    """Build the same database as `_build_db_from_json` from definition objects."""
    enum_defs = {e.name: EnumDefinition(id=e.id, name=e.name, enumerators=[EnumDataType(value, name) for name, value in e.members])
                 for e in spec.enums}
    bitmask_def = BitMaskCollectionDefinition(
        id=spec.bitmask_id, name=spec.name,
        masks={sm.name: BitMaskDefinition(sm.start, sm.end, enum_defs.get(sm.enum)) for sm in spec.submasks})
    msg_def = MessageDefinition(
        id=f'{spec.name.lower()}_msg', log_id=1, name=spec.msg_name, latest_message_crc=0,
        fields={0: [FieldDefinition(name=spec.field_name, type=FIELD_TYPE.SIMPLE, data_type=DATA_TYPE.ULONG,
                                    bitmask_id=spec.bitmask_id)]})

    db = MessageDatabase(message_family='OEM')
    db.append_enumerations(list(enum_defs.values()))
    db.append_bitmasks([bitmask_def])
    db.append_messages([msg_def])
    return db


DB_BUILDERS = {'json': _build_db_from_json, 'definitions': _build_db_from_definitions}


def _plain_submask_cases(specs: List[BitmaskSpec]) -> list:
    """Build (spec, submask, value) params, one per plain-int sub-mask × value."""
    return [pytest.param(spec, sm, value, id=f'{spec.name}-{sm.name}-{value:#010x}')
            for spec in specs for sm in spec.submasks if sm.enum is None for value in VALUES]


def _enum_submask_cases(specs: List[BitmaskSpec]) -> list:
    """Build (spec, submask) params, one per enum-typed sub-mask."""
    return [pytest.param(spec, sm, id=f'{spec.name}-{sm.name}')
            for spec in specs for sm in spec.submasks if sm.enum is not None]


def _spec_cases(specs: List[BitmaskSpec]) -> list:
    """Build (spec,) params, one per spec."""
    return [pytest.param(spec, id=spec.name) for spec in specs]


def _spec_value_cases(specs: List[BitmaskSpec]) -> list:
    """Build (spec, value) params, one per spec × value."""
    return [pytest.param(spec, value, id=f'{spec.name}-{value:#010x}')
            for spec in specs for value in VALUES]


@pytest.fixture(scope='module', params=list(DB_BUILDERS))
def make_bitmask_db(request):
    """Cached factory: build (and memoize) a MessageDatabase for a given BitmaskSpec with each builder."""
    builder = DB_BUILDERS[request.param]
    cache: dict = {}

    def build(spec: BitmaskSpec) -> MessageDatabase:
        if spec.name not in cache:
            cache[spec.name] = builder(spec)
        return cache[spec.name]

    return build


def _bitfield_type(db: MessageDatabase, spec: BitmaskSpec):
    """Recover the concrete BitField subtype for `spec` via a default message field."""
    message = db.get_msg_type(spec.msg_name)()
    return type(getattr(message, spec.field_name))



class TestConstruction:
    """Direct construction of a concrete subtype attaches the bitmask interpretation."""

    @pytest.mark.parametrize('spec,value', _spec_value_cases(BITMASK_SPECS))
    def test_value_stored(self, make_bitmask_db, spec: BitmaskSpec, value: int):
        # Arrange
        bitfield_type = _bitfield_type(make_bitmask_db(spec), spec)
        # Act
        instance = bitfield_type(value)
        # Assert
        assert instance.value == value

    @pytest.mark.parametrize('spec', _spec_cases(BITMASK_SPECS))
    def test_default_value_is_zero(self, make_bitmask_db, spec: BitmaskSpec):
        # Arrange
        bitfield_type = _bitfield_type(make_bitmask_db(spec), spec)
        # Act
        instance = bitfield_type()
        # Assert
        assert instance.value == 0

    @pytest.mark.parametrize('spec', _spec_cases(BITMASK_SPECS))
    def test_repr_shows_mask_id(self, make_bitmask_db, spec: BitmaskSpec):
        # Arrange
        bitfield_type = _bitfield_type(make_bitmask_db(spec), spec)
        # Act
        text = repr(bitfield_type(0))
        # Assert
        assert 'mask_id' in text

    @pytest.mark.parametrize('spec', _spec_cases(BITMASK_SPECS))
    def test_dir_lists_submasks(self, make_bitmask_db, spec: BitmaskSpec):
        # Arrange
        bitfield_type = _bitfield_type(make_bitmask_db(spec), spec)
        # Act
        names = dir(bitfield_type(0))
        # Assert
        assert {sm.name for sm in spec.submasks}.issubset(names)

    @pytest.mark.parametrize('spec', _spec_cases(BITMASK_SPECS))
    def test_is_instance_of_base_bitfield(self, make_bitmask_db, spec: BitmaskSpec):
        # Arrange
        bitfield_type = _bitfield_type(make_bitmask_db(spec), spec)
        # Act
        instance = bitfield_type(0)
        # Assert
        assert isinstance(instance, BitField)


class TestSubMaskExtraction:
    """Plain-int sub-masks extract the bits described by their range."""

    @pytest.mark.parametrize('spec,submask,value', _plain_submask_cases(BITMASK_SPECS))
    def test_extracts_expected_bits(self, make_bitmask_db, spec: BitmaskSpec, submask: SubMaskSpec, value: int):
        # Arrange
        bitfield_type = _bitfield_type(make_bitmask_db(spec), spec)
        # Act
        extracted = getattr(bitfield_type(value), submask.name)
        # Assert
        assert extracted == submask.extract(value)

    @pytest.mark.parametrize('spec', _spec_cases(BITMASK_SPECS))
    def test_unknown_submask_name_raises(self, make_bitmask_db, spec: BitmaskSpec):
        # Arrange
        bitfield_type = _bitfield_type(make_bitmask_db(spec), spec)
        # Act / Assert
        with pytest.raises(AttributeError):
            bitfield_type(0).not_a_submask


class TestEnumSubMask:
    """Enum-typed sub-masks resolve to members of the bound IntEnum type."""

    @pytest.mark.parametrize('spec,submask', _enum_submask_cases(BITMASK_SPECS))
    def test_known_value_returns_member(self, make_bitmask_db, spec: BitmaskSpec, submask: SubMaskSpec):
        # Arrange
        db = make_bitmask_db(spec)
        bitfield_type = _bitfield_type(db, spec)
        enum_spec = spec.enum(submask.enum)
        enum_type = db.get_enum_type_by_id(enum_spec.id)
        # Act / Assert: every enumerator, placed at the sub-mask's offset, reads back as its member.
        for name, member_value in enum_spec.members:
            extracted = getattr(bitfield_type(member_value << submask.start), submask.name)
            assert isinstance(extracted, enum_type)
            assert extracted == member_value
            assert extracted.name == name

    @pytest.mark.parametrize('spec,submask', _enum_submask_cases(BITMASK_SPECS))
    def test_unknown_value_maps_to_edie_unknown(self, make_bitmask_db, spec: BitmaskSpec, submask: SubMaskSpec):
        # Arrange
        db = make_bitmask_db(spec)
        bitfield_type = _bitfield_type(db, spec)
        enum_spec = spec.enum(submask.enum)
        enum_type = db.get_enum_type_by_id(enum_spec.id)
        known_values = {value for _, value in enum_spec.members}
        width = submask.end - submask.start
        unknown = next(v for v in range(1 << width) if v not in known_values)
        # Act
        extracted = getattr(bitfield_type(unknown << submask.start), submask.name)
        # Assert
        assert extracted == enum_type.EDIE_UNKNOWN


class TestConsistencyWithDecodePath:
    """A directly-constructed subtype matches the instance read off a message field set to the same value."""

    @pytest.mark.parametrize('spec,value', _spec_value_cases(BITMASK_SPECS))
    def test_submasks_match_field_readback(self, make_bitmask_db, spec: BitmaskSpec, value: int):
        # Arrange
        db = make_bitmask_db(spec)
        message = db.get_msg_type(spec.msg_name)()
        bitfield_type = type(getattr(message, spec.field_name))
        # Act
        direct = bitfield_type(value)
        message.status = value
        from_field = getattr(message, spec.field_name)
        # Assert
        assert type(direct) is type(from_field)
        assert direct.value == from_field.value
        for sm in spec.submasks:
            assert getattr(direct, sm.name) == getattr(from_field, sm.name)


class TestIntegerBehaviour:
    """A concrete BitField subtype behaves like its underlying integer value."""

    @pytest.mark.parametrize('spec,value', _spec_value_cases(BITMASK_SPECS))
    def test_delegates_to_int(self, make_bitmask_db, spec: BitmaskSpec, value: int):
        # Arrange
        bitfield_type = _bitfield_type(make_bitmask_db(spec), spec)
        # Act
        instance = bitfield_type(value)
        # Assert
        assert int(instance) == value
        assert instance == value
        assert instance + 1 == value + 1
        assert instance | 0 == value
        assert (instance & 0xFF) == (value & 0xFF)


class TestOperators:
    """Every operator on a BitField gives the same result, of the same type, as on its integer value."""

    @pytest.mark.parametrize('left,right', OPERAND_PAIRS)
    @pytest.mark.parametrize('op', BINARY_OPERATORS, ids=lambda op: op.__name__)
    @pytest.mark.parametrize('spec', _spec_cases(BITMASK_SPECS))
    def test_bitfield_with_int(self, make_bitmask_db, spec: BitmaskSpec, op, left: int, right: int):
        # Arrange
        bitfield = _bitfield_type(make_bitmask_db(spec), spec)(left)
        expected = op(left, right)
        # Act
        result = op(bitfield, right)
        # Assert
        assert result == expected
        assert type(result) is type(expected)

    @pytest.mark.parametrize('left,right', OPERAND_PAIRS)
    @pytest.mark.parametrize('op', BINARY_OPERATORS, ids=lambda op: op.__name__)
    @pytest.mark.parametrize('spec', _spec_cases(BITMASK_SPECS))
    def test_int_with_bitfield(self, make_bitmask_db, spec: BitmaskSpec, op, left: int, right: int):
        # Arrange
        bitfield = _bitfield_type(make_bitmask_db(spec), spec)(right)
        expected = op(left, right)
        # Act
        result = op(left, bitfield)
        # Assert
        assert result == expected
        assert type(result) is type(expected)

    @pytest.mark.parametrize('left,right', OPERAND_PAIRS)
    @pytest.mark.parametrize('op', BINARY_OPERATORS, ids=lambda op: op.__name__)
    @pytest.mark.parametrize('spec', _spec_cases(BITMASK_SPECS))
    def test_bitfield_with_bitfield(self, make_bitmask_db, spec: BitmaskSpec, op, left: int, right: int):
        # Arrange
        bitfield_type = _bitfield_type(make_bitmask_db(spec), spec)
        expected = op(left, right)
        # Act
        result = op(bitfield_type(left), bitfield_type(right))
        # Assert
        assert result == expected
        assert type(result) is type(expected)

    @pytest.mark.parametrize('operand', UNARY_OPERANDS)
    @pytest.mark.parametrize('op', UNARY_OPERATORS, ids=lambda op: op.__name__)
    @pytest.mark.parametrize('spec', _spec_cases(BITMASK_SPECS))
    def test_unary(self, make_bitmask_db, spec: BitmaskSpec, op, operand: int):
        # Arrange
        bitfield = _bitfield_type(make_bitmask_db(spec), spec)(operand)
        # Act
        result = op(bitfield)
        # Assert
        assert result == op(operand)
        assert type(result) is int

    @pytest.mark.parametrize('spec', _spec_cases(BITMASK_SPECS))
    def test_conversions(self, make_bitmask_db, spec: BitmaskSpec):
        # Arrange
        bitfield_type = _bitfield_type(make_bitmask_db(spec), spec)
        bitfield = bitfield_type(OPERAND)
        # Act / Assert
        assert float(bitfield) == float(OPERAND)
        assert operator.index(bitfield) == OPERAND
        assert hex(bitfield) == hex(OPERAND)
        assert list(range(OPERAND + 1))[bitfield] == OPERAND
        assert bool(bitfield) is True
        assert bool(bitfield_type(0)) is False

    @pytest.mark.parametrize('spec', _spec_cases(BITMASK_SPECS))
    def test_hash_matches_int(self, make_bitmask_db, spec: BitmaskSpec):
        # Arrange
        bitfield = _bitfield_type(make_bitmask_db(spec), spec)(OPERAND)
        # Act / Assert
        assert hash(bitfield) == hash(OPERAND)
        assert {OPERAND: 'found'}[bitfield] == 'found'

    @pytest.mark.parametrize('op', [operator.add, operator.and_, operator.lt], ids=lambda op: op.__name__)
    @pytest.mark.parametrize('spec', _spec_cases(BITMASK_SPECS))
    def test_unsupported_operand_raises(self, make_bitmask_db, spec: BitmaskSpec, op):
        # Arrange
        bitfield = _bitfield_type(make_bitmask_db(spec), spec)(OPERAND)
        # Act / Assert
        with pytest.raises(TypeError):
            op(bitfield, 'not a number')


class TestBaseNotConstructible:
    """The base BitField is the shared base for the generated subtypes and cannot be instantiated directly."""

    @pytest.mark.parametrize('construct', [
        pytest.param(lambda: BitField(), id='no-args'),
        pytest.param(lambda: BitField(0), id='positional'),
        pytest.param(lambda: BitField(value=0), id='keyword'),
    ])
    def test_direct_construction_raises(self, construct):
        # Act / Assert
        with pytest.raises(TypeError):
            construct()
