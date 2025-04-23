################################################################################
#
# COPYRIGHT NovAtel Inc, 2022. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
################################################################################
#                            DESCRIPTION
#
# \file novateltest.hpp
# \brief Unit tests focused on the intermediate format attained via decoding.
################################################################################

import pytest

import novatel_edie as ne

@pytest.fixture
def decoder():
    """Fixture for creating a decoder instance."""
    return ne.Decoder()


def test_field_names_and_values(decoder: ne.Decoder):
    """Test that the field names are correct."""
    # Arrange
    data = b"#BESTPOSA,COM1,0,60.5,FINESTEERING,2166,327153.000,02000000,b1f6,16248;SOL_COMPUTED,WAAS,51.15043699323,-114.03067932462,1096.9772,-17.0000,WGS84,0.6074,0.5792,0.9564,\"131\",7.000,0.000,42,34,34,28,00,0b,1f,37*47bbdc4f\r\n"
    exp_fields = [
        'solution_status', 'position_type', 'latitude', 'longitude', 'height', 'undulation',
        'datum_id', 'latitude_std_dev', 'longitude_std_dev', 'height_std_dev', 'base_id',
        'diff_age','solution_age', 'num_svs', 'num_soln_svs', 'num_soln_L1_svs',
        'num_soln_multi_svs', 'measurement_source', 'ext_sol_stat', 'gal_and_bds_mask',
        'gps_and_glo_mask']
    exp_values = [
        ne.enums.SolStatus.SOL_COMPUTED, ne.enums.SolType.WAAS, 51.15043699323,
        -114.03067932462, 1096.9772, -17.0, ne.enums.Datum.WGS84, 0.6074000000953674,
        0.579200029373169, 0.9563999772071838, '131', 7.0, 0.0, 42, 34, 34, 28, 0, 11, 31, 55]
    # Act
    msg = decoder.decode(data)
    fields = msg.get_fields()
    values = msg.get_values()
    # Assert
    assert fields == exp_fields, f"Expected fields: {exp_fields}, but got: {fields}"
    assert values == exp_values, f"Expected values: {exp_values}, but got: {values}"
