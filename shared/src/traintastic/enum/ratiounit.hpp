/**
 * shared/src/traintastic/enum/ratiounit.hpp
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

#ifndef TRAINTASTIC_SHARED_TRAINTASTIC_ENUM_RATIOUNIT_HPP
#define TRAINTASTIC_SHARED_TRAINTASTIC_ENUM_RATIOUNIT_HPP

#include <cstdint>

enum class RatioUnit
{
  Ratio = 0,
  Percent = 1,
};

ENUM_NAME(RatioUnit, "ratio_unit")

ENUM_VALUES(RatioUnit, 2,
{
  {RatioUnit::Ratio, "ratio"},
  {RatioUnit::Percent, "percent"},
})

#endif
