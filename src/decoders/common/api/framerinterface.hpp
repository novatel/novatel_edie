////////////////////////////////////////////////////////////////////////
//
// COPYRIGHT NovAtel Inc, 2022. All rights reserved.
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
////////////////////////////////////////////////////////////////////////
//                            DESCRIPTION
//
//! \file common.hpp
//! \brief Header file containing the common structs, enums and defines
//! used throughout the EDIE source code.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef FRAMERINTERFACE_HPP
#define FRAMERINTERFACE_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include <memory>
#include "circularbuffer.hpp"
#include "logger/logger.hpp"

//============================================================================
//! \class FramerInterface
//! \brief Base class for all framers.  Contains necessary buffers and member
//! variables, defining generic framer operations.
//============================================================================
class FramerInterface
{
protected:
   std::shared_ptr<spdlog::logger> pclMyLogger;
   CircularBuffer clMyCircularDataBuffer;

   uint32_t uiMyCalculatedCRC32{ 0U };

   uint32_t uiMyByteCount{ 0U };
   uint32_t uiMyExpectedPayloadLength{ 0U };
   uint32_t uiMyExpectedMessageLength{ 0U };

   bool bMyReportUnknownBytes{ true };

   virtual void HandleUnknownBytes(unsigned char* pucBuffer_, uint32_t uiUnknownBytes_) = 0;

public:

   //----------------------------------------------------------------------------
   //! \brief A constructor for the FramerInterface class.
   //
   //! \param[in] strLoggerName_ String to name the internal logger.
   //----------------------------------------------------------------------------
   FramerInterface(const std::string& strLoggerName_) :
      pclMyLogger(Logger().RegisterLogger(strLoggerName_))
   {
      clMyCircularDataBuffer.Clear();
      pclMyLogger->debug("Framer initialized");
   }

   //----------------------------------------------------------------------------
   //! \brief Get the internal logger.
   //
   //! \return Shared pointer to the internal logger object.
   //----------------------------------------------------------------------------
   std::shared_ptr<spdlog::logger>
   GetLogger() const
   {
      return pclMyLogger;
   }

   //----------------------------------------------------------------------------
   //! \brief Get the internal logger.
   //
   //! \param[in] eLevel_ Shared pointer to the internal logger object.
   //----------------------------------------------------------------------------
   void
   SetLoggerLevel(spdlog::level::level_enum eLevel_) const
   {
      pclMyLogger->set_level(eLevel_);
   }

   //----------------------------------------------------------------------------
   //! \brief Shutdown the internal logger.
   //----------------------------------------------------------------------------
   static void
   ShutdownLogger()
   {
      Logger::Shutdown();
   }

   //----------------------------------------------------------------------------
   //! \brief Configure the framer to return unknown bytes in the provided
   //! buffer.
   //
   //! \param[in] bReportUnknownBytes_ Set to true to return unknown bytes.
   //----------------------------------------------------------------------------
   virtual void
   SetReportUnknownBytes(bool bReportUnknownBytes_)
   {
      bMyReportUnknownBytes = bReportUnknownBytes_;
   }

   //----------------------------------------------------------------------------
   //! \brief Get the number of bytes available in the internal circular buffer.
   //
   //! \return The number of bytes available in the internal circular buffer.
   //----------------------------------------------------------------------------
   virtual uint32_t
   GetBytesAvailableInBuffer() const
   {
      return clMyCircularDataBuffer.GetCapacity() - clMyCircularDataBuffer.GetLength();
   }

   //----------------------------------------------------------------------------
   //! \brief Write new bytes to the internal circular buffer.
   //
   //! \param[in] pucDataBuffer_ The data buffer containing the bytes to be
   //! written into the framer buffer.
   //! \param[in] uiDataBytes_ The number of bytes contained in pucDataBuffer_.
   //
   //! \return The number of bytes written to the internal circular buffer.
   //----------------------------------------------------------------------------
   virtual uint32_t Write(unsigned char* pucDataBuffer_, uint32_t uiDataBytes_)
   {
      return clMyCircularDataBuffer.Append(pucDataBuffer_, uiDataBytes_);
   }

   //----------------------------------------------------------------------------
   //! \brief Flush bytes from the internal circular buffer.
   //
   //! \param[in] pucBuffer_ The buffer to contain the flushed bytes.
   //! \param[in] uiBufferSize_ The size of the provided buffer.
   //
   //! \return The number of bytes flushed from the internal circular buffer.
   //----------------------------------------------------------------------------
   virtual uint32_t
   Flush(unsigned char* pucBuffer_, uint32_t uiBufferSize_)
   {
      const uint32_t uiBytesToFlush = std::min(clMyCircularDataBuffer.GetLength(), uiBufferSize_);

      HandleUnknownBytes(pucBuffer_, uiBytesToFlush);
      return uiBytesToFlush;
   }
};

#endif // FRAMERINTERFACE_HPP
