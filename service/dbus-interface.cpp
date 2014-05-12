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

#include "service-proxy.h"

class DBusInterfaceImpl
{
public:
    /* Allocated on main thread with init */
    Item::Store::Ptr items;
    std::thread t;

    /* Allocated on thread, and cleaned up there */
    GMainLoop* loop;
    proxyPay* serviceProxy;

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

            g_clear_object(&serviceProxy);

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

    /* Export objects into the bus before we get a name */
    void busAcquired (GDBusConnection* bus)
    {
        serviceProxy = proxy_pay_skeleton_new();
        g_signal_connect(G_OBJECT(serviceProxy),
                         "handle-list-applications",
                         G_CALLBACK(listApplications_staticHelper),
                         this);
        g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(serviceProxy),
                                         bus,
                                         "/com/canonical/pay",
                                         NULL);

    }

    /* We only get this on errors, so we need to throw the exception and
       be done with it. */
    void nameLost ()
    {
        g_main_loop_quit(loop);
        throw std::runtime_error("Unable to get dbus name: 'com.canonical.pay'");
    }

    /* Someone wants to know what applications we have */
    bool listApplications (GDBusMethodInvocation* invocation)
    {
        auto applications = items->listApplications();
        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE_TUPLE);
        g_variant_builder_open(&builder, G_VARIANT_TYPE("ao"));

        for (auto application : applications)
        {
            std::string prefix("/com/canonical/pay/");
            std::string encoded = DBusInterface::encodePath(application);

            prefix += encoded;

            g_variant_builder_add_value(&builder, g_variant_new_object_path(prefix.c_str()));
        }

        g_variant_builder_close(&builder); // tuple
        g_dbus_method_invocation_return_value(invocation, g_variant_builder_end(&builder));
        return true;
    }

    /**************************************
     * Subtree Functions
     **************************************/
    gchar** subtreeEnumerate (void)
    {
        /* TODO: Yes */
    }

    GDBusInterfaceInfo** subtreeIntrospect (void)
    {
        /* TODO: this */
    }

    void applicationCall(const gchar* sender, const gchar* path, const gchar* method, GVariant* params,
                         GDBusMethodInvocation* invocation)
    {

    }

    void itemCall(const gchar* sender, const gchar* path, const gchar* method, GVariant* params,
                  GDBusMethodInvocation* invocation)
    {

    }

    /**************************************
     * Static Helpers, C language binding
     **************************************/
    static void busAcquired_staticHelper (GDBusConnection* bus, const gchar* name, gpointer user_data)
    {
        DBusInterfaceImpl* notthis = static_cast<DBusInterfaceImpl*>(user_data);
        notthis->busAcquired(bus);
    }

    static void nameLost_staticHelper (GDBusConnection* bus, const gchar* name, gpointer user_data)
    {
        DBusInterfaceImpl* notthis = static_cast<DBusInterfaceImpl*>(user_data);
        notthis->nameLost();
    }

    static gboolean listApplications_staticHelper (proxyPay* proxy, GDBusMethodInvocation* invocation, gpointer user_data)
    {
        DBusInterfaceImpl* notthis = static_cast<DBusInterfaceImpl*>(user_data);
        return notthis->listApplications(invocation);
    }

    static gchar** subtreeEnumerate_staticHelper (GDBusConnection* bus, const gchar* sender, const gchar* object_path,
                                                  gpointer user_data)
    {
        DBusInterfaceImpl* notthis = static_cast<DBusInterfaceImpl*>(user_data);
        return notthis->subtreeEnumerate();
    }

    static GDBusInterfaceInfo** subtreeIntrospect_staticHelper (GDBusConnection* bus, const gchar* sender,
                                                                const gchar* object_path, const gchar* node, gpointer user_data)
    {
        DBusInterfaceImpl* notthis = static_cast<DBusInterfaceImpl*>(user_data);
        return notthis->subtreeIntrospect();
    }

    static const GDBusInterfaceVTable* subtreeDispatch_staticHelper (GDBusConnection* bus, const gchar* sender,
                                                                     const gchar* path, const gchar* interface, const gchar* node, gpointer* out_user_data, gpointer user_data)
    {
        *out_user_data = user_data;
        const GDBusInterfaceVTable* retval = nullptr;

        if (g_strcmp0(interface, "com.caonical.pay.application") == 0)
        {
            retval = &applicationVtable;
        }
        else if (g_strcmp0(interface, "com.caonical.pay.item") == 0)
        {
            retval = &itemVtable;
        }

        return retval;
    }

    static constexpr GDBusSubtreeVTable subtreeVtable =
    {
enumerate:
        subtreeEnumerate_staticHelper,
introspect:
        subtreeIntrospect_staticHelper,
dispatch:
        subtreeDispatch_staticHelper
    };

    static void applicationCall_staticHelper (GDBusConnection* connection, const gchar* sender, const gchar* path,
                                              const gchar* interface, const gchar* method, GVariant* params, GDBusMethodInvocation* invocation, gpointer user_data)
    {
        DBusInterfaceImpl* notthis = static_cast<DBusInterfaceImpl*>(user_data);
        return notthis->applicationCall(sender, path, method, params, invocation);
    }

    static constexpr GDBusInterfaceVTable applicationVtable =
    {
method_call:
        applicationCall_staticHelper,
get_property:
        nullptr,
set_property:
        nullptr
    };

    static void itemCall_staticHelper (GDBusConnection* connection, const gchar* sender, const gchar* path,
                                       const gchar* interface, const gchar* method, GVariant* params, GDBusMethodInvocation* invocation, gpointer user_data)
    {
        DBusInterfaceImpl* notthis = static_cast<DBusInterfaceImpl*>(user_data);
        return notthis->itemCall(sender, path, method, params, invocation);
    }

    static constexpr GDBusInterfaceVTable itemVtable =
    {
method_call:
        itemCall_staticHelper,
get_property:
        nullptr,
set_property:
        nullptr
    };
};

DBusInterface::DBusInterface (const Item::Store::Ptr& in_items)
{
    impl = std::make_shared<DBusInterfaceImpl>(in_items);
}

bool
DBusInterface::busStatus ()
{
    return true;
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

