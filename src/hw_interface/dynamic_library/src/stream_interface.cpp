// ===============================================================================
// |                                                                             |
// |  COPYRIGHT NovAtel Inc, 2022. All rights reserved.                          |
// |                                                                             |
// |  Permission is hereby granted, free of charge, to any person obtaining a    |
// |  copy of this software and associated documentation files (the "Software"), |
// |  to deal in the Software without restriction, including without limitation  |
// |  the rights to use, copy, modify, merge, publish, distribute, sublicense,   |
// |  and/or sell copies of the Software, and to permit persons to whom the      |
// |  Software is furnished to do so, subject to the following conditions:       |
// |                                                                             |
// |  The above copyright notice and this permission notice shall be included    |
// |  in all copies or substantial portions of the Software.                     |
// |                                                                             |
// |  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR |
// |  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   |
// |  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    |
// |  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER |
// |  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    |
// |  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        |
// |  DEALINGS IN THE SOFTWARE.                                                  |
// |                                                                             |
// ===============================================================================
// ! \file stream_interface.cpp
// ===============================================================================

#include "stream_interface.hpp"

InputFileStream* ifs_init(char* pcInputFilePath_)
{
    auto pclIFS = new InputFileStream(pcInputFilePath_);
    return pclIFS;
}

void ifs_del(InputStreamInterface* pclIFS_)
{
    if (pclIFS_)
    {
        delete pclIFS_;
        pclIFS_ = nullptr;
    }
}

void ifs_read(InputFileStream* pclIFS_, StreamReadStatus* pstReadStatus_, char* pcReadBuf_, int iBufSize_)
{
    ReadDataStructure stReadData;
    stReadData.cData = pcReadBuf_;
    stReadData.uiDataSize = iBufSize_;
    StreamReadStatus stReadStatus = pclIFS_->ReadData(stReadData);
    // TODO: why make the temporary object if we're just going to copy over all the members?
    pstReadStatus_->bEOS = stReadStatus.bEOS;
    pstReadStatus_->uiCurrentStreamRead = stReadStatus.uiCurrentStreamRead;
    pstReadStatus_->uiPercentStreamRead = stReadStatus.uiPercentStreamRead;
    pstReadStatus_->ullStreamLength = stReadStatus.ullStreamLength;
}
