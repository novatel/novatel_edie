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

/*! \file inputportstream.hpp
 *  \brief It is a Derived class from main InputStream. Input to the decoder is file.
 * 
 */

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef INPUTPORTSTREAM_HPP
#define INPUTPORTSTREAM_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "inputstreaminterface.hpp"
#include "decoders/novatel/api/novatelparser.hpp"
#include "lib/driverinterface/api/IDeviceDriver.h"

/*! \class CallBackRead
 *   \brief A Derived class will be used by decoder, if the decoded input is device driver object.
 * 
 *  Derived from base class IDriverRead.
 *
*/
class CallBackRead : virtual public IDriverRead
{
public:
    /*! A Constructor
	 *  \brief  Creates CallBackRead class object.
	 */
   CallBackRead()
   {
   }
   /*! A default destructor */   
   ~CallBackRead()
   {
      pMyNovatelParser = NULL;
   }
   /*! \fn void SetNovatelParser(NovatelParser*)
    *  \brief Set Novatel parser
    *   
    *  \param [in] Novatel parser object
    */
   void SetNovatelParser(NovatelParser* ptrNovatelParser)
   {
      pMyNovatelParser = ptrNovatelParser;
   }
   /*! \fn virtual void MyDriverReadHandler(ObjectCredentials &, DriverInterfaceEventArgument &)
    *  \brief This is Driver read handler
    *   
    *  \param [in] ObjectCredentials and DriverInterfaceEventArgument
    */
   virtual void MyDriverReadHandler(ObjectCredentials &, DriverInterfaceEventArgument &objArgument_)
   {
      std::string sCharBuf((const char*)&objArgument_.ucCharBuffer[0],(INT)objArgument_.ulNumBytes);
      pMyNovatelParser->getCircularBuffer()->Append((UCHAR*)sCharBuf.c_str(), (UINT)objArgument_.ulNumBytes);
      pMyNovatelParser->getStreamReadStatus()->uiCurrentStreamRead = (UINT)objArgument_.ulNumBytes;
   }
private:
   /*! \var pMyNovatelParser
    *
    *  Novatel Parser object.
    */
   NovatelParser* pMyNovatelParser;
};

/*! \class InputPortStream
 *   \brief A Derived class will be used by decoder, if the decoded input is IDeviceDriver.
 * 
 *  Derived from base class InputStreamInterface.
 *
*/
class InputPortStream : public InputStreamInterface
{
public:
    /*! A Constructor.
	 *  \brief  Creates PortStream Object for reading.
	 *  \param [in] IDeviceDriver object for Output.
	 * 
	 */ 
   InputPortStream(IDeviceDriver*);
	/*! Public Copy Constructor 
	 *
	 *  A copy constructor is a member function which initializes an object using another object of the same class. 
	 */   
   InputPortStream(const InputPortStream& clTemp);
	/*! Public assignment operator 
	 *
	 *  The copy assignment operator is called whenever selected by overload resolution, 
	 *  e.g. when an object appears on the left side of an assignment expression.
	 */   
   const InputPortStream& operator= (const InputPortStream& clTemp);
    /*! A virtual destructor.
	 *  \brief  Clears PortStream objects.
	 * 
	 */   
   virtual ~InputPortStream();
   /*! \fn StreamReadStatus ReadData(ReadDataStructure&)
    *  \brief Read data from input port.
    *  \param [in] ReadDataStructure object.
    *  \return StreamReadStatus read data statistics.
    */
   StreamReadStatus ReadData(ReadDataStructure&);
   /*! \fn void SetTimeOut(DOUBLE dTimeout_)
    *  \brief Set time out.
    *  \param [in] DOUBLE TimeOut value.
    */   
   void SetTimeOut(DOUBLE dTimeout_);
   /*! \fn void RegisterCallBack(NovatelParser* pNovatelParser)
    *  \brief Registers Call bck method
    *  \param [in] NovatelParser object.
    */   
   void RegisterCallBack(NovatelParser* pNovatelParser);
   /*! \fn void EnableCallBack(BOOL bCallBack)
    *  \brief Enables call back, by default reading is set to polling mechanism.
    *  \param [in] BOOL TRUE or FALSE.
    */   
   void EnableCallBack(BOOL bCallBack);
   /*! \fn BOOL IsCallBackEnable()
    *  \brief Checks if call back reading is set.
    *  \return TRUE or FALSE
    */   
   BOOL IsCallBackEnable();

private:
   /*! \var pDeviceDriver
    *
    * IDeviceDriver pointer.
    */
   IDeviceDriver* pDeviceDriver;
   /*! \var dMyTimeOut
    *
    *  Time Out value.
    */   
   DOUBLE dMyTimeOut;
   /*! \var clCallBackRead
    *
    *  CallBackRead object.
    */   
   CallBackRead clCallBackRead;
   /*! \var bMyCallBack
    *
    *  Call Back value.
    */   
   BOOL bMyCallBack;
};

#endif