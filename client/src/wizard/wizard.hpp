/**
 * client/src/wizard/wizard.hpp
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

#ifndef TRAINTASTIC_CLIENT_WIZARD_WIZARD_HPP
#define TRAINTASTIC_CLIENT_WIZARD_WIZARD_HPP

#include <QWizard>

class AbstractProperty;
class TextPage;
class PropertyPage;

class Wizard : public QWizard
{
  public:
    explicit Wizard(QWidget* parent = nullptr);

  protected:
    TextPage* addTextPage(const QString& title, const QString& text);
    PropertyPage* addPropertyPage(const QString& title, const QString& text, std::initializer_list<AbstractProperty*> properties);
    inline PropertyPage* addPropertyPage(const QString& title, const QString& text, AbstractProperty* property)
    {
      return addPropertyPage(title, text, std::initializer_list<AbstractProperty*>{property});
    }
};

#endif
