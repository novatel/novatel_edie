import novatel_edie as ne
import pytest
from novatel_edie import HEADER_FORMAT, STATUS



class Helper:
    def __init__(self, test_resources_path):
        self.test_resources = test_resources_path
        self.framer_manager = ne.FramerManager()
        self.framer_manager.report_unknown_bytes = True
        self.framer_manager.payload_only = False

    def test_framer_errors(self, expected_error, buffer_size=ne.MAX_MESSAGE_LENGTH):
        pass
        with pytest.raises(expected_error):
            self.framer.get_frame(buffer_size)

    def test_framer(self, expected_header_format, length, buffer_size=ne.MAX_MESSAGE_LENGTH, response=False):
        # Arrange
        expected_meta_data = ne.MetaData()
        if length is not None:
            expected_meta_data.length = length
        expected_meta_data.response = response
        expected_meta_data.format = expected_header_format
        # Act
        _, test_meta_data = self.framer.get_frame(buffer_size)

        # Assert
        compare_metadata(test_meta_data, expected_meta_data, ignore_length=length is None)

    def get_file_contents(self, filename):
        return (self.test_resources / filename).read_bytes()

    def write_bytes_to_framer(self, data):
        assert self.framer.write(data) == len(data)

    def write_file_to_framer(self, filename):
        data = self.get_file_contents(filename)
        self.write_bytes_to_framer(data)


@pytest.fixture(scope="function")
def helper(decoders_test_resources):
    return Helper(decoders_test_resources)

def compare_metadata(test_md, expected_md, ignore_length=False):
    assert test_md.format == expected_md.format, "MetaData format mismatch"
    assert test_md.measurement_source == expected_md.measurement_source, "MetaData measurement_source mismatch"
    assert test_md.time_status == expected_md.time_status, "MetaData time_status mismatch"
    assert test_md.response == expected_md.response, "MetaData response mismatch"
    assert test_md.week == expected_md.week, "MetaData week mismatch"
    assert test_md.milliseconds == expected_md.milliseconds, "MetaData milliseconds mismatch"
    assert test_md.binary_msg_length == expected_md.binary_msg_length, "MetaData binary_msg_length mismatch"
    if not ignore_length:
        assert test_md.length == expected_md.length, "MetaData length mismatch"
    assert test_md.header_length == expected_md.header_length, "MetaData header_length mismatch"
    assert test_md.message_id == expected_md.message_id, "MetaData message_id mismatch"
    assert test_md.message_crc == expected_md.message_crc, "MetaData message_crc mismatch"
    assert test_md.message_name == expected_md.message_name, "MetaData message_name mismatch"


# -------------------------------------------------------------------------------------------------------
# ASCII Framer Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_ascii_complete(helper):
    data = b"#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.ASCII, len(data))
