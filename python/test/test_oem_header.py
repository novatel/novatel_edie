"""Tests for in-Python construction of OEM message headers."""

import pytest

from novatel_edie import TIME_STATUS, MESSAGE_FORMAT
import novatel_edie.oem as oem
from novatel_edie.oem import RECEIVER_STATUS_VERSION


@pytest.mark.parametrize('kwargs', [
    pytest.param({}, id='defaults'),
    pytest.param({
        'message_id': 42,
        'message_type': 0x42,
        'port_address': 0xC0,
        'length': 128,
        'sequence': 7,
        'idle_time': 10,
        'time_status': TIME_STATUS.FINESTEERING,
        'week': 2300,
        'milliseconds': 345600.0,
        'receiver_status': 0x12345678,
        'message_definition_crc': 0xDEADBEEF,
        'receiver_sw_version': 1234,
    }, id='all_fields'),
    pytest.param({
        'message_id': 7,
        'port_address': 0xAB,
        'time_status': TIME_STATUS.COARSE,
        'week': 1234,
        'milliseconds': 500.0,
        'receiver_sw_version': 99,
    }, id='partial_fields'),
])
class TestHeaderFields:
    """Round-trip every Header field through both construction and setattr."""

    @staticmethod
    def _assert_fields(h: oem.Header, expected: dict):
        """Asserts that fields match what is expected"""
        assert h.message_id == expected['message_id']
        assert h.message_type.raw_value == expected['message_type']
        assert h.port_address == expected['port_address']
        assert h.length == expected['length']
        assert h.sequence == expected['sequence']
        assert h.idle_time == expected['idle_time']
        assert h.time_status == expected['time_status']
        assert h.week == expected['week']
        assert h.milliseconds == expected['milliseconds']
        assert h.receiver_status.raw_value == expected['receiver_status']
        assert h.message_definition_crc == expected['message_definition_crc']
        assert h.receiver_sw_version == expected['receiver_sw_version']

    def test_kw_construct(self, kwargs: dict):
        expected = {
            'message_id': 0,
            'message_type': 0,
            'port_address': 0,
            'length': 0,
            'sequence': 0,
            'idle_time': 0,
            'time_status': TIME_STATUS.UNKNOWN,
            'week': 0,
            'milliseconds': 0.0,
            'receiver_status': 0,
            'message_definition_crc': 0,
            'receiver_sw_version': 0,
            **kwargs,
        }
        h = oem.Header(**kwargs)
        self._assert_fields(h, expected)

    def test_setattr(self, kwargs: dict):
        expected = {
            'message_id': 0,
            'message_type': 0,
            'port_address': 0,
            'length': 0,
            'sequence': 0,
            'idle_time': 0,
            'time_status': TIME_STATUS.UNKNOWN,
            'week': 0,
            'milliseconds': 0.0,
            'receiver_status': 0,
            'message_definition_crc': 0,
            'receiver_sw_version': 0,
            **kwargs,
        }
        h = oem.Header()
        for key, value in kwargs.items():
            setattr(h, key, value)
        self._assert_fields(h, expected)


class TestTimeStatus:
    """Round-trip TIME_STATUS values through the header property."""

    @pytest.mark.parametrize('value', list(TIME_STATUS))
    def test_enum_value_setting(self, value: TIME_STATUS):
        h = oem.Header(time_status=value)
        assert h.time_status == value

    @pytest.mark.parametrize('value', [0, 1, 20, 99, 150, 255])
    def test_non_enum_setting(self, value: int):
        h = oem.Header(time_status=value)
        assert h.time_status == value
        if value in list(TIME_STATUS):
            assert isinstance(h.time_status, TIME_STATUS)
        else:
            assert not isinstance(h.time_status, TIME_STATUS)
        assert isinstance(h.time_status, int)


