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
//    This stores message types from the API
//
/////////////////////////////////////////////////////////////////////////////

#ifndef DATAPACKET_H
#define DATAPACKET_H

#include <string>
#include "ErrorCodes.h"

class DataPacket
{
public:
   enum MESSAGE_TYPE
   {
      MESSAGE_TYPE_TRANSMIT,
      MESSAGE_TYPE_RECEIVE,
      MESSAGE_TYPE_ERROR,
      MESSAGE_TYPE_UNKNOWN,
   };

   DataPacket();

   DataPacket(MESSAGE_TYPE eMessageType_,
         const char* sMessage_            = "",
         int iSize_                       = 0,
         eErrorCode eErrorCode_           = eErrorCodes_NoError);

   virtual ~DataPacket();

   const char* GetString();
   int         GetLength();
   eErrorCode  GetErrorCode();

   void SetPacket(MESSAGE_TYPE eMsgType_, const char* string_, int iMessagelength_, eErrorCode eErrorCode_);

private:
   MESSAGE_TYPE eMessageType;
   std::string  sMessage;
   int          iSize;
   eErrorCode   eError;

};

#endif