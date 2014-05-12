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
#include <thread>

class DBusInterfaceImpl
{
public:
    /* Allocated on main thread with init */
    Item::Store::Ptr items;
    std::thread t;

    /* Allocated on thread, and cleaned up there */
    GMainLoop* loop;

    /* Allocated on thread, cleaned up on main */
    GDBusConnection* bus;

    /* Allocates a thread to do dbus work */
    DBusInterfaceImpl (const Item::Store::Ptr& in_items) : items(in_items)
    {
        t = std::thread([this]()
        {
            GMainContext* context = g_main_context_new();
            loop = g_main_loop_new(context, FALSE);

            g_main_context_push_thread_default(context);

            g_bus_own_name(G_BUS_TYPE_SESSION,
                           "com.canonical.pay",
                           G_BUS_NAME_OWNER_FLAGS_NONE,
                           busAcquired_staticHelper,
                           nullptr,
                           nameLost_staticHelper,
                           this,
                           nullptr /* free func for this */);

            g_main_loop_run(loop);

            g_clear_pointer(&loop, g_main_loop_unref);
            g_clear_pointer(&context, g_main_context_unref);
        });
    }

    ~DBusInterfaceImpl ()
    {
        if (loop != nullptr)
        {
            g_main_loop_quit(loop);
        }

        if (t.joinable())
        {
            t.join();
        }
    }

    GDBusConnection* getBus (void)
    {
        return bus;
    }

    /* Export objects into the bus before we get a name */
    void busAcquired ()
    {

    }

    /* We only get this on errors, so we need to throw the exception and
       be done with it. */
    void nameLost ()
    {
        throw std::runtime_error("Unable to get dbus name: 'com.canonical.pay'");
    }


    /**************************************
     * Static Helpers, C language binding
     **************************************/
    static void busAcquired_staticHelper (GDBusConnection* bus, const gchar* name, gpointer user_data)
    {
        DBusInterfaceImpl* notthis = static_cast<DBusInterfaceImpl*>(user_data);
        notthis->busAcquired();
    }

    static void nameLost_staticHelper (GDBusConnection* bus, const gchar* name, gpointer user_data)
    {
        DBusInterfaceImpl* notthis = static_cast<DBusInterfaceImpl*>(user_data);
        notthis->nameLost();
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

