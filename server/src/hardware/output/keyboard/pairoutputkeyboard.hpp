/**
 * server/src/hardware/output/keyboard/pairoutputkeyboard.hpp
 *
 * This file is part of the traintastic source code.
 *
 * Copyright (C) 2024 Reinder Feenstra
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

#ifndef TRAINTASTIC_SERVER_HARDWARE_OUTPUT_KEYBOARD_PAIROUTPUTKEYBOARD_HPP
#define TRAINTASTIC_SERVER_HARDWARE_OUTPUT_KEYBOARD_PAIROUTPUTKEYBOARD_HPP

#include "outputkeyboard.hpp"
#include <traintastic/enum/outputpairvalue.hpp>
#include "../../../core/method.hpp"
#include "../../../core/event.hpp"

class PairOutputKeyboard : public OutputKeyboard
{
  CLASS_ID("output_keyboard.pair")

  public:
    Method<void(uint32_t, OutputPairValue)> setOutputValue;
    Event<uint32_t, OutputPairValue> outputValueChanged;

    PairOutputKeyboard(OutputController& controller, OutputChannel channel_);

    std::vector<OutputInfo> getOutputInfo() const final;
    void fireOutputValueChanged(uint32_t address, OutputValue value) final;
};

#endif
