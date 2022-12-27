/**
 * server/src/core/objectlisttablemodel.hpp
 *
 * This file is part of the traintastic source code.
 *
 * Copyright (C) 2019-2021 Reinder Feenstra
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

#ifndef TRAINTASTIC_SERVER_CORE_OBJECTLISTTABLEMODEL_HPP
#define TRAINTASTIC_SERVER_CORE_OBJECTLISTTABLEMODEL_HPP

#include "tablemodel.hpp"
//#include "objectlist.hpp"
template<typename T>
class ObjectList;

template<typename T>
class ObjectListTableModel : public TableModel
{
  friend class ObjectList<T>;

  private:
    std::shared_ptr<ObjectList<T>> m_list;

  protected:
    static constexpr uint32_t invalidColumn = std::numeric_limits<uint32_t>::max();

    const T& getItem(uint32_t row) const { return *m_list->m_items[row]; }
    virtual void propertyChanged(BaseProperty& property, uint32_t row) = 0;

  public:
    ObjectListTableModel(ObjectList<T>& list) :
      TableModel(),
      m_list{std::static_pointer_cast<ObjectList<T>>(list.shared_from_this())}
    {
      list.m_models.push_back(this);
      setRowCount(static_cast<uint32_t>(list.m_items.size()));
    }

    ~ObjectListTableModel() override
    {
      auto it = std::find(m_list->m_models.begin(), m_list->m_models.end(), this);
      assert(it != m_list->m_models.end());
      m_list->m_models.erase(it);
    }
};

#endif
