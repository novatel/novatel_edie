////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020 NovAtel Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

/*! \file outputportstream.hpp
 *  \brief Class definition to provide basic port operations for writing on output ports.
 * 
 *  Class has defined in a such a way that will provide basic port operations and writing data to it, 
 *  Will support, Write ... operations etc.
 */

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef OUTPUTPORTSTREAM_HPP
#define OUTPUTPORTSTREAM_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "outputstreaminterface.hpp"
#include "decoders/novatel/api/filters/messagedatafilter.hpp"
#include "lib/driverinterface/api/IDeviceDriver.h"

/*! \class OutputPortStream
 *  \brief A Derived class from parent interface class OutputStreamInterface.
 * 
 *  More detailed PortStream class description.
 *  
*/
class OutputPortStream : public OutputStreamInterface
{
public:
    /*! A Constructor.
	 *  \brief  Creates PortStream Object for writing output. And intializes MessageDataFilter object to NULL.
	 *  \param [in] IDeviceDriver object for Output.
	 * 
	 */    
   OutputPortStream(IDeviceDriver*);
    /*! A Constructor.
	 *  \brief  Creates PortStream Object for writing output. And intializes MessageDataFilter object with provided.
	 *  \param [in] IDeviceDriver object for Output.
	 *  \param [in] rMessageDataFilter Object used for filtering output.
	 * 
	 */   
   OutputPortStream(IDeviceDriver*, MessageDataFilter&);
	/*! Public Copy Constructor 
	 *
	 *  A copy constructor is a member function which initializes an object using another object of the same class. 
	 */   
   OutputPortStream(const OutputPortStream& clTemp);
	/*! Public assignment operator 
	 *
	 *  The copy assignment operator is called whenever selected by overload resolution, 
	 *  e.g. when an object appears on the left side of an assignment expression.
	 */   
   const OutputPortStream& operator= (const OutputPortStream& clTemp);
    /*! A virtual destructor.
	 *  \brief  Clears MessageDataFilter and PortStream objects.
	 * 
	 */   
   virtual ~OutputPortStream();
   /*! \fn UINT WriteData(BaseMessageData&)
    *  \brief Write data to output port.
    *  \param [in] pclBaseMessageData BaseMessageData object.
    *  \sa BaseMessageData
    *  \return Number of bytes written to output port.
    */
   UINT WriteData(BaseMessageData&);

private:
   /*! IDeviceDriver Class object.
    * \sa IDeviceDriver
    */
   IDeviceDriver* pDeviceDriver;
   /*! MessageDataFilter Class object.
    * \sa MessageDataFilter
    */   
   MessageDataFilter* pMessageDataFilter;
};

#endif