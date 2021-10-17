/**
 * server/src/hardware/protocol/loconet/iohandler/serialiohandler.cpp
 *
 * This file is part of the traintastic source code.
 *
 * Copyright (C) 2021 Reinder Feenstra
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "serialiohandler.hpp"
#include "../kernel.hpp"
#include "../messages.hpp"
#include "../../../../log/logmessageexception.hpp"
#include "../../../../utils/serialport.hpp"

namespace LocoNet {

SerialIOHandler::SerialIOHandler(Kernel& kernel, const std::string& device, uint32_t baudrate, SerialFlowControl flowControl)
  : IOHandler(kernel)
  , m_serialPort{m_kernel.ioContext()}
  , m_readBufferOffset{0}
  , m_writeBufferOffset{0}
{
  using namespace SerialPort;

  boost::system::error_code ec;

  m_serialPort.open(device, ec);
  if(ec)
    throw LogMessageException(LogMessage::E2010_SERIAL_PORT_OPEN_FAILED_X, ec);

  if((ec = setBaudRate(m_serialPort, baudrate)))
    throw LogMessageException(LogMessage::E2013_SERIAL_PORT_SET_BAUDRATE_FAILED_X, ec);

  if((ec = setCharacterSize(m_serialPort, characterSize)))
    throw LogMessageException(LogMessage::E2014_SERIAL_PORT_SET_DATA_BITS_FAILED_X, ec);

  if((ec = setStopBitsOne(m_serialPort)))
    throw LogMessageException(LogMessage::E2015_SERIAL_PORT_SET_STOP_BITS_FAILED_X, ec);

  if((ec = setParityNone(m_serialPort)))
    throw LogMessageException(LogMessage::E2016_SERIAL_PORT_SET_PARITY_FAILED_X, ec);

  if((ec = setFlowControl(m_serialPort, flowControl)))
    throw LogMessageException(LogMessage::E2017_SERIAL_PORT_SET_FLOW_CONTROL_FAILED_X, ec);
}

SerialIOHandler::~SerialIOHandler()
{
  if(m_serialPort.is_open())
    m_serialPort.close();
}

void SerialIOHandler::start()
{
  read();
}

void SerialIOHandler::stop()
{
  m_serialPort.close();
}

bool SerialIOHandler::send(const Message& message)
{
  if(m_writeBufferOffset + message.size() > m_writeBuffer.size())
    return false;

  const bool wasEmpty = m_writeBufferOffset == 0;
  memcpy(m_writeBuffer.data() + m_writeBufferOffset, &message, message.size());
  m_writeBufferOffset += message.size();

  if(wasEmpty)
    write();

  return true;
}

void SerialIOHandler::read()
{
  m_serialPort.async_read_some(boost::asio::buffer(m_readBuffer.data() + m_readBufferOffset, m_readBuffer.size() - m_readBufferOffset),
    [this](const boost::system::error_code& ec, std::size_t bytesTransferred)
    {
      if(!ec)
      {
        const std::byte* pos = m_readBuffer.data();
        bytesTransferred += m_readBufferOffset;

        while(bytesTransferred > 1)
        {
          const Message* message = reinterpret_cast<const Message*>(pos);

          size_t drop = 0;
          while((message->size() == 0 || (message->size() <= bytesTransferred && !isValid(*message))) && bytesTransferred > 0)
          {
            drop++;
            pos++;
            bytesTransferred--;
            message = reinterpret_cast<const Message*>(pos);
          }

          if(drop != 0)
          {
            //EventLoop::call(
            //  [this, drop]()
            //  {
                //Log::log(*this, LogMessage::W2001_RECEIVED_MALFORMED_DATA_DROPPED_X_BYTES, drop);
            //  });
          }
          else if(message->size() <= bytesTransferred)
          {
            m_kernel.receive(*message);
            pos += message->size();
            bytesTransferred -= message->size();
          }
          else
            break;
        }

        if(bytesTransferred != 0)
          memmove(m_readBuffer.data(), pos, bytesTransferred);
        m_readBufferOffset = bytesTransferred;

        read();
      }
      else{}
        //EventLoop::call(
       //   [this, ec]()
         // {
            //Log::log(*this, LogMessage::E2002_SERIAL_READ_FAILED_X, ec);
            //online = false;
          //});
    });
}

void SerialIOHandler::write()
{
  m_serialPort.async_write_some(boost::asio::buffer(m_writeBuffer.data(), m_writeBufferOffset),
    [this](const boost::system::error_code& ec, std::size_t bytesTransferred)
    {
      if(!ec)
      {
        if(bytesTransferred < m_writeBufferOffset)
        {
          m_writeBufferOffset -= bytesTransferred;
          memmove(m_writeBuffer.data(), m_writeBuffer.data() + bytesTransferred, m_writeBufferOffset);
          write();
        }
        else
          m_writeBufferOffset = 0;
      }
      else if(ec != boost::asio::error::operation_aborted)
      {
        // LogMessage::E1006_SOCKET_WRITE_FAILED_X, ec
      }
    });
}

}
