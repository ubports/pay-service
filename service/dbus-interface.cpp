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

#include "dbus-interface.h"

#include <gio/gio.h>

class DBusInterfaceImpl
{
public:
    Item::Store::Ptr items;
    GDBusConnection* bus;

    DBusInterfaceImpl (const Item::Store::Ptr& in_items) : items(in_items)
    {
        bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    }

    ~DBusInterfaceImpl ()
    {
        g_clear_object(&bus);
    }

    GDBusConnection* getBus (void)
    {
        return bus;
    }
};

DBusInterface::DBusInterface (const Item::Store::Ptr& in_items)
{
    impl = std::make_shared<DBusInterfaceImpl>(in_items);
}

bool
DBusInterface::busStatus ()
{
    return impl->getBus() != nullptr;
}

std::string
DBusInterface::encodePath (const std::string& input)
{
    std::string output = "";
    bool first = true;

    for (unsigned char c : input)
    {
        std::string retval;

        if ((c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9' && !first))
        {
            retval = std::string((char*)&c, 1);
        }
        else
        {
            char buffer[5] = {0};
            std::snprintf(buffer, 4, "_%2X", c);
            retval = std::string(buffer);
        }

        output += retval;
        first = false;
    }

    return output;
}

std::string
DBusInterface::decodePath (const std::string& input)
{
    std::string output;

    try
    {
        for (int i = 0; i < input.size(); i++)
        {
            if (input[i] == '_')
            {
                char buffer[3] = {0};
                buffer[0] = input[i + 1];
                buffer[1] = input[i + 2];

                unsigned char value = std::stoi(buffer, nullptr, 16);
                output += value;
                i += 2;
            }
            else
            {
                output += input[i];
            }
        }
    }
    catch (...)
    {
        /* We can get out of bounds on the parsing if the
           string is invalid. Just return what we have. */
    }

    return output;
}

