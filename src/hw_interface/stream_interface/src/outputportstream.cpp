////////////////////////////////////////////////////////////////////////////////
//
//  Author: akhan
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
//    
//
////////////////////////////////////////////////////////////////////////////////

// Includes
#include "outputportstream.hpp"

// code
// ---------------------------------------------------------
OutputPortStream::OutputPortStream(IDeviceDriver* rIDeviceDriver)
{
   pDeviceDriver = rIDeviceDriver;
   pMessageDataFilter = NULL;
}

// ---------------------------------------------------------
OutputPortStream::OutputPortStream(IDeviceDriver* rIDeviceDriver, MessageDataFilter& rMessageDataFilter)
{
   pDeviceDriver = rIDeviceDriver;
   pMessageDataFilter = &rMessageDataFilter;
}

// ---------------------------------------------------------
OutputPortStream::OutputPortStream(const OutputPortStream& clTemp)
{
   pMessageDataFilter = clTemp.pMessageDataFilter;
   pDeviceDriver = clTemp.pDeviceDriver;
   //*this = clTemp;
}

// ---------------------------------------------------------
const OutputPortStream& OutputPortStream::operator= (const OutputPortStream& clTemp)
{
   if(this != &clTemp)
   {
      pMessageDataFilter = clTemp.pMessageDataFilter;
      pDeviceDriver = clTemp.pDeviceDriver;
   }
   return *this;
}

// ---------------------------------------------------------
OutputPortStream::~OutputPortStream()
{
   pDeviceDriver = NULL;
   pMessageDataFilter = NULL;
}

// ---------------------------------------------------------
UINT OutputPortStream::WriteData(BaseMessageData& pBaseMessageData)
{
   UINT uiReturn = 0;
   BOOL bWriteStatus = FALSE;

   if (pMessageDataFilter != NULL)  
   {
      if (pMessageDataFilter->Filter(pBaseMessageData) == TRUE )
      {
         bWriteStatus = pDeviceDriver->Write(pBaseMessageData.getMessageData() , pBaseMessageData.getMessageLength());
      }
   }
   else
   {
      bWriteStatus = pDeviceDriver->Write(pBaseMessageData.getMessageData() , pBaseMessageData.getMessageLength());
   }

   if (bWriteStatus == TRUE)
   {
      uiReturn = pBaseMessageData.getMessageLength();
   }
   return uiReturn;
}
