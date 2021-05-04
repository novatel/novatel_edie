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
//    This stores port configurations
//
/////////////////////////////////////////////////////////////////////////////

#ifndef PORTCONFIG_H
#define PORTCONFIG_H

#include <string.h>

class PortConfig
{
public:

   enum PORT_TYPE
   {
      PORT_TYPE_SERIAL,
      PORT_TYPE_TCPIP,
      PORT_TYPE_UNKNOWN,
   };
   PortConfig(){};
   virtual ~PortConfig(){};

protected:
   int       iId;
   PORT_TYPE ePortType;
};

class SerialPortConfig : public PortConfig
{
public:
   enum COMPORT_HANDSHAKE           // State of flow control
   {
      COMPORT_HANDSHAKE_NONE,       // No Flow control
      COMPORT_HANDSHAKE_CTS,        // Use CTS
      COMPORT_HANDSHAKE_XON,        // Use XON/XOFF
      COMPORT_HANDSHAKE_UNKNOWN,    // Port found in a handshake mode that comport does not support,
      // and if in a set this indicates don't touch the handshaking
      COMPORT_HANDSHAKE_MAX,        // Always at the end
   };
   enum COMPORT_PARITY              // State of Parity
   {
      COMPORT_PARITY_NONE,
      COMPORT_PARITY_EVEN,
      COMPORT_PARITY_ODD,
      COMPORT_PARITY_MARK,
      COMPORT_PARITY_SPACE,
      COMPORT_PARITY_MAX
   };

   enum RTS_CNTL
   {  
      RTS_CNTL_ENABLE,
      RTS_CNTL_DISABLE,
   };

   enum DTR_CNTL
   {
      DTR_CNTL_ENABLE,
      DTR_CNTL_DISABLE,
   };

   SerialPortConfig( int iPort_,
      int iBaud_ = 9600,
      int iDatabits_ = 8,
      COMPORT_PARITY eParity_ = COMPORT_PARITY_NONE,
      int iStopBits_ = 1,
      COMPORT_HANDSHAKE eHandshake_ = COMPORT_HANDSHAKE_NONE):
         iBaud(iBaud_),
         iDatabits(iDatabits_),
         eParity(eParity_),
         iStopbits(iStopBits_),
         eHandshake(eHandshake_)
   {ePortType = PORT_TYPE_SERIAL;
   iId = iPort_;};

   int               GetPort()      {return iId;}
   int               GetBaud()      {return iBaud;}
   int               GetDatabits()  {return iDatabits;}
   COMPORT_PARITY    GetParity()    {return eParity;}
   int               GetStopbits()  {return iStopbits;}
   COMPORT_HANDSHAKE GetHandshake() {return eHandshake;}

   virtual ~SerialPortConfig() {};

private:

   SerialPortConfig():
      iBaud(9600),
      iDatabits(8),
      eParity(COMPORT_PARITY_NONE),
      iStopbits(1),
      eHandshake(COMPORT_HANDSHAKE_NONE)
   {ePortType = PORT_TYPE_SERIAL;
   iId = -1;};

   int iBaud;
   int iDatabits;
   COMPORT_PARITY eParity;
   int iStopbits;
   COMPORT_HANDSHAKE eHandshake;

};

class TCPIPPortConfig : public PortConfig
{
public:

   TCPIPPortConfig( char* iPort_,
      int iRxPort_ = 3001,
      int iTxPort_ = 3002):
         iRxPort(iRxPort_),
         iTxPort(iTxPort_)
   {ePortType = PORT_TYPE_TCPIP;
         ipaddress = new char[128];
         strcpy_s(ipaddress,15,iPort_);
   iId = -1;};


   virtual ~TCPIPPortConfig() {delete[] ipaddress; };

   char *ipaddress;
   int iRxPort;
   int iTxPort;

private:

   TCPIPPortConfig():
      ipaddress("192.0.0"),
      iRxPort(3001),
      iTxPort(3002)
   {ePortType = PORT_TYPE_TCPIP;
   iId = -1;};

};

#endif