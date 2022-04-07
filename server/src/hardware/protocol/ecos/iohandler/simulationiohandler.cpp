/**
 * server/src/hardware/protocol/ecos/iohandler/simulationiohandler.cpp
 *
 * This file is part of the traintastic source code.
 *
 * Copyright (C) 2022 Reinder Feenstra
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

#include "simulationiohandler.hpp"
#include "../kernel.hpp"
#include "../messages.hpp"

namespace ECoS {

SimulationIOHandler::SimulationIOHandler(Kernel& kernel)
  : IOHandler(kernel)
{
}

bool SimulationIOHandler::send(std::string_view message)
{
  Request request;
  if(!parseRequest(message, request))
    return false;

  if(request.command == Command::request && request.options.size() == 1 && request.options[0] == Option::view)
  {
    return reply(std::string("<REPLY ").append(message).append(">\r\n<END 0 (OK)>\r\n"));
  }

  if(request.objectId == ObjectId::ecos)
  {
    if(request.command == Command::get && request.options.size() == 1 && request.options[0] == Option::info)
    {
      return reply(
        "<REPLY get(1, info)>\r\n"
        "1 ECoS2\r\n"
        "1 ProtocolVersion[0.5]\r\n"
        "1 ApplicationVersion[4.2.6]\r\n"
        "1 HardwareVersion[2.0]\r\n"
        "<END 0 (OK)>\r\n");
    }
  }

  return reply(std::string("<REPLY ").append(message).append(">\r\n<END 999 (Traintastic: no simulation support)>\r\n"));
}

bool SimulationIOHandler::reply(std::string_view message)
{
  // post the reply, so it has some delay
  m_kernel.ioContext().post(
    [this, data=std::string(message)]()
    {
      m_kernel.receive(data);
    });

  return true;
}

}