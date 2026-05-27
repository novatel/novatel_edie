"""Smoke tests for in-Python construction and mutation of real OEM messages.

Scope:
 - Message construction with optional header + scalar/enum/string kwargs
 - setattr on SIMPLE, ENUM, STRING fields after construction
 - Encode/decode round-trip after Python-side mutation
 - Variable-length array setter on a real message (PASSCOM1)
 - Array-specific validation on a real message (RANGE)
 - General validation (unknown kwarg / unknown setattr / type mismatch) on
   a real message (BESTPOS)
"""

import pytest

import novatel_edie.oem as oem
from novatel_edie import ENCODE_FORMAT
from novatel_edie.oem.messages import BESTPOS, BESTPOS_B1F6, RANGE, PASSCOM1
from novatel_edie.oem.enums import SolStatus, SolType, Datum


class TestMessageConstruction:
    """Construct real OEM messages from scratch with kwargs."""

    def test_scalar_kwargs_only(self):
        m = BESTPOS_B1F6(latitude=45.5, longitude=-122.3)
        assert m.latitude == 45.5
        assert m.longitude == -122.3
        # __init__ overrides these two header fields from the message definition,
        # regardless of what the caller passed.
        assert m.header.message_id == 42  # BESTPOS logID
        assert m.header.message_definition_crc == 0xB1F6

    def test_with_header_and_scalar_kwargs(self):
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

    def test_bestpos_full(self):
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


class TestSetattr:
    """Assign fields on real messages after construction."""

    def test_simple_field(self):
        m = BESTPOS()
        m.latitude = 12.34
        m.longitude = -56.78
        assert m.latitude == 12.34
        assert m.longitude == -56.78

    def test_enum_field(self):
        m = BESTPOS()
        m.solution_status = 3  # SolStatus.SOL_COMPUTED
        assert m.solution_status == SolStatus.SINGULARITY


class TestRoundTrip:
    """Verify Python-side mutations survive encode + decode."""

    def test_setattr_then_encode_then_decode(self, decoders_test_resources):
        # Build a message in Python, encode it, decode it back, confirm the field
        # we set survived the trip.
        m = BESTPOS(latitude=49.123, longitude=-123.456, orthometric_height=42.5)
        encoded = m.encode(ENCODE_FORMAT.ASCII)
        raw = bytes(encoded.message)
        parser = oem.Parser()
        parser.write(raw)
        parsed = parser.read()
        assert isinstance(parsed, BESTPOS)
        assert parsed is not None
        assert parsed.latitude == pytest.approx(49.123, abs=1e-6)
        assert parsed.longitude == pytest.approx(-123.456, abs=1e-6)
        assert parsed.orthometric_height == pytest.approx(42.5, abs=1e-6)


class TestVariableLengthArray:
    """Variable-length array setter on PASSCOM1."""

    @pytest.mark.parametrize('value,exp_value', [
        pytest.param(
            '#BESTPOSA,COM3,0,80.0,FINESTEERING,1337,400920.000,02000000,4ca6,1899;SOL_COMPUT',
            list(b'#BESTPOSA,COM3,0,80.0,FINESTEERING,1337,400920.000,02000000,4ca6,1899;SOL_COMPUT'),
            id='partial_bestpos')
        ])
    def test_set_variable_length_array(self, value, exp_value):
        # Arrange
        m = PASSCOM1()
        # Act
        m.buffer = value
        # Assert
        assert m.buffer == exp_value


class TestArrayValidation:
    """Array/length-field rejections on a real message (RANGE)."""

    def test_field_array_setattr_rejected(self):
        r = RANGE()
        with pytest.raises(TypeError, match='FieldArray'):
            r.obs = [1, 2, 3]

    def test_length_field_write_rejected(self):
        r = RANGE()
        with pytest.raises(AttributeError, match='Length cannot be set directly'):
            r.obs_length = 5


class TestGeneralValidation:
    """Message-shape-agnostic rejections on a real message (BESTPOS)."""

    def test_unknown_kwarg_rejected(self):
        with pytest.raises(AttributeError, match='this_is_not_a_field'):
            BESTPOS(this_is_not_a_field=1)

    def test_unknown_setattr_rejected(self):
        m = BESTPOS()
        with pytest.raises(AttributeError, match='does_not_exist'):
            m.does_not_exist = 1

    def test_setattr_type_mismatch_raises(self):
        m = BESTPOS()
        with pytest.raises(TypeError):
            m.latitude = 'not a number'
