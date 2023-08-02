/**
 * server/src/board/tile/rail/sensorrailtile.cpp
 *
 * This file is part of the traintastic source code.
 *
 * Copyright (C) 2020-2021,2023 Reinder Feenstra
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

#include "sensorrailtile.hpp"
#include "../../../world/world.hpp"
#include "../../../core/attributes.hpp"
#include "../../../core/objectproperty.tpp"
#include "../../../utils/sensor.hpp"
#include "../../../utils/displayname.hpp"

SensorRailTile::SensorRailTile(World& world, std::string_view _id) :
  StraightRailTile(world, _id, TileId::RailSensor),
  name{this, "name", id, PropertyFlags::ReadWrite | PropertyFlags::Store},
  input{this, "input", nullptr, PropertyFlags::ReadWrite | PropertyFlags::Store,
    [this](const std::shared_ptr<Input>& value)
    {
      if(value)
        inputPropertyChanged(input->value);
      else
        state.setValueInternal(SensorState::Unknown);
    },
    [this](const std::shared_ptr<Input>& value)
    {
      if(input)
        disconnectInput(*input);

      if(value)
        connectInput(*value);

      return true;
    }},
  type{this, "type", SensorType::OccupyDetector, PropertyFlags::ReadWrite | PropertyFlags::Store,
    [this](SensorType /*value*/)
    {
      if(input)
        inputPropertyChanged(input->value);
    }},
  invert{this, "invert", false, PropertyFlags::ReadWrite | PropertyFlags::Store,
    [this](bool /*value*/)
    {
      if(input)
        inputPropertyChanged(input->value);
    }},
  state{this, "state", SensorState::Unknown, PropertyFlags::ReadOnly | PropertyFlags::StoreState}
{
  const bool editable = contains(m_world.state.value(), WorldState::Edit);

  Attributes::addEnabled(name, editable);
  Attributes::addDisplayName(name, DisplayName::Object::name);
  m_interfaceItems.add(name);
  Attributes::addEnabled(input, editable);
  Attributes::addObjectList(input, m_world.inputs);
  m_interfaceItems.add(input);
  Attributes::addEnabled(type, editable);
  Attributes::addValues(type, sensorTypeValues);
  m_interfaceItems.add(type);
  Attributes::addEnabled(invert, editable);
  m_interfaceItems.add(invert);
  Attributes::addObjectEditor(state, false);
  Attributes::addValues(state, sensorStateValues);
  m_interfaceItems.add(state);
}

SensorRailTile::~SensorRailTile()
{
  assert(!input);
  assert(!m_inputPropertyChanged.connected());
  assert(!m_inputDestroying.connected());
}

void SensorRailTile::loaded()
{
  StraightRailTile::loaded();

  if(input)
    connectInput(*input);
}

void SensorRailTile::destroying()
{
  input = nullptr;
  StraightRailTile::destroying();
}

void SensorRailTile::worldEvent(WorldState worldState, WorldEvent worldEvent)
{
  StraightRailTile::worldEvent(worldState, worldEvent);

  const bool editable = contains(worldState, WorldState::Edit);

  Attributes::setEnabled(name, editable);
  Attributes::setEnabled(input, editable);
  Attributes::setEnabled(type, editable);
  Attributes::setEnabled(invert, editable);
}

void SensorRailTile::connectInput(Input& object)
{
  object.consumers.appendInternal(shared_from_this());
  m_inputDestroying = object.onDestroying.connect(
    [this]([[maybe_unused]] Object& obj)
    {
      assert(input.value().get() == &obj);
      input = nullptr;
    });
  m_inputPropertyChanged = object.propertyChanged.connect(std::bind(&SensorRailTile::inputPropertyChanged, this, std::placeholders::_1));
}

void SensorRailTile::disconnectInput(Input& object)
{
  m_inputPropertyChanged.disconnect();
  m_inputDestroying.disconnect();
  object.consumers.removeInternal(shared_from_this());
}

void SensorRailTile::inputPropertyChanged(BaseProperty& property)
{
  assert(input);
  if(&property == static_cast<BaseProperty*>(&input->value))
    state.setValueInternal(toSensorState(type, input->value.value() ^ invert.value()));
}
