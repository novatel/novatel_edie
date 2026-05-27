"""Tests for in-Python construction of OEM message headers."""

import novatel_edie.oem as oem


def test_default_construct_zero_arg():
    h = oem.Header()
    assert h.week == 0
    assert h.milliseconds == 0.0
    assert h.message_id == 0


def test_kw_construct_round_trip():
    h = oem.Header(
        message_id=42,
        week=2300,
        milliseconds=345600.0,
        port_address=0xC0,
        sequence=7,
        idle_time=10,
        receiver_sw_version=1234,
    )
    assert h.message_id == 42
    assert h.week == 2300
    assert h.milliseconds == 345600.0
    assert h.port_address == 0xC0
    assert h.sequence == 7
    assert h.idle_time == 10
    assert h.receiver_sw_version == 1234