class TestReceiverStatus:
    """Round-trip RecieverStatus values through the header property."""

    @pytest.mark.parametrize('value', [0, 0x1, 0x12345678, 0xFFFFFFFF])
    def test_int_round_trip(self, value: int):
        h = oem.Header(receiver_status=value)
        assert h.receiver_status.raw_value == value

    @pytest.mark.parametrize('value', [0, 0x1, 0x12345678, 0xFFFFFFFF])
    def test_positional_int_construct(self, value: int):
        rs = oem.RecieverStatus(value)
        assert rs.raw_value == value

    @pytest.mark.parametrize('field_name,mask', [
        ('reciever_error', 0x00000001),
        ('temperature_warning', 0x00000002),
        ('voltage_warning', 0x00000004),
        ('antenna_powered', 0x00000008),
        ('lna_failure', 0x00000010),
        ('antenna_open_circuit', 0x00000020),
        ('antenna_short_circuit', 0x00000040),
        ('cpu_overload', 0x00000080),
        ('com_buffer_overrun', 0x00000100),
        ('spoofing_detected', 0x00000200),
        ('reserved', 0x00000400),
        ('link_overrun', 0x00000800),
        ('input_overrun', 0x00001000),
        ('aux_transmit_overrun', 0x00002000),
        ('antenna_gain_out_of_range', 0x00004000),
        ('jammer_detected', 0x00008000),
        ('ins_reset', 0x00010000),
        ('imu_communication_failure', 0x00020000),
        ('gps_almanac_invalid', 0x00040000),
        ('position_solution_invalid', 0x00080000),
        ('position_fixed', 0x00100000),
        ('clock_steering_disabled', 0x00200000),
        ('clock_model_invalid', 0x00400000),
        ('external_oscillator_locked', 0x00800000),
        ('software_resource_warning', 0x01000000),
        ('tracking_mode_hdr', 0x08000000),
        ('digital_filtering_enabled', 0x10000000),
        ('auxiliary_3_event', 0x20000000),
        ('auxiliary_2_event', 0x40000000),
        ('auxiliary_1_event', 0x80000000),
    ])
    class TestAllFields:
        """Set each flag in isolation and verify it maps to its expected bit."""

        def test_via_constructor(self, field_name: str, mask: int):
            rs = oem.RecieverStatus(**{field_name: True})
            assert getattr(rs, field_name) is True
            assert rs.raw_value == mask

        def test_via_setattr(self, field_name: str, mask: int):
            rs = oem.RecieverStatus()
            setattr(rs, field_name, True)
            assert getattr(rs, field_name) is True
            assert rs.raw_value == mask

    @pytest.mark.parametrize('value', list(RECEIVER_STATUS_VERSION))
    class TestVersionBits:
        """Set the 2-bit version field via each RECEIVER_STATUS_VERSION member and verify it maps to bits 25-26."""

        def test_via_constructor(self, value: RECEIVER_STATUS_VERSION):
            rs = oem.RecieverStatus(version_bits=value)
            assert rs.version_bits == value
            assert isinstance(rs.version_bits, RECEIVER_STATUS_VERSION)
            assert rs.raw_value == int(value) << 25

        def test_via_setattr(self, value: RECEIVER_STATUS_VERSION):
            rs = oem.RecieverStatus()
            rs.version_bits = value
            assert rs.version_bits == value
            assert isinstance(rs.version_bits, RECEIVER_STATUS_VERSION)
            assert rs.raw_value == int(value) << 25

    @pytest.mark.parametrize('kwargs,expected_raw', [
        ({}, 0x00000000),
        ({'reciever_error': True}, 0x00000001),
        ({'spoofing_detected': True}, 0x00000200),
        ({'jammer_detected': True, 'spoofing_detected': True}, 0x00008200),
        ({'auxiliary_1_event': True}, 0x80000000),
        ({'reciever_error': True, 'auxiliary_1_event': True}, 0x80000001),
    ])
    class TestFieldConstruction:
        """Build a RecieverStatus from its decoded boolean fields and verify each path."""

        def test_construct(self, kwargs: dict, expected_raw: int):
            rs = oem.RecieverStatus(**kwargs)
            assert rs.raw_value == expected_raw
            for field_name, expected_value in kwargs.items():
                assert getattr(rs, field_name) == expected_value

        def test_header_round_trip(self, kwargs: dict, expected_raw: int):
            rs = oem.RecieverStatus(**kwargs)
            h = oem.Header(receiver_status=rs)
            assert h.receiver_status.raw_value == expected_raw
            for field_name, expected_value in kwargs.items():
                assert getattr(h.receiver_status, field_name) == expected_value

    class TestEquality:
        """RecieverStatus equality semantics: only compares equal to another RecieverStatus with the same raw value."""

        @pytest.mark.parametrize('a,b', [
            (oem.RecieverStatus(), oem.RecieverStatus()),
            (oem.RecieverStatus(reciever_error=True), oem.RecieverStatus(reciever_error=True)),
            (oem.RecieverStatus(auxiliary_1_event=True, jammer_detected=True),
             oem.RecieverStatus(auxiliary_1_event=True, jammer_detected=True)),
        ])
        def test_equal_when_fields_match(self, a: oem.RecieverStatus, b: oem.RecieverStatus):
            assert a == b
            assert not a != b

        @pytest.mark.parametrize('a,b', [
            (oem.RecieverStatus(), oem.RecieverStatus(reciever_error=True)),
            (oem.RecieverStatus(temperature_warning=True), oem.RecieverStatus(temperature_warning=False)),
            (oem.RecieverStatus(jammer_detected=True), oem.RecieverStatus(spoofing_detected=True)),
        ])
        def test_unequal_when_any_field_differs(self, a: oem.RecieverStatus, b: oem.RecieverStatus):
            assert a != b
            assert not a == b

        @pytest.mark.parametrize('other', [0, 0x42, None, 'foo', [1, 2, 3], object()])
        def test_non_recieverstatus_compares_false(self, other):
            rs = oem.RecieverStatus()
            assert (rs == other) is False
            assert (rs != other) is True


class TestMessageType:
    """Round-trip MessageType values through the header property."""

    @pytest.mark.parametrize('value', [0, 0x1, 0x42, 0xFF])
    def test_int_round_trip(self, value: int):
        h = oem.Header(message_type=value)
        assert h.message_type.raw_value == value

    @pytest.mark.parametrize('value', [0, 0x1, 0x42, 0xFF])
    def test_positional_int_construct(self, value: int):
        mt = oem.MessageType(value)
        assert mt.raw_value == value

    @pytest.mark.parametrize('is_response,msg_format,sibling_id', [
        (False, MESSAGE_FORMAT.BINARY, 0),
        (True, MESSAGE_FORMAT.ASCII, 5),
        (False, MESSAGE_FORMAT.ABBREV, 0x1F),
        (True, MESSAGE_FORMAT.RSRVD, 0x10),
    ])
    class TestFieldConstruction:
        """Build a MessageType from its decoded fields and verify each path."""

        def test_construct(self, is_response: bool, msg_format: MESSAGE_FORMAT, sibling_id: int):
            mt = oem.MessageType(is_response=is_response, format=msg_format, sibling_id=sibling_id)
            assert mt.is_response == is_response
            assert mt.format == msg_format
            assert mt.sibling_id == sibling_id

        def test_header_round_trip(self, is_response: bool, msg_format: MESSAGE_FORMAT, sibling_id: int):
            mt = oem.MessageType(is_response=is_response, format=msg_format, sibling_id=sibling_id)
            h = oem.Header(message_type=mt)
            assert h.message_type.is_response == is_response
            assert h.message_type.format == msg_format
            assert h.message_type.sibling_id == sibling_id
            assert h.message_type.raw_value == mt.raw_value

    class TestEquality:
        """MessageType equality semantics: only compares equal to another MessageType with the same raw byte."""

        @pytest.mark.parametrize('a,b', [
            (oem.MessageType(), oem.MessageType()),
            (oem.MessageType(is_response=True, format=MESSAGE_FORMAT.ASCII, sibling_id=5),
             oem.MessageType(is_response=True, format=MESSAGE_FORMAT.ASCII, sibling_id=5)),
            (oem.MessageType(sibling_id=0x1F), oem.MessageType(sibling_id=0x1F)),
        ])
        def test_equal_when_fields_match(self, a: oem.MessageType, b: oem.MessageType):
            assert a == b
            assert not a != b

        @pytest.mark.parametrize('a,b', [
            (oem.MessageType(), oem.MessageType(is_response=True)),
            (oem.MessageType(format=MESSAGE_FORMAT.ASCII), oem.MessageType(format=MESSAGE_FORMAT.BINARY)),
            (oem.MessageType(sibling_id=1), oem.MessageType(sibling_id=2)),
        ])
        def test_unequal_when_any_field_differs(self, a: oem.MessageType, b: oem.MessageType):
            assert a != b
            assert not a == b

        @pytest.mark.parametrize('other', [0, 0x42, None, 'foo', [1, 2, 3], object()])
        def test_non_messagetype_compares_false(self, other):
            mt = oem.MessageType()
            assert (mt == other) is False
            assert (mt != other) is True
