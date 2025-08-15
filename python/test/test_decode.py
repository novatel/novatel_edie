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

def nest_values(message: ne.Field):
    """Convert the values of field into a nested list structure.

    Args:
        message: A message or field to retrieve values from.
    Returns:
        list: A nested list of simple values.
    """
    values = message.get_field_values()
    for i, value in enumerate(values):
        if isinstance(value, ne.Field):
            values[i] = nest_values(value)
        elif isinstance(value, list):
            list_values = []
            for item in value:
                if isinstance(item, ne.Field):
                    list_values.append(nest_values(item))
                else:
                    list_values.append(item)
            values[i] = list_values
    return values

def compare_with_floating_point(item1, item2) -> bool:
    """Compare items for equality, allowing for floating point tolerance within lists.

    Args:
        item1: The first item to compare.
        item2: The second item to compare.

    Returns:
        bool: True if items are equal or within floating point tolerance, False otherwise.
    """
    if isinstance(item1, list) and isinstance(item2, list):
        if len(item1) != len(item2):
            return False
        for item1, item2 in zip(item1, item2):
            if not compare_with_floating_point(item1, item2):
                return False
        return True
    elif isinstance(item1, dict) and isinstance(item2, dict):
        if item1.keys() != item2.keys():
            return False
        for key in item1.keys():
            if not compare_with_floating_point(item1[key], item2[key]):
                return False
        return True
    elif isinstance(item1, float) and isinstance(item2, float):
        # Compare floating point numbers with a tolerance
        ret_val = abs(item1 - item2) < 1e-3
        return ret_val
    return item1 == item2


@pytest.mark.parametrize("data, exp_fields, exp_values", [
    pytest.param(
        b"#BESTPOSA,COM1,0,60.5,FINESTEERING,2166,327153.000,02000000,b1f6,16248;SOL_COMPUTED,WAAS,51.15043699323,-114.03067932462,1096.9772,-17.0000,WGS84,0.6074,0.5792,0.9564,\"131\",7.000,0.000,42,34,34,28,00,0b,1f,37*47bbdc4f\r\n",
        [
            'solution_status', 'position_type', 'latitude', 'longitude', 'height', 'undulation',
            'datum_id', 'latitude_std_dev', 'longitude_std_dev', 'height_std_dev', 'base_id',
            'diff_age','solution_age', 'num_svs', 'num_soln_svs', 'num_soln_L1_svs',
            'num_soln_multi_svs', 'measurement_source', 'ext_sol_stat', 'gal_and_bds_mask',
            'gps_and_glo_mask'
        ],
        [
            ne.enums.SolStatus.SOL_COMPUTED,
            ne.enums.SolType.WAAS,
            51.15043699323,
            -114.03067932462,
            1096.9772,
            -17.0,
            ne.enums.Datum.WGS84,
            0.6074000000953674,
            0.579200029373169,
            0.9563999772071838,
            '131',
            7.0,
            0.0,
            42,
            34,
            34,
            28,
            0,
            11,
            31,
            55
        ],
        id="BESTPOS"
    ),
    pytest.param(
        b'#RANGECMP2A,USB1,0,66.5,FINESTEERING,2378,227093.000,03000020,1fe3,32768;5,ffffffffff*58bf791d',
        ['range_data_length', 'range_data'],
        [
            5,
            [0xFF, 0xFF, 0xFF, 0xFF, 0xFF]
        ],
        id="RANGECMP2"
    ),
    pytest.param(
        b'#RANGEA,USB1,0,66.0,FINESTEERING,2378,230219.000,03000020,5103,32768;1,27,0,24627698.557,0.246,-129419430.405069,0.014,3666.953,42.6,344.455,0810dc04*1d271f23',
        ['obs_length', 'obs'],
        [
            1,
            [[27, 0, 24627698.557, 0.246, -129419430.405069, 0.014, 3666.953, 42.6, 344.455, 135322628]]
        ],
        id="RANGEA"
    )
])
def test_field_names_and_values(
        data: bytes, exp_fields: list, exp_values: list, decoder: ne.Decoder):
    """Test that the field names are correct."""
    # Act
    msg = decoder.decode(data)
    fields = msg.get_field_names()
    values = nest_values(msg)
    # Assert
    assert fields == exp_fields, f"Expected fields: {exp_fields}, but got: {fields}"
    assert compare_with_floating_point(values, exp_values), f"Expected values: {exp_values}, but got: {values}"


@pytest.mark.parametrize("data, exp_dict", [
    (
        b'#SATVIS2A,USB1,1,64.0,FINESTEERING,2379,485000.000,02000020,a867,32768;QZSS,TRUE,TRUE,5,194,1,13.5,306.6,142.865,142.827,195,1,-19.6,268.0,-241.672,-241.710,199,1,-25.6,293.3,-3.932,-3.970,200,16,-41.5,329.5,-3.242,-3.280,196,16,-48.1,278.4,327.067,327.029*c8e214c6\r\n',
        {'system_type': ne.enums.SystemType.QZSS,
         'is_sat_vis_valid': True,
         'was_gnss_almanac_used': True,
         'sat_vis_list_length': 5,
         'sat_vis_list': [
             {'id': {'prn_or_slot': 194, 'frequency_channel': 0}, 'sat_health': 1, 'elevation': 13.5, 'azimuth': 306.6, 'true_doppler': 142.865, 'apparent_doppler': 142.827},
             {'id': {'prn_or_slot': 195, 'frequency_channel': 0}, 'sat_health': 1, 'elevation': -19.6, 'azimuth': 268.0, 'true_doppler': -241.672, 'apparent_doppler': -241.71},
             {'id': {'prn_or_slot': 199, 'frequency_channel': 0}, 'sat_health': 1, 'elevation': -25.6, 'azimuth': 293.3, 'true_doppler': -3.932, 'apparent_doppler': -3.97},
             {'id': {'prn_or_slot': 200, 'frequency_channel': 0}, 'sat_health': 16, 'elevation': -41.5, 'azimuth': 329.5, 'true_doppler': -3.242, 'apparent_doppler': -3.28},
             {'id': {'prn_or_slot': 196, 'frequency_channel': 0}, 'sat_health': 16, 'elevation': -48.1, 'azimuth': 278.4, 'true_doppler': 327.067, 'apparent_doppler': 327.029}
         ],
         'header': {
             'message_id': 1043,
             'message_type': 32,
             'port_address': 1440,
             'length': 0,
             'sequence': 1,
             'idle_time': 128,
             'time_status': ne.TIME_STATUS.FINESTEERING,
             'week': 2379,
             'milliseconds': 485000000.000,
             'receiver_status': 33554464,
             'message_definition_crc': 43111,
             'receiver_sw_version': 32768
         }
        }
    )
])
def test_to_dict(data: bytes, exp_dict: dict, decoder: ne.Decoder):
    """Test that the to_dict method works correctly."""
    # Act
    msg = decoder.decode(data)
    dict_msg = msg.to_dict()
    # Assert
    assert compare_with_floating_point(dict_msg, exp_dict), f"Expected dict: {exp_dict}, but got: {dict_msg}"
