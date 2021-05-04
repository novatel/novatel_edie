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
//    Interface Definition for all Device Drivers
//
/////////////////////////////////////////////////////////////////////////////

#ifndef IDEVICEDRIVER_H
#define IDEVICEDRIVER_H

#include <agents.h>
#include "DataPacket.h"
#include "IDataList.h"
#include "DriverInterfaceDelegates.h"
#include "PortConfig.h"

using namespace concurrency;

class IDeviceDriver
{
public:
   IDeviceDriver(){ evDataRead = new concurrency::event(); evError = new concurrency::event();};
   virtual ~IDeviceDriver(){ delete evDataRead; delete evError;};

   inline IDeviceDriver(const IDeviceDriver& rhs)
   {CopyValues(rhs);}
   inline IDeviceDriver& operator= (const IDeviceDriver & other)
   {
      if(this != &other)
      {
         CopyValues(other);
      }
      return *this;
   }

   // Interface Calls
   virtual void        RegisterCallback(IDriverRead*)                       {};
   virtual bool        Open( PortConfig* )                                  {return false;};
   virtual bool        Read(char*, int &, double = 0.0)                     {return false;};
   virtual bool        Write(const char*, int )                             {return false;};
   virtual bool        Close()                                              {return false;};
   virtual bool        GetLastError(char*, int &, int&)                     {return false;};
   virtual PortConfig* GetConfig()                                          {return NULL;};

   concurrency::event* const GetDataEvent() const                                    {return evDataRead;}
   concurrency::event* const GetErrorEvent() const                                   {return evError;}

   // Helper Methods
   void                SetQueue(IDataList<DataPacket>* pList_)   {plist = pList_;}
   inline void         ClearErrorQueue()
   {
      if(plist != NULL)
      {
         plist->clear();
      }
   }

protected:
   IDataList<DataPacket>* plist;

private:
   concurrency::event*  evDataRead;
   concurrency::event*  evError;

   inline void CopyValues(const IDeviceDriver& rhs)
   {
      evDataRead = rhs.evDataRead;
      evError = rhs.evError;
      plist = rhs.plist;
   }
};

#endif