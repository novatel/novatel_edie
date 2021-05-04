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
#include "inputportstream.hpp"
#include <iostream>

// code
// ---------------------------------------------------------
InputPortStream::InputPortStream(IDeviceDriver* rIDeviceDriver)
{
   pDeviceDriver = rIDeviceDriver;
   dMyTimeOut = 0.0;
   bMyCallBack = FALSE;
}

// ---------------------------------------------------------
InputPortStream::InputPortStream(const InputPortStream& clTemp)
{
   pDeviceDriver = clTemp.pDeviceDriver;
   //*this = clTemp;
}

// ---------------------------------------------------------
const InputPortStream& InputPortStream::operator= (const InputPortStream& clTemp)
{
   if(this != &clTemp)
   {
      pDeviceDriver = clTemp.pDeviceDriver;
   }
   return *this;
}

// ---------------------------------------------------------
InputPortStream::~InputPortStream()
{
   pDeviceDriver = NULL;
}

// ---------------------------------------------------------
StreamReadStatus InputPortStream::ReadData(ReadDataStructure& pReadDataStructure)
{ 
   StreamReadStatus stPortReadStatus;

   if (bMyCallBack != TRUE)
   {
      INT iDataSize = (INT)pReadDataStructure.uiDataSize;
      pDeviceDriver->Read(pReadDataStructure.cData, iDataSize, dMyTimeOut);
      stPortReadStatus.uiCurrentStreamRead = (UINT)iDataSize;
   }

   return stPortReadStatus;
}

// ---------------------------------------------------------
void InputPortStream::SetTimeOut(DOUBLE dTimeout_)
{
   dMyTimeOut = dTimeout_;
}

// ---------------------------------------------------------
void InputPortStream::RegisterCallBack(NovatelParser* pNovatelParser)
{
   if(bMyCallBack == TRUE)
   {
      clCallBackRead.SetNovatelParser(pNovatelParser);
      pDeviceDriver->RegisterCallback(&clCallBackRead);
   }
}

// ---------------------------------------------------------
void InputPortStream::EnableCallBack(BOOL bCallBack)
{
   bMyCallBack = bCallBack;
}

// ---------------------------------------------------------
BOOL InputPortStream::IsCallBackEnable()
{
   return bMyCallBack;
}
