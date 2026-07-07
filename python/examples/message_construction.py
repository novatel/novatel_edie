#!/usr/bin/env python3
########################################################################
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
########################################################################
#                            DESCRIPTION
#
# \file message_construction.py
# \brief Demonstrate how to construct and mutate Message objects from
# Python, then encode the result.
########################################################################

import novatel_edie as ne
import novatel_edie.oem as oem
import novatel_edie.oem.messages as oem_msgs
import novatel_edie.oem.enums as oem_enums


def main():
    print(f"Decoder library information:\n{ne.CPP_PRETTY_VERSION}")

    # Construct a message by passing its fields as keyword arguments.
    # Fields that are omitted take type-appropriate defaults (0, "", etc.).
    bestpos = oem_msgs.BESTPOS(
        solution_status=oem_enums.SolStatus.SOL_COMPUTED,
        position_type=oem_enums.SolType.SINGLE,
        latitude=51.15043706870,
        longitude=-114.03067882331,
        orthometric_height=1097.3462,
    )
    print(f"Constructed message: {bestpos.to_dict()}")

    # A message's header can be supplied explicitly. Any fields not passed
    # to Header() take type-appropriate defaults.
    header = oem.Header(week=2300, milliseconds=345600.0)
    bestpos_with_header = oem_msgs.BESTPOS(header=header, latitude=51.0)
    print(
        f"Header week: {bestpos_with_header.header.week}, "
        f"milliseconds: {bestpos_with_header.header.milliseconds}"
    )
    # message_id and message_definition_crc are always taken from the message
    # definition, regardless of what the supplied header contains, so a
    # constructed message always identifies as itself.
    print(
        f"message_id: {bestpos_with_header.header.message_id}, "
        f"message_definition_crc: {bestpos_with_header.header.message_definition_crc}"
    )

    # Fields can also be mutated after construction via setattr.
    bestpos.latitude = 51.16
    bestpos.longitude = -114.04
    print(f"Mutated position: (lat={bestpos.latitude}, long={bestpos.longitude})")

    # FIELD_ARRAY fields (e.g. RANGE.obs) hold a repeated sub-message. They can
    # be assigned a plain list of Field objects, or an explicit FieldArray.
    range_msg = oem_msgs.RANGE()
    ObsField = oem_msgs.RANGE_obs_Field
    range_msg.obs = [
        ObsField(sv_prn=5, psr=20.0),
        ObsField(sv_prn=7, psr=21.0),
    ]
    print(f"RANGE now has {range_msg.obs_length} observations")

    # Individual elements of a FieldArray can be replaced in place.
    range_msg.obs[0] = ObsField(sv_prn=99)
    print(f"First observation's sv_prn is now {range_msg.obs[0].sv_prn}")

    # A field of an individual element can also be modified in place.
    range_msg.obs[0].psr = 25.0
    print(f"First observation's psr is now {range_msg.obs[0].psr}")

    # Once built, a message can be encoded like any decoded message.
    encoded = bestpos.encode(ne.ENCODE_FORMAT.ASCII)
    print(f"BESTPOS ASCII: {encoded.message}")

    # And it can be decoded straight back, since it round-trips through the
    # same database it was defined against.
    parser = oem.Parser()
    parser.write(bytes(encoded.message))
    decoded = parser.read()
    assert isinstance(decoded, oem_msgs.BESTPOS)
    print(f"Round-tripped latitude: {decoded.latitude}")


if __name__ == "__main__":
    main()
