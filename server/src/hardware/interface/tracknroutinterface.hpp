/**
 * server/src/hardware/interface/tracknrouteinterface.hpp
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

#ifndef TRAINTASTIC_SERVER_HARDWARE_INTERFACE_TRACKNROUTINTERFACE_HPP
#define TRAINTASTIC_SERVER_HARDWARE_INTERFACE_TRACKNROUTINTERFACE_HPP

#include "interface.hpp"
#include "../protocol/tracknroute/kernel.hpp"
#include "../protocol/tracknroute/settings.hpp"
#include "../input/inputcontroller.hpp"
#include "../input/list/inputlist.hpp"
#include "../../core/objectproperty.hpp"

/**
 * @brief Track 'n Route hardware interface
 */
class TracknRouteInterface final
  : public Interface
  , public InputController
{
  CLASS_ID("interface.track_n_route")
  DEFAULT_ID("tnr")
  CREATE(TracknRouteInterface)

  private:
    std::unique_ptr<TracknRoute::Kernel> m_kernel;

    void addToWorld() final;
    void destroying() final;

    void idChanged(const std::string& newId) final;

  protected:
    bool setOnline(bool& value) final;

  public:
    Property<std::string> device;
    ObjectProperty<TracknRoute::Settings> tracknRoute;
    ObjectProperty<InputList> inputs;

    TracknRouteInterface(const std::weak_ptr<World>& world, std::string_view _id);

    // InputController:
    std::pair<uint32_t, uint32_t> inputAddressMinMax() const final { return {TracknRoute::Kernel::inputAddressMin, TracknRoute::Kernel::inputAddressMax}; }
    [[nodiscard]] bool addInput(Input& input) final;
    [[nodiscard]] bool removeInput(Input& input) final;
};

#endif
