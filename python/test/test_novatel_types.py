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
# \brief Unit tests for OEM Framer, HeaderDecoder, MessageDecoder,
# Encoder and Filter.
################################################################################

from pathlib import Path

import novatel_edie as ne
from novatel_edie import HEADERFORMAT, STATUS
import pytest

# -------------------------------------------------------------------------------------------------------
# Novatel Types Unit Tests
# -------------------------------------------------------------------------------------------------------
class NovatelTypesTest : public.testing.Test

protected:
   class DecoderTester : public MessageDecoder:
   public:
      DecoderTester(JsonReader* json_db_) : MessageDecoder(json_db_) {}

      STATUS TestDecodeAscii(const std.vector<BaseField*> MsgDefFields_, const char** log_buf_, std.vector<FieldContainer> intermediate_format_):
         return DecodeAscii(MsgDefFields_, const_cast<char**>(log_buf_), intermediate_format_);

      STATUS TestDecodeBinary(const std.vector<BaseField*> MsgDefFields_, unsigned char** log_buf_, std.vector<FieldContainer> intermediate_format_):
         uint16_t MsgDefFieldsSize = 0;
         for (BaseField* field : MsgDefFields_)
         {
            MsgDefFieldsSize += field.data_type.length;
         }
         return DecodeBinary(MsgDefFields_, log_buf_, intermediate_format_, MsgDefFieldsSize);