/////////////////////////////////////////////////////////////////////////////
//
//   COPYRIGHT NovAtel Inc, 2018 all rights reserved.
//
//   No part of this software may be reproduced or modified in any
//   form or by any means - electronic, mechanical, photocopying,
//   recording, or otherwise - without the prior written consent of
//   NovAtel Inc.
//
//   Author(s): P. Wong
//
/////////////////////////////////////////////////////////////////////////////
//                            DESCRIPTION
//
//    Global Error Codes From GetLastError
//
/////////////////////////////////////////////////////////////////////////////
#ifndef ERRORCODES_H
#define ERRORCODES_H

enum eErrorCode
{
   eErrorCodes_GeneralError     = -1,
   eErrorCodes_NoError          =  0,

   // Serial Driver Errors
   eErrorCodes_Serial_CommError             =  1,
   eErrorCodes_Serial_BufferOverflowError   =  2,
   eErrorCodes_Serial_RxBufferError         =  3,
   eErrorCodes_Serial_GeneralExceptError    =  4,
   eErrorCodes_Serial_GeneralUnhandledError =  5,
};

#endif