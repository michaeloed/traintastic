/**
 * server/src/board/tile/rail/blockrailtile.hpp
 *
 * This file is part of the traintastic source code.
 *
 * Copyright (C) 2020-2023 Reinder Feenstra
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

#ifndef TRAINTASTIC_SERVER_BOARD_TILE_RAIL_BLOCKRAILTILE_HPP
#define TRAINTASTIC_SERVER_BOARD_TILE_RAIL_BLOCKRAILTILE_HPP

#include "railtile.hpp"
#include "../../map/node.hpp"
#include "../../../core/method.hpp"
#include "../../../core/objectproperty.hpp"
#include "../../../core/vectorproperty.hpp"
#include "../../../enum/blockstate.hpp"
#include "../../../hardware/input/map/blockinputmap.hpp"

class Train;
class TrainBlockStatus;
class BlockInputMapItem;

class BlockRailTile : public RailTile
{
  CLASS_ID("board_tile.rail.block")
  DEFAULT_ID("block")
  CREATE_DEF(BlockRailTile)

  private:
    Node m_node;

    void updateHeightWidthMax();

  protected:
    void worldEvent(WorldState worldState, WorldEvent worldEvent) final;
    void loaded() final;
    void destroying() final;
    void setRotate(TileRotate value) final;

    void updateState();
    void updateTrainMethodEnabled();
    void setState(BlockState value);

  public:
    boost::signals2::signal<void (const BlockRailTile&, BlockState)> stateChanged;

    Property<std::string> name;
    ObjectProperty<BlockInputMap> inputMap;
    Property<BlockState> state;
    VectorProperty<SensorState> sensorStates;
    ObjectVectorProperty<TrainBlockStatus> trains;
    Method<void(std::shared_ptr<Train>)> assignTrain;
    Method<void()> removeTrain;
    Method<void()> flipTrain;
    Event<const std::shared_ptr<Train>&, const std::shared_ptr<BlockRailTile>&> onTrainAssigned;
    Event<const std::shared_ptr<Train>&, const std::shared_ptr<BlockRailTile>&> onTrainRemoved;

    BlockRailTile(World& world, std::string_view _id);

    std::optional<std::reference_wrapper<const Node>> node() const final { return m_node; }
    std::optional<std::reference_wrapper<Node>> node() final { return m_node; }
    void getConnectors(std::vector<Connector>& connectors) const final;

    void inputItemValueChanged(BlockInputMapItem& item);
};

#endif
