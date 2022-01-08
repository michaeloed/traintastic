/**
 * server/src/hardware/interface/tracknrouteinterface.cpp
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

#include "tracknroutinterface.hpp"
#include "../input/list/inputlisttablemodel.hpp"
#include "../protocol/tracknroute/config.hpp"
#include "../protocol/tracknroute/iohandler/serialiohandler.hpp"
#include "../../core/attributes.hpp"
#include "../../log/log.hpp"
#include "../../log/logmessageexception.hpp"
#include "../../utils/displayname.hpp"
#include "../../world/world.hpp"

TracknRouteInterface::TracknRouteInterface(const std::weak_ptr<World>& world, std::string_view _id)
  : Interface(world, _id)
  , device{this, "device", "", PropertyFlags::ReadWrite | PropertyFlags::Store}
  , tracknRoute{this, "track_n_route", nullptr, PropertyFlags::ReadOnly | PropertyFlags::Store | PropertyFlags::SubObject}
  , inputs{this, "inputs", nullptr, PropertyFlags::ReadOnly | PropertyFlags::NoStore | PropertyFlags::SubObject}
{
  name = "Track 'n Route";
  tracknRoute.setValueInternal(std::make_shared<TracknRoute::Settings>(*this, tracknRoute.name()));
  inputs.setValueInternal(std::make_shared<InputList>(*this, inputs.name()));

  Attributes::addDisplayName(device, DisplayName::Serial::device);
  Attributes::addEnabled(device, !online);
  m_interfaceItems.insertBefore(device, notes);

  m_interfaceItems.insertBefore(tracknRoute, notes);

  Attributes::addDisplayName(inputs, DisplayName::Hardware::inputs);
  m_interfaceItems.insertBefore(inputs, notes);
}

bool TracknRouteInterface::addInput(Input& input)
{
  const bool success = InputController::addInput(input);
  if(success)
    inputs->addObject(input.shared_ptr<Input>());
  return success;
}

bool TracknRouteInterface::removeInput(Input& input)
{
  const bool success = InputController::removeInput(input);
  if(success)
    inputs->removeObject(input.shared_ptr<Input>());
  return success;
}

bool TracknRouteInterface::setOnline(bool& value)
{
  if(!m_kernel && value)
  {
    try
    {
      m_kernel = TracknRoute::Kernel::create<TracknRoute::SerialIOHandler>(tracknRoute->config(), device.value());

      status.setValueInternal(InterfaceStatus::Initializing);

      m_kernel->setLogId(id.value());
      m_kernel->setOnStarted(
        [this]()
        {
          status.setValueInternal(InterfaceStatus::Online);
        });
      m_kernel->setInputController(this);
      m_kernel->start();

      Attributes::setEnabled(device, false);
    }
    catch(const LogMessageException& e)
    {
      status.setValueInternal(InterfaceStatus::Offline);
      Log::log(*this, e.message(), e.args());
      return false;
    }
  }
  else if(m_kernel && !value)
  {
    Attributes::setEnabled(device, true);

    m_kernel->stop();
    m_kernel.reset();

    status.setValueInternal(InterfaceStatus::Offline);
  }
  return true;
}

void TracknRouteInterface::addToWorld()
{
  Interface::addToWorld();

  if(auto world = m_world.lock())
  {
    world->inputControllers->add(std::dynamic_pointer_cast<InputController>(shared_from_this()));
  }
}

void TracknRouteInterface::destroying()
{
  for(const auto& input : *inputs)
  {
    assert(input->interface.value() == std::dynamic_pointer_cast<InputController>(shared_from_this()));
    input->interface = nullptr;
  }

  if(auto world = m_world.lock())
  {
    world->inputControllers->remove(std::dynamic_pointer_cast<InputController>(shared_from_this()));
  }

  Interface::destroying();
}

void TracknRouteInterface::idChanged(const std::string& newId)
{
  if(m_kernel)
    m_kernel->setLogId(newId);
}
