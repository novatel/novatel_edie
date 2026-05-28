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
    FieldArray
)


class TestStandardArray:
    """Tests for ArrayFieldDefinition-backed arrays (FIXED_LENGTH and VARIABLE_LENGTH)."""
    @pytest.fixture(scope='class')
    def standard_array_message_type(self):
        """Message with a single variable-length UCHAR array field (auto-generates payload_length)."""
        msg_def = MessageDefinition(
            id='standard_array_msg', log_id=0, name='standard_array_msg', latest_message_crc=0,
            fields={0: [
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
                'printable_buffer',
                type=FIELD_TYPE.VARIABLE_LENGTH_ARRAY,
                conversion=r'%P',
                data_type=DATA_TYPE.UCHAR,
                array_length=64,
            ),
            ArrayFieldDefinition(
                'char_fixed_arr',
                type=FIELD_TYPE.FIXED_LENGTH_ARRAY,
                conversion=r'%u',
                data_type=DATA_TYPE.UCHAR,
                array_length=64,
            ),
            ArrayFieldDefinition(
                'fixed_arr',
                type=FIELD_TYPE.FIXED_LENGTH_ARRAY,
                conversion=r'%hu',
                data_type=DATA_TYPE.USHORT,
                array_length=64,
            ),
            ArrayFieldDefinition(
                'var_arr',
                type=FIELD_TYPE.VARIABLE_LENGTH_ARRAY,
                conversion=r'%d',
                data_type=DATA_TYPE.UINT,
                array_length=64,
            ),
        ]},
        )
        db = MessageDatabase(message_family='OEM')
        db.append_messages([msg_def])
        return db.get_msg_type('standard_array_msg')

    @pytest.mark.parametrize('value', [
        pytest.param(b'', id='empty'),
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
        pytest.param(b'', id='empty'),
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
    class TestPrintableByteArraySetters:
        """Set the %P UCHAR variable-length array; accepts str/bytes/list, returns list of ints."""
        field = 'printable_buffer'

        def test_setter(self, standard_array_message_type: type, value: bytes, input_type: Callable):
            # Arrange
            m = standard_array_message_type()
            set_value = input_type(value)
            exp_value = list(value)
            # Act
            setattr(m, self.field, set_value)
            # Assert
            assert getattr(m, self.field) == exp_value

        def test_constructor(self, standard_array_message_type: type, value: bytes, input_type: Callable):
            # Arrange
            set_value = input_type(value)
            exp_value = list(value)
            # Act
            m = standard_array_message_type(**{self.field: set_value})
            # Assert
            assert getattr(m, self.field) == exp_value

    @pytest.mark.parametrize('value', [
        pytest.param([], id='empty'),
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

    @pytest.mark.parametrize('field,array_length', [
        ('fixed_arr', 64),
        ('char_fixed_arr', 64),
    ])
    def test_fixed_array_default_construction_is_zero_padded(self, standard_array_message_type: type, field: str, array_length: int):
        # Act
        m = standard_array_message_type()
        # Assert
        assert getattr(m, field) == [0] * array_length

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

        @pytest.mark.parametrize('field', ['char_fixed_arr', 'fixed_arr', 'var_arr'])
        @pytest.mark.parametrize('value', [
            pytest.param(b'abc', id='bytes'),
            pytest.param('abc', id='str'),
        ])
        def test_string_value_rejected_for_regular_field(self, standard_array_message_type: type, field: str, value):
            m = standard_array_message_type()
            with pytest.raises(TypeError):
                setattr(m, field, value)

        @pytest.mark.parametrize('field', ['char_fixed_arr', 'fixed_arr', 'var_arr'])
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
    def message_db(self) -> MessageDatabase:
        """Database holding two messages: one top-level FieldArray, one nested FieldArray."""
        point_msg = MessageDefinition(
            id='point_msg', log_id=0, name='point_msg', latest_message_crc=0,
            fields={0: [
                FieldArrayFieldDefinition(
                    name='points',
                    array_length=4,
                    fields=[
                        FieldDefinition(name='x', type=FIELD_TYPE.SIMPLE, data_type=DATA_TYPE.INT),
                        FieldDefinition(name='y', type=FIELD_TYPE.SIMPLE, data_type=DATA_TYPE.INT),
                    ],
                ),
            ]},
        )
        polyline_msg = MessageDefinition(
            id='polyline_msg', log_id=1, name='polyline_msg', latest_message_crc=0,
            fields={0: [
                FieldArrayFieldDefinition(
                    name='polylines',
                    array_length=4,
                    fields=[
                        FieldArrayFieldDefinition(
                            name='vertices',
                            array_length=8,
                            fields=[
                                FieldDefinition(name='x', type=FIELD_TYPE.SIMPLE, data_type=DATA_TYPE.INT),
                                FieldDefinition(name='y', type=FIELD_TYPE.SIMPLE, data_type=DATA_TYPE.INT),
                            ],
                        ),
                    ],
                ),
            ]},
        )
        db = MessageDatabase(message_family='OEM')
        db.append_messages([point_msg, polyline_msg])
        return db

    @pytest.fixture(scope='class')
    def point_message_type(self, message_db: MessageDatabase) -> type:
        return message_db.get_msg_type('point_msg')

    @pytest.fixture(scope='class')
    def polyline_message_type(self, message_db: MessageDatabase) -> type:
        return message_db.get_msg_type('polyline_msg')

    @pytest.fixture(scope='class')
    def point_type(self, message_db: MessageDatabase) -> type:
        return message_db.get_field_type('point_msg', 'points')

    @pytest.fixture(scope='class')
    def polyline_type(self, message_db: MessageDatabase) -> type:
        return message_db.get_field_type('polyline_msg', 'polylines')

    @pytest.fixture(scope='class')
    def vertex_type(self, message_db: MessageDatabase) -> type:
        return message_db.get_field_type('polyline_msg', ['polylines', 'vertices'])

    def test_field_array_construction(self, point_message_type: type, point_type: type):
        m = point_message_type(
            points=FieldArray([
                point_type(x=-5, y=5),
                point_type(x=0, y=0),
                point_type(x=10, y=-3),
            ])
        )
        assert len(m.points) == 3
        assert (m.points[0].x, m.points[0].y) == (-5, 5)
        assert (m.points[1].x, m.points[1].y) == (0, 0)
        assert (m.points[2].x, m.points[2].y) == (10, -3)

    def test_field_array_construction_implicit(self, point_message_type: type, point_type: type):
        m = point_message_type(
            points=[
                point_type(x=-5, y=5),
                point_type(x=0, y=0),
                point_type(x=10, y=-3),
            ]
        )
        assert len(m.points) == 3
        assert (m.points[0].x, m.points[0].y) == (-5, 5)
        assert (m.points[1].x, m.points[1].y) == (0, 0)
        assert (m.points[2].x, m.points[2].y) == (10, -3)


    def test_nested_field_array_construction(self, polyline_message_type: type, polyline_type: type, vertex_type: type):
        m = polyline_message_type(
            polylines=[
                polyline_type(vertices=[
                    vertex_type(x=0, y=0),
                    vertex_type(x=1, y=2),
                    vertex_type(x=3, y=4),
                ]),
                polyline_type(vertices=[
                    vertex_type(x=-1, y=-1),
                    vertex_type(x=-2, y=-2),
                ]),
            ]
        )
        assert len(m.polylines) == 2
        assert len(m.polylines[0].vertices) == 3
        assert (m.polylines[0].vertices[0].x, m.polylines[0].vertices[0].y) == (0, 0)
        assert (m.polylines[0].vertices[1].x, m.polylines[0].vertices[1].y) == (1, 2)
        assert (m.polylines[0].vertices[2].x, m.polylines[0].vertices[2].y) == (3, 4)
        assert len(m.polylines[1].vertices) == 2
        assert (m.polylines[1].vertices[0].x, m.polylines[1].vertices[0].y) == (-1, -1)
        assert (m.polylines[1].vertices[1].x, m.polylines[1].vertices[1].y) == (-2, -2)

    def test_field_array_reassignment(self, point_message_type: type, point_type: type):
        # Arrange
        p = point_type(x=-5, y=5)
        m = point_message_type(points=[p])
        # Act
        m.points = [point_type(x=0, y=0)]
        # Assert
        assert m.points[0].x == 0
        assert m.points[0].y == 0
        assert p.x == -5
        assert p.y == 5

    def test_field_array_setelem(self, point_type: type):
        # Arrange
        p1 = point_type(x=-5, y=5)
        p2 = point_type(x=-3, y=4)
        points = FieldArray([p1, p2])
        # Act
        points[1] = point_type(10, 7)
        # Assert
        assert points[0].x == -5
        assert points[0].y == 5

    def test_field_array_setattr_rejected(self, point_message_type: type):
        m = point_message_type()
        with pytest.raises(TypeError, match='FieldArray'):
            m.points = [1, 2, 3]

    def test_field_array_too_many_points_rejected(self, point_message_type: type, point_type: type):
        with pytest.raises(ValueError, match='maximum array size'):
            point_message_type(
                points=[point_type(x=i, y=i) for i in range(5)]
            )

    def test_field_array_too_many_points_rejected_direct(self, point_type: type):
        with pytest.raises(ValueError, match='maximum array size'):
            FieldArray([point_type(x=i, y=i) for i in range(5)])


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

    def test_setattr_type_mismatch_raises(self, message_type: type):
        m = message_type()
        with pytest.raises(TypeError):
            m.value = 'not a number'
