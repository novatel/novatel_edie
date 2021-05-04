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
//    Entry Point of the API for Library API Calls
//
/////////////////////////////////////////////////////////////////////////////


#ifndef DriverInterfaceAPI_H
#define DriverInterfaceAPI_H

#include "IDeviceDriver.h"
#include "PortConfig.h"
//#include "SerialDriver.h"

#ifdef BuildDLL
   extern "C" {
   #define DriverInterface_EXT __declspec(dllexport)
   #define DriverInterface_CALL __stdcall
   //#define DriverInterface_CALL
#else
   #define DriverInterface_EXT
   #define DriverInterface_CALL
#endif

namespace DriverInterfaceAPI
{
   // Return codes from CANInterface API functions
   enum DriverAPI_Status : unsigned int
   {
      DriverAPI_Success                  = 0x00000000,
      DriverAPI_Failed                   = 0x00000001,
   };

   //struct PortConfigStruct
   //{
   //   int   iId;
   //   int   ePortType;
   //};

   //struct SerialPortConfigStruct : PortConfigStruct
   //{
   //   int iBaud;
   //   int iDatabits;
   //   int eParity;
   //   int iStopbits;
   //   int eHandshake;
   //};

   //struct TCPIPPortConfigStruct : public PortConfigStruct
   //{
   //   char *ipaddress;
   //   int iRxPort;
   //   int iTxPort;

   //};
   
   DriverInterface_EXT const char* const DriverInterface_CALL GetAPIVersion();

   DriverInterface_EXT const char* const DriverInterface_CALL GetDriverVersion();

   DriverInterface_EXT const char* const DriverInterface_CALL GetDriverDate();

   DriverInterface_EXT DriverAPI_Status DriverInterface_CALL GetInterface(PortConfig * stConfig_, IDeviceDriver*& iDriver_, IDriverRead* iCallback_ = NULL);

   DriverInterface_EXT IDeviceDriver* DriverInterface_CALL GetInterface2(PortConfig * stConfig_);

   //DriverInterface_EXT DriverAPI_Status DriverInterface_CALL CloseInterface(IDeviceDriver * iDriver_);

   /*DriverInterface_EXT DriverAPI_Status DriverInterface_CALL OpenInterface(PortConfig * stConfig_, IDeviceDriver* iDriver_);
   DriverInterface_EXT DriverAPI_Status DriverInterface_CALL CloseInterface(IDeviceDriver* iDriver_);

   DriverInterface_EXT bool DriverInterface_CALL ReadFromInterface(IDeviceDriver* iDriver_, char* message_, int &iNumRead, double dTimeout_ = 0.0);
   DriverInterface_EXT bool DriverInterface_CALL WriteToInterface(IDeviceDriver* iDriver_, const char* message_, int iNumBytes_);


   DriverInterface_EXT SerialPortConfig* DriverInterface_CALL CreateSerialPortConfig(unsigned int iPort_);
   DriverInterface_EXT DriverAPI_Status DriverInterface_CALL DestroySerialPortConfig(PortConfig* stConfig_);

   DriverInterface_EXT TCPIPPortConfig* DriverInterface_CALL CreateTCPIPPortConfig(char * ip_, unsigned int txport_, unsigned int rxport_);
   DriverInterface_EXT DriverAPI_Status DriverInterface_CALL DestroyTCPIPPortConfig(PortConfig* stConfig_);*/

//   DriverInterface_EXT DriverAPI_Status DriverInterface_CALL GetSerialInterface(SerialPortConfig * stConfig_, SerialDriver & iDriver_, IDriverRead* iCallback_ = NULL);
}

#ifdef BuildDLL
}
#endif
#endif
