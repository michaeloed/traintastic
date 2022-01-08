/**
 * server/src/hardware/protocol/tracknroute/kernel.cpp
 *
 * This file is part of the traintastic source code.
 *
 * Copyright (C) 2021-2022 Reinder Feenstra
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

#include "kernel.hpp"
#include "messages.hpp"
#include "../../input/inputcontroller.hpp"
#include "../../../utils/setthreadname.hpp"
#include "../../../core/eventloop.hpp"
#include "../../../log/log.hpp"

namespace TracknRoute {

Kernel::Kernel(const Config& config)
  : m_ioContext{1}
  , m_startupDelayTimer{m_ioContext}
  , m_inputController{nullptr}
  , m_config{config}
#ifndef NDEBUG
  , m_started{false}
#endif
{
}

void Kernel::setConfig(const Config& config)
{
  m_ioContext.post(
    [this, newConfig=config]()
    {
      m_config = newConfig;
    });
}

void Kernel::start()
{
  assert(m_ioHandler);
  assert(!m_started);

  // reset all state values
  m_inputValues.fill(TriState::Undefined);

  m_thread = std::thread(
    [this]()
    {
      setThreadName("tracknroute");
      auto work = std::make_shared<boost::asio::io_context::work>(m_ioContext);
      m_ioContext.run();
    });

  m_ioContext.post(
    [this]()
    {
      m_ioHandler->start();

      m_startupDelayTimer.expires_after(boost::asio::chrono::milliseconds(m_config.startupDelay));
      m_startupDelayTimer.async_wait(std::bind(&Kernel::startupDelayExpired, this, std::placeholders::_1));
    });

#ifndef NDEBUG
  m_started = true;
#endif
}

void Kernel::stop()
{
  m_ioContext.post(
    [this]()
    {
      m_ioHandler->stop();
    });

  m_ioContext.stop();

  m_thread.join();

#ifndef NDEBUG
  m_started = false;
#endif
}

void Kernel::receive(const Message& message)
{
  if(m_config.debugLogRXTX)
    EventLoop::call(
      [this, msg=toString(message)]()
      {
        Log::log(m_logId, LogMessage::D2002_RX_X, msg);
      });

  switch(message.opCode)
  {
    case OpCode::GetVersionResponse:
      EventLoop::call(
        [this, response=static_cast<const GetVersionResponse&>(message)]()
        {
          Log::log(m_logId, LogMessage::I2004_HARDWARE_VERSION_X,
            std::string()
              .append(std::to_string(response.hardwareMajor))
              .append(".")
              .append(std::to_string(response.hardwareMinor)));

          Log::log(m_logId, LogMessage::I2003_FIRMWARE_VERSION_X,
            std::string()
              .append(std::to_string(response.softwareMajor))
              .append(".")
              .append(std::to_string(response.softwareMinor))
              .append(".")
              .append(std::to_string(response.softwarePatch)));
        });
      break;

    case OpCode::Detection:
      if(m_inputController)
      {
        const auto& detection = static_cast<const Detection&>(message);
        const auto address = detection.address();
        const auto value = toTriState(detection.value());
        if(address < m_inputValues.size() && m_inputValues[address] != value)
        {
          m_inputValues[address] = value;

          EventLoop::call(
            [this, address, value]()
            {
              m_inputController->updateInputValue(1 + address, value);
            });
        }
      }
      break;

    case OpCode::GetVersion_:
      break;
  }
}

void Kernel::setIOHandler(std::unique_ptr<IOHandler> handler)
{
  assert(handler);
  assert(!m_ioHandler);
  m_ioHandler = std::move(handler);
}

void Kernel::send(const Message& message)
{
  if(m_ioHandler->send(message))
  {
    if(m_config.debugLogRXTX)
      EventLoop::call(
        [this, msg=toString(message)]()
        {
          Log::log(m_logId, LogMessage::D2001_TX_X, msg);
        });
  }
  else
  {} // log message and go to error state
}

void Kernel::startupDelayExpired(const boost::system::error_code& ec)
{
  if(ec)
    return;

  send(GetVersion_());

  if(m_onStarted)
    EventLoop::call(
      [this]()
      {
        m_onStarted();
      });
}

}
