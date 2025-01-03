////////////////////////////////////////////////////////////////////////////////
//
//  Author:  Peter Wong
//
////////////////////////////////////////////////////////////////////////////////
//
//  COPYRIGHT NovAtel Inc, 2018. All rights reserved.
//
//  No part of this software may be reproduced or modified in any
//  form or by any means - electronic, mechanical, photocopying,
//  recording, or otherwise - without the prior written consent of
//  NovAtel Inc.
// 
////////////////////////////////////////////////////////////////////////////////
//
//  DESCRIPTION:
//  Public Delgate Information for Callbacks on the calling application to the API
//
////////////////////////////////////////////////////////////////////////////////

#ifndef Driver_INTERFACE_DELEGATES_H
#define Driver_INTERFACE_DELEGATES_H

#define  MAX_BUFFER_SIZE         20480*8   // The maximum size of buffer that can be given

#include <stdio.h>

enum DriverEventStatus
{
   Driver_READ_EMPTY   =  1,
   Driver_READ_SUCCESS =  0,
   Driver_READ_ERROR   = -1,
};

struct DriverInterfaceEventArgument
{
   unsigned long long  ulNumBytes;
   DriverEventStatus   eDriverStatus;
   unsigned char       ucCharBuffer[MAX_BUFFER_SIZE+1];
};

struct ObjectCredentials
{
   int identifier;
};

class IDriverRead
{
public:
   virtual void MyDriverReadHandler(ObjectCredentials &, DriverInterfaceEventArgument &objArgument_) {
      objArgument_.ucCharBuffer[MAX_BUFFER_SIZE+1] = '\0';
      printf_s("MyDriverReadHandler was called with value %d.\n", objArgument_.ucCharBuffer[0]);
   }
};

#endif