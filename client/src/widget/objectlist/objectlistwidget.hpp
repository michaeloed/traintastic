/**
 * client/src/widget/objectlist/objectlistwidget.hpp
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

#ifndef TRAINTASTIC_CLIENT_WIDGET_OBJECTLIST_OBJECTLISTWIDGET_HPP
#define TRAINTASTIC_CLIENT_WIDGET_OBJECTLIST_OBJECTLISTWIDGET_HPP

#include <QWidget>
#include "../../network/objectptr.hpp"

class QToolBar;
class QToolButton;
class TableWidget;
class MethodAction;

class ObjectListWidget : public QWidget
{
  Q_OBJECT

  private:
    //const QString m_id;
    int m_requestId;
    int m_requestIdAdd;
    ObjectPtr m_object;
    QToolBar* m_toolbar;
    QToolButton* m_buttonAdd;
    QAction* m_actionAdd;
    QAction* m_actionEdit;
    MethodAction* m_actionDelete;
    TableWidget* m_tableWidget;

    void tableSelectionChanged();

  private slots:
    void tableDoubleClicked(const QModelIndex& index);

  protected:
    const ObjectPtr& object() { return m_object; }
    QToolBar* toolbar() { return m_toolbar; }

    virtual void tableSelectionChanged(bool hasSelection);
    virtual void objectDoubleClicked(const QString& id);
    QStringList getSelectedObjectIds() const;

  public:
    explicit ObjectListWidget(const ObjectPtr& object, QWidget* parent = nullptr);
    ~ObjectListWidget() override;
};

#endif