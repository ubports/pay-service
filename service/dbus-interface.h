/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Ted Gould <ted.gould@canonical.com>
 */

#include "item-interface.h"
#include <memory>
#include <core/signal.h>

#ifndef DBUS_INTERFACE_HPP__
#define DBUS_INTERFACE_HPP__ 1

class DBusInterfaceImpl;

class DBusInterface
{
public:
    DBusInterface (const Item::Store::Ptr& in_items);
    ~DBusInterface () { };

    bool busStatus ();

    static std::string encodePath (const std::string& input);
    static std::string decodePath (const std::string& input);

    core::Signal<> connectionReady;

    typedef std::shared_ptr<DBusInterface> Ptr;

private:
    std::shared_ptr<DBusInterfaceImpl> impl;
};


#endif // DBUS_INTERFACE_HPP__
