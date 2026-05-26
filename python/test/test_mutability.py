################################################################################
#
# Tests for in-Python construction and mutation of OEM messages.
#
# Scope (matches what is actually implemented):
#  - Header zero-arg and kw construction
#  - Message construction with optional header + scalar/enum/string kwargs
#  - setattr on SIMPLE, ENUM, STRING fields after construction
#  - Refusal of array kwargs / array setattr / length-field writes / unknown
#    attribute names
#
################################################################################

import pytest

import novatel_edie as ne
import novatel_edie.oem as oem
from novatel_edie import ENCODE_FORMAT
from novatel_edie.oem.messages import BESTPOS, BESTPOS_B1F6, LOG, RANGE, PASSCOM1
from novatel_edie.oem.enums import SolStatus, SolType, Datum


# -----------------------------------------------------------------------------
# Positive cases
# -----------------------------------------------------------------------------

def test_header_default_construct_zero_arg():
    h = oem.Header()
    assert h.week == 0
    assert h.milliseconds == 0.0
    assert h.message_id == 0


def test_header_kw_construct_round_trip():
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


def test_message_construct_scalar_kwargs_only():
    m = BESTPOS_B1F6(latitude=45.5, longitude=-122.3)
    assert m.latitude == 45.5
    assert m.longitude == -122.3
    # __init__ overrides these two header fields from the message definition,
    # regardless of what the caller passed.
    assert m.header.message_id == 42  # BESTPOS logID
    assert m.header.message_definition_crc == 0xB1F6


def test_message_construct_with_header_and_scalar_kwargs():
    h = oem.Header(week=2300, milliseconds=345600.0)
    m = BESTPOS(header=h, latitude=99.0)
    # Caller-supplied header fields survive.
    assert m.header.week == 2300
    assert m.header.milliseconds == 345600.0
    # Field value applied.
    assert m.latitude == 99.0
    # Two header fields are always overridden by __init__.
    assert m.header.message_id == 42
    assert m.header.message_definition_crc != 0


def test_setattr_on_simple_field_after_construct():
    m = BESTPOS()
    m.latitude = 12.34
    m.longitude = -56.78
    assert m.latitude == 12.34
    assert m.longitude == -56.78


def test_setattr_on_enum_field():
    m = BESTPOS()
    m.solution_status = 3  # SolStatus.SOL_COMPUTED
    assert m.solution_status == SolStatus.SINGULARITY


def test_round_trip_setattr_then_encode_then_decode(decoders_test_resources):
    # Build a message in Python, encode it, decode it back, confirm the field
    # we set survived the trip.
    m = BESTPOS(latitude=49.123, longitude=-123.456, orthometric_height=42.5)
    encoded = m.encode(ENCODE_FORMAT.ASCII)
    # encoded is PyMessageData; pull out the message bytes via the buffer attr.
    raw = bytes(encoded.message)
    # Decode the raw output back through a Parser.
    parser = oem.Parser()
    parser.write(raw)
    parsed = parser.read()
    assert isinstance(parsed, BESTPOS)
    assert parsed is not None
    assert parsed.latitude == pytest.approx(49.123, abs=1e-6)
    assert parsed.longitude == pytest.approx(-123.456, abs=1e-6)
    assert parsed.orthometric_height == pytest.approx(42.5, abs=1e-6)

