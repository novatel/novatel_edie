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
//  Interface to the Data structure holding data
//
////////////////////////////////////////////////////////////////////////////////


#ifndef IDATALIST_H
#define IDATALIST_H

#include <agents.h>

template < typename ElemType>
class IDataList
{
public:
   virtual bool push(const ElemType& anElement) = 0;
   virtual bool pop(ElemType& anElement) = 0;
   virtual void clear() = 0;

   virtual bool wasError() const = 0; 
   virtual bool isEmpty() const = 0;
   virtual int GetSize() const = 0;
   virtual int GetCapacity() const = 0;

   virtual concurrency::event* GetDataEvent()=0;
};

#endif