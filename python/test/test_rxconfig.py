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
#! \brief Unit Tests for decoding and encoding of RxConfig messages.
################################################################################

import novatel_edie as ne
from novatel_edie import messages as ne_msgs
from novatel_edie import enums as ne_enums
from novatel_edie import STATUS, ENCODE_FORMAT
import pytest

@pytest.fixture
def decoder():
    return ne.Decoder()


def test_rxconfig_decoding(decoder: ne.Decoder):
    """Test that the payload of an RXCONFIG message can be converted to bytes and decoded."""
    # Arrange
    raw_message = b"#RXCONFIGA,COM1,0,54.0,FINESTEERING,2172,155744.316,02010000,f702,16248;#INTERFACEMODEA,COM1,0,54.0,FINESTEERING,2172,155744.316,02010000,f702,16248;COM1,NOVATEL,NOVATEL,ON*ca0f5c51*71be1427\r\n"

    # Act
    decoded_message: ne_msgs.RXCONFIG = decoder.decode(raw_message)
    decoded_message_data = bytes(decoded_message.embedded_message_data)
    decoded_embedded_message: ne_msgs.INTERFACEMODE = decoder.decode(decoded_message_data)

    # Assert
    assert decoded_embedded_message.name == "INTERFACEMODE"
    assert decoded_embedded_message.port == ne_enums.CommPort.COM1
    assert decoded_embedded_message.responses == ne_enums.OnOff.ON