def test_bestpos_full():
    # Act
    m = BESTPOS(
        header=None,
        solution_status=SolStatus.COLD_START,
        position_type=SolType.FIXEDPOS,
        latitude=12.1,
        longitude=32.3,
        orthometric_height=12,
        undulation=10,
        datum_id=Datum.WGS84,
        latitude_std_dev=1.5,
        longitude_std_dev=1.5,
        height_std_dev=2.0,
        base_id='AB12',
        diff_age=3.0,
        solution_age=4.0,
        num_svs=12,
        num_soln_svs=10,
        num_soln_L1_svs=8,
        num_soln_multi_svs=6,
        extended_solution_status2=0,
        ext_sol_stat=0,
        gal_and_bds_mask=0,
        gps_and_glo_mask=0
    )
    # Assert
    assert m.solution_status == SolStatus.COLD_START
    assert m.position_type == SolType.FIXEDPOS
    assert m.latitude == 12.1
    assert m.longitude == 32.3
    assert m.orthometric_height == 12
    assert m.undulation == 10
    assert m.datum_id == Datum.WGS84
    assert m.latitude_std_dev == 1.5
    assert m.longitude_std_dev == 1.5
    assert m.height_std_dev == 2.0
    assert m.base_id == 'AB12'
    assert m.diff_age == 3.0
    assert m.solution_age == 4.0
    assert m.num_svs == 12
    assert m.num_soln_svs == 10
    assert m.num_soln_L1_svs == 8
    assert m.num_soln_multi_svs == 6
    assert m.extended_solution_status2 == 0
    assert m.ext_sol_stat == 0
    assert m.gal_and_bds_mask == 0
    assert m.gps_and_glo_mask == 0

@pytest.mark.parametrize('value', [
    b'AB12', b'CD34', b'EF56'
])
class TestArraySetters:
    """Test setting a simple array with all support values types."""
    def test_bytes_setter(self, value: bytes):
        # Arrange
        m = BESTPOS()
        set_value = value
        exp_value = value.decode('ascii')
        # Act
        m.base_id = set_value
        # Assert
        assert m.base_id == exp_value

    def test_string_setter(self, value: bytes):
       # Arrange
        m = BESTPOS()
        set_value = value.decode('ascii')
        exp_value = value.decode('ascii')
        # Act
        m.base_id = set_value
        # Assert
        assert m.base_id == exp_value

    def test_list_setter(self, value: bytes):
       # Arrange
        m = BESTPOS()
        set_value = list(value)
        exp_value = value.decode('ascii')
        # Act
        m.base_id = set_value
        # Assert
        assert m.base_id == exp_value



@pytest.mark.parametrize('value,exp_value', [
    pytest.param(
        '#BESTPOSA,COM3,0,80.0,FINESTEERING,1337,400920.000,02000000,4ca6,1899;SOL_COMPUT',
        list(b'#BESTPOSA,COM3,0,80.0,FINESTEERING,1337,400920.000,02000000,4ca6,1899;SOL_COMPUT'),
        id='partial_bestpos')
    ])
def test_set_variable_length_array(value, exp_value):
    # Arrange
    m = PASSCOM1()
    # Act
    m.buffer = value
    # Assert
    assert m.buffer == exp_value


# -----------------------------------------------------------------------------
# Negative cases
# -----------------------------------------------------------------------------

def test_unknown_kwarg_rejected():
    with pytest.raises(AttributeError) as exc:
        BESTPOS(this_is_not_a_field=1)
    assert "this_is_not_a_field" in str(exc.value)


def test_unknown_setattr_rejected():
    m = BESTPOS()
    with pytest.raises(AttributeError) as exc:
        m.does_not_exist = 1
    assert "does_not_exist" in str(exc.value)


def test_field_array_setattr_rejected():
    # RANGE has a FIELD_ARRAY field `obs` — assigning to it should fail with
    # the unsupported-FIELD_ARRAY message.
    r = RANGE()
    with pytest.raises(AttributeError) as exc:
        r.obs = [1, 2, 3]
    assert "FIELD_ARRAY" in str(exc.value)


def test_length_field_write_rejected():
    # _length attributes are synthesized for FIELD_ARRAY / VARIABLE_LENGTH_ARRAY
    # fields. RANGE.obs is a FIELD_ARRAY, so RANGE has an obs_length entry.
    r = RANGE()
    with pytest.raises(AttributeError) as exc:
        r.obs_length = 5
    assert "Length cannot be set directly" in str(exc.value)


def test_setattr_type_mismatch_raises():
    # Passing a non-castable value (a string for a float field) should raise
    # AttributeError (wrapped from the underlying cast failure).
    m = BESTPOS()
    with pytest.raises(TypeError):
        m.latitude = "not a number"
