/**
 * server/src/hardware/protocol/tracknroute/messages.hpp
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

#ifndef TRAINTASTIC_SERVER_HARDWARE_PROTOCOL_TRACKNROUTE_MESSAGES_HPP
#define TRAINTASTIC_SERVER_HARDWARE_PROTOCOL_TRACKNROUTE_MESSAGES_HPP

#include <cstdint>
#include <string>

namespace TracknRoute {

struct Message;

uint8_t calcChecksum(const Message& message);
bool isChecksumValid(const Message& message);
std::string toString(const Message& message);

enum class OpCode : uint8_t
{
  GetVersion_ = 0x10, // GetVersion is defined in winapi
  GetVersionResponse = 0x15,
  Detection = 0x21,
};

struct Message
{
  OpCode opCode;

  Message(OpCode _opCode)
    : opCode{_opCode}
  {
  }

  constexpr uint8_t dataSize() const
  {
    return static_cast<uint8_t>(opCode) & 0x0F;
  }

  constexpr uint8_t size() const
  {
    return 2 + dataSize();
  }
};

// GetVersion is defined in winapi
struct GetVersion_ : Message
{
  uint8_t checksum;

  GetVersion_()
    : Message(OpCode::GetVersion_)
    , checksum{calcChecksum(*this)}
  {
  }
};
static_assert(sizeof(GetVersion_) == 2);

struct GetVersionResponse : Message
{
  uint8_t softwareMajor;
  uint8_t softwareMinor;
  uint8_t softwarePatch;
  uint8_t hardwareMajor;
  uint8_t hardwareMinor;
  uint8_t checksum;

  GetVersionResponse()
    : Message(OpCode::GetVersionResponse)
  {
  }
};
static_assert(sizeof(GetVersionResponse) == 7);

struct Detection : Message
{
  static constexpr uint8_t addressMask = 0x7F;
  static constexpr uint8_t valueMask = ~addressMask;

  uint8_t data;
  uint8_t checksum;

  Detection(const uint8_t _address = 0, bool _value = false)
    : Message(OpCode::Detection)
    , data((_value ? valueMask : 0x00) | (_address & addressMask))
    , checksum{calcChecksum(*this)}
  {
  }

  constexpr uint8_t address() const
  {
    return data & addressMask;
  }

  constexpr bool value() const
  {
    return data & valueMask;
  }
};
static_assert(sizeof(Detection) == 3);

}

#endif
