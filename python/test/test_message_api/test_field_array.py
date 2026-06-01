"""Tests for the in-Python message construction / mutation API on
FieldArrayFieldDefinition-backed arrays of nested records, exercised against
synthetic MessageDefinitions built in-test (no reliance on the shipped OEM
database for the message types under test).
"""

import pytest

from novatel_edie import (
    MessageDefinition,
    FieldDefinition,
    FieldArrayFieldDefinition,
    FIELD_TYPE,
    DATA_TYPE,
    MessageDatabase,
    FieldArray,
)


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
        points[1] = point_type(x=10, y=7)
        # Assert
        assert points[0].x == -5
        assert points[0].y == 5
        assert points[1].x == 10
        assert points[1].y == 7
        assert p2.x == -3
        assert p2.y == 4

    def test_nested_field_array_reassignment(self, polyline_message_type: type, polyline_type: type, vertex_type: type):
        # Arrange
        v1 = vertex_type(x=1, y=1)
        p1 = polyline_type(vertices=[vertex_type(x=0, y=0), v1])
        p2 = polyline_type(vertices=[vertex_type(x=2, y=2)])
        m = polyline_message_type(polylines=[p1, p2])
        # Act
        m.polylines = [polyline_type(vertices=[vertex_type(x=10, y=10), vertex_type(x=11, y=11)])]
        # Assert
        assert len(m.polylines) == 1
        assert len(m.polylines[0].vertices) == 2
        assert (m.polylines[0].vertices[0].x, m.polylines[0].vertices[0].y) == (10, 10)
        assert (m.polylines[0].vertices[1].x, m.polylines[0].vertices[1].y) == (11, 11)
        # Originals remain independent
        assert v1.x == 1
        assert v1.y == 1
        assert len(p1.vertices) == 2
        assert (p1.vertices[0].x, p1.vertices[0].y) == (0, 0)
        assert (p1.vertices[1].x, p1.vertices[1].y) == (1, 1)
        assert len(p2.vertices) == 1
        assert (p2.vertices[0].x, p2.vertices[0].y) == (2, 2)

    def test_nested_field_array_setelem(self, polyline_type: type, vertex_type: type):
        # Arrange
        p1 = polyline_type(vertices=[vertex_type(x=0, y=0), vertex_type(x=1, y=1)])
        p2 = polyline_type(vertices=[vertex_type(x=2, y=2)])
        polylines = FieldArray([p1, p2])
        # Act
        polylines[1] = polyline_type(vertices=[vertex_type(x=10, y=10), vertex_type(x=11, y=11), vertex_type(x=12, y=12)])
        # Assert
        assert len(polylines[0].vertices) == 2
        assert (polylines[0].vertices[0].x, polylines[0].vertices[0].y) == (0, 0)
        assert (polylines[0].vertices[1].x, polylines[0].vertices[1].y) == (1, 1)
        assert len(polylines[1].vertices) == 3
        assert (polylines[1].vertices[0].x, polylines[1].vertices[0].y) == (10, 10)
        assert (polylines[1].vertices[1].x, polylines[1].vertices[1].y) == (11, 11)
        assert (polylines[1].vertices[2].x, polylines[1].vertices[2].y) == (12, 12)
        # Original p2 is still independent
        assert len(p2.vertices) == 1
        assert (p2.vertices[0].x, p2.vertices[0].y) == (2, 2)

    def test_field_array_setelem_over_temp(self, point_type: type):
        # Arrange
        points = FieldArray([point_type(x=-5, y=5)])
        # Act
        points[0] = point_type(x=0, y=1)
        # Assert
        assert points[0].x == 0
        assert points[0].y == 1

    def test_empty_field_array(self, point_message_type: type):
        # Act
        points = FieldArray([])
        m = point_message_type(points=points)
        # Assert
        assert len(points) == 0
        assert len(m.points) == 0

    def test_empty_field_array_implicit(self, point_message_type: type):
        # Act
        m = point_message_type(points=[])
        # Assert
        assert len(m.points) == 0

    def test_drop_temp_field_array(self, point_message_type: type, point_type: type):
        # Arrange
        m = point_message_type(points=[point_type(x=0,y=1)])
        # Act
        m.points = []
        # Assert
        assert len(m.points) == 0

    def test_drop_non_temp_field_array(self, point_message_type: type, point_type: type):
        # Arrange
        points = [point_type(x=0,y=1)]
        m = point_message_type(points=points)
        # Act
        m.points = []
        # Assert
        assert points[0].x == 0
        assert points[0].y == 1
        assert len(m.points) == 0


    @pytest.mark.skip('Not implemented')
    def test_field_array_delelem(self, point_type: type):
        # Arrange
        p1 = point_type(x=-5, y=5)
        p2 = point_type(x=-3, y=4)
        points = FieldArray([p1, p2])
        # Act
        del points[0]
        # Assert
        assert points[0].x == 3
        assert points[0].y == 4

    @pytest.mark.skip('Not implemented')
    def test_field_array_append(self, point_type: type):
        # Arrange
        p1 = point_type(x=-5, y=5)
        p2 = point_type(x=-3, y=4)
        points = FieldArray([p1])
        # Act
        points.append(p2)
        # Assert
        assert points[0].x == -5
        assert points[0].y == 5
        assert points[1].x == 3
        assert points[1].y == 4


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
