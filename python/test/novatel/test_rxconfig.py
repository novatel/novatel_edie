################################################################################
#
# COPYRIGHT Novatel Inc, 2021. All rights reserved.
#
# No part of this software may be reproduced or modified in any form
# or by any means - electronic, mechanical, photocopying, recording,
# or otherwise - without the prior written consent of NovAtel Inc.
#
################################################################################
#                            DESCRIPTION
#
#! \file rxconfigtest.hpp
#! \brief Unit Tests for RxConfigHandler.
################################################################################


import novatel_edie as ne
from novatel_edie import HEADERFORMAT, STATUS, ENCODEFORMAT
import pytest


@pytest.fixture(scope="function")
def rx_config_handler():
    return ne.RxConfigHandler()


def compare_message_data(test_message_data, expected_message_data):
    result = True
    if test_message_data.message != expected_message_data[0]:
        print("MessageData.message contents do not match")
        result = False
    if test_message_data.header != expected_message_data[1]:
        print("MessageData.message_header contents do not match")
        result = False
    if test_message_data.body != expected_message_data[2]:
        print("MessageData.message_body contents do not match")
        result = False
    return result


def write_bytes_to_handler(handler, data):
    assert handler.write(data) == len(data)


def TestSameFormatCompare(rx_config_handler, format_, expected_rx_config_message_data, expected_embedded_message_data):
    (status, test_rx_config_message_data, test_rx_config_meta_data,
     test_embedded_message_data, test_embedded_meta_data) = rx_config_handler.convert(format_)
    if status != STATUS.SUCCESS:
        print(f"Convert failed with code {status}")
        return False
    if (not compare_message_data(test_rx_config_message_data, expected_rx_config_message_data) or
            not compare_message_data(test_embedded_message_data, expected_embedded_message_data)):
        return False
    return True

# -------------------------------------------------------------------------------------------------------
# Logging Framer Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_LOGGER():
    name = "rxconfig_handler"
    level = ne.LogLevel.OFF
    logger = ne.RxConfigHandler().logger
    logger.set_level(level)
    assert logger.name == name
    assert logger.level == level
    assert ne.Logging.get(name) is not None

# -------------------------------------------------------------------------------------------------------
# Round-trip unit tests.
# -------------------------------------------------------------------------------------------------------
def test_RXCONFIG_ROUNDTRIP_ASCII(rx_config_handler):
    log = b"#RXCONFIGA,COM1,0,54.0,FINESTEERING,2172,155744.316,02010000,f702,16248;#INTERFACEMODEA,COM1,0,54.0,FINESTEERING,2172,155744.316,02010000,f702,16248;COM1,NOVATEL,NOVATEL,ON*ca0f5c51*71be1427\r\n"
    expected_rx_config_message_data = (log[:192], log[:72], log[72:][:120])
    expected_embedded_message_data = (log[72:][:109], log[72:][:77], log[149:][:32])
    write_bytes_to_handler(rx_config_handler, log)
    assert TestSameFormatCompare(rx_config_handler, ENCODEFORMAT.ASCII, expected_rx_config_message_data, expected_embedded_message_data)

def test_RXCONFIG_ROUNDTRIP_ABBREV(rx_config_handler):
    # NOTE: This RXCONFIG message is NOT what an OEM7 receiver would produce.  The space after the embedded header is added intentionally by RxConfigHandler to make it decodeable.
    log = b"<RXCONFIG COM2 78 33.0 UNKNOWN 0 12.468 024c0000 f702 32768\r\n<     PDPFILTER COM2 78 33.0 UNKNOWN 0 12.468 024c0000 dab4 32768 \r\n<     DISABLE\r\nMore bytes..."
    expected_rx_config_message_data = (log[:144], log[:61], log[61:][:83])
    expected_embedded_message_data = (log[61:][:83], log[61:][:68], log[129:][:15])
    write_bytes_to_handler(rx_config_handler, log)
    assert TestSameFormatCompare(rx_config_handler, ENCODEFORMAT.ABBREV_ASCII, expected_rx_config_message_data, expected_embedded_message_data)

def test_RXCONFIG_ROUNDTRIP_BINARY(rx_config_handler):
   # RXCONFIG
    log = bytes([0xAA, 0x44, 0x12, 0x1C, 0x80, 0x00, 0x00, 0x20, 0x30, 0x00, 0x00, 0x00, 0x65, 0xB4, 0x7C, 0x08, 0x3C, 0x78, 0x48, 0x09, 0x00, 0x00, 0x01, 0x02, 0x02, 0xF7, 0x78, 0x3F, 0xAA, 0x44, 0x12, 0x1C, 0x03, 0x00, 0x00, 0x20, 0x10, 0x00, 0x00, 0x00, 0x65, 0xB4, 0x7C, 0x08, 0x3C, 0x78, 0x48, 0x09, 0x00, 0x00, 0x01, 0x02, 0x02, 0xF7, 0x78, 0x3F, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x67, 0x74, 0xB2, 0xEC, 0x0E, 0xD1, 0xFB, 0x06])
    expected_rx_config_message_data = (log[:80], log[:ne.OEM4_BINARY_HEADER_LENGTH], log[ne.OEM4_BINARY_HEADER_LENGTH:][:52])
    expected_embedded_message_data = (log[ne.OEM4_BINARY_HEADER_LENGTH:][:48], log[ne.OEM4_BINARY_HEADER_LENGTH:][:ne.OEM4_BINARY_HEADER_LENGTH], log[ne.OEM4_BINARY_HEADER_LENGTH*2:][:20])
    write_bytes_to_handler(rx_config_handler, log)
    assert TestSameFormatCompare(rx_config_handler, ENCODEFORMAT.BINARY, expected_rx_config_message_data, expected_embedded_message_data)


# -------------------------------------------------------------------------------------------------------
# Conversion to JSON unit tests.
# -------------------------------------------------------------------------------------------------------
def test_RXCONFIG_CONVERT_ASCII_TO_JSON(rx_config_handler):
    log = b"#RXCONFIGA,COM2,235,77.0,UNKNOWN,0,0.727,02000020,f702,17002;#ADJUST1PPSA,COM2,235,77.0,UNKNOWN,0,0.727,02000020,f702,17002;OFF*4c2dbb6d*1600a42a\r\n"
    json_log = b"""{"header":{"message": "RXCONFIG","id": 128,"port": "COM2","sequence_num": 235,"percent_idle_time": 77.0,"time_status": "UNKNOWN","week": 0,"seconds": 0.727,"receiver_status": 33554464,"HEADER_reserved1": 63234,"receiver_sw_version": 17002},"body":{"embedded_header":{"message": "ADJUST1PPS","id": 429,"port": "COM2","sequence_num": 235,"percent_idle_time": 77.0,"time_status": "UNKNOWN","week": 0,"seconds": 0.727,"receiver_status": 33554464,"HEADER_reserved1": 63234,"receiver_sw_version": 17002},"embedded_body":{"mode": "OFF"}}}"""
    expected_rx_config_message_data = (json_log[0:][:531], json_log[10:][:229], json_log[247:][:283])
    expected_embedded_message_data = (json_log[247:][:283], json_log[266:][:231], json_log[514:][:15])
    write_bytes_to_handler(rx_config_handler, log)
    assert TestSameFormatCompare(rx_config_handler, ENCODEFORMAT.JSON, expected_rx_config_message_data, expected_embedded_message_data)

def test_RXCONFIG_CONVERT_ABBREV_TO_JSON(rx_config_handler):
    log = b"<RXCONFIG COM2 187 78.5 UNKNOWN 0 0.839 02000020 f702 17002\r\n<     SBASECUTOFF COM2 187 78.5 UNKNOWN 0 0.839 02000020 f702 17002 \r\n<     -5.0\r\n[PISSCOM1]"
    json_log = b"""{"header":{"message": "RXCONFIG","id": 128,"port": "COM2","sequence_num": 187,"percent_idle_time": 78.5,"time_status": "UNKNOWN","week": 0,"seconds": 0.839,"receiver_status": 33554464,"HEADER_reserved1": 63234,"receiver_sw_version": 17002},"body":{"embedded_header":{"message": "SBASECUTOFF","id": 1000,"port": "COM2","sequence_num": 187,"percent_idle_time": 78.5,"time_status": "UNKNOWN","week": 0,"seconds": 0.839,"receiver_status": 33554464,"HEADER_reserved1": 63234,"receiver_sw_version": 17002},"embedded_body":{"elevation_cutoff_angle": -5.0}}}"""
    expected_rx_config_message_data = (json_log[0:][:550], json_log[10:][:229], json_log[247:][:302])
    expected_embedded_message_data = (json_log[247:][:302], json_log[266:][:233], json_log[516:][:32])
    write_bytes_to_handler(rx_config_handler, log)
    assert TestSameFormatCompare(rx_config_handler, ENCODEFORMAT.JSON, expected_rx_config_message_data, expected_embedded_message_data)

def test_RXCONFIG_CONVERT_BINARY_TO_JSON(rx_config_handler):
    log = bytes([0xAA, 0x44, 0x12, 0x1C, 0x80, 0x00, 0x00, 0x40, 0x3C, 0x00, 0x00, 0x00, 0x9C, 0xB4, 0xBB, 0x08, 0x47, 0x74, 0x6A, 0x18, 0x20, 0x00, 0x00, 0x02, 0x02, 0xF7, 0x6A, 0x42, 0xAA, 0x44, 0x12, 0x1C, 0xDE, 0x04, 0x00, 0x40, 0x1C, 0x00, 0x00, 0x00, 0x9C, 0xB4, 0xBB, 0x08, 0x47, 0x74, 0x6A, 0x18, 0x20, 0x00, 0x00, 0x02, 0x02, 0xF7, 0x6A, 0x42, 0x02, 0x00, 0x00, 0x00, 0x00, 0x08, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xF4, 0xD7, 0x46, 0x65, 0x5D, 0x3D, 0xED, 0xF6])
    json_log = b"""{"header":{"message": "RXCONFIG","id": 128,"port": "COM2","sequence_num": 0,"percent_idle_time": 78.0,"time_status": "FINESTEERING","week": 2235,"seconds": 409629.767,"receiver_status": 33554464,"HEADER_reserved1": 63234,"receiver_sw_version": 17002},"body":{"embedded_header":{"message": "SERIALCONFIG","id": 1246,"port": "COM2","sequence_num": 0,"percent_idle_time": 78.0,"time_status": "FINESTEERING","week": 2235,"seconds": 409629.767,"receiver_status": 33554464,"HEADER_reserved1": 63234,"receiver_sw_version": 17002},"embedded_body":{"port": "COM2","baud_rate": 460800,"parity": "N","data_bits": 8,"stop_bits": 1,"hand_shaking": "N","breaks": "ON"}}}"""
    expected_rx_config_message_data = (json_log[0:][:656], json_log[10:][:240], json_log[258:][:397])
    expected_embedded_message_data = (json_log[258:][:397], json_log[277:][:245], json_log[539:][:115])
    write_bytes_to_handler(rx_config_handler, log)
    assert TestSameFormatCompare(rx_config_handler, ENCODEFORMAT.JSON, expected_rx_config_message_data, expected_embedded_message_data)
