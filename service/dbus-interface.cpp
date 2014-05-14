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
#include <cstring>

#include "proxy-service.h"
#include "proxy-package.h"

class DBusInterfaceImpl
{
public:
    /* Allocated on main thread with init */
    Item::Store::Ptr items;
    std::thread t;

    /* Allocated on thread, and cleaned up there */
    GMainLoop* loop;
    proxyPay* serviceProxy;
    proxyPayPackage* packageProxy;

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
            g_clear_object(&packageProxy);

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

    void busAcquired (GDBusConnection* bus);

    /* We only get this on errors, so we need to throw the exception and
       be done with it. */
    void nameLost ()
    {
        g_main_loop_quit(loop);
        throw std::runtime_error("Unable to get dbus name: 'com.canonical.pay'");
    }

    /* Someone wants to know what packages we have */
    bool listPackages (GDBusMethodInvocation* invocation)
    {
        auto packages = items->listApplications();
        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE_TUPLE);
        g_variant_builder_open(&builder, G_VARIANT_TYPE("ao"));

        for (auto package : packages)
        {
            std::string prefix("/com/canonical/pay/");
            std::string encoded = DBusInterface::encodePath(package);

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
    gchar** subtreeEnumerate (const gchar* path)
    {
        GArray* nodes = g_array_new(TRUE, FALSE, sizeof(gchar*));
        const gchar* node = path + std::strlen("/com/canonical/pay/");

        if (node[0] == '\0')
        {
            auto packages = items->listApplications();
            for (auto package : packages)
            {
                std::string encoded = DBusInterface::encodePath(package);
                gchar* val = g_strdup(encoded.c_str());
                g_array_append_val(nodes, val);
            }
        }
        else
        {
            std::string appname(node);
            std::string decoded = DBusInterface::decodePath(appname);
            auto litems = items->getItems(decoded);
            for (auto item : *litems)
            {
                gchar* val = g_strdup(item.first.c_str());
                g_array_append_val(nodes, val);
            }
        }

        return reinterpret_cast<gchar**>(g_array_free(nodes, FALSE));
    }

    GDBusInterfaceInfo** subtreeIntrospect (const gchar* path, const gchar* node)
    {
        GDBusInterfaceInfo* skelInfo = nullptr;

        skelInfo = g_dbus_interface_skeleton_get_info(G_DBUS_INTERFACE_SKELETON(packageProxy));

        GDBusInterfaceInfo** retval = g_new0(GDBusInterfaceInfo*, 2);
        retval[0] = g_dbus_interface_info_ref(skelInfo);

        return retval;
    }

    void packageCall(const gchar* sender, const gchar* path, const gchar* method, GVariant* params,
                     GDBusMethodInvocation* invocation)
    {
        const gchar* encoded_package = path + std::strlen("/com/canonical/pay/");
        std::string package = DBusInterface::decodePath(std::string(encoded_package));

        if (g_strcmp0(method, "ListItems") == 0)
        {
            GVariantBuilder builder;
            g_variant_builder_init(&builder, G_VARIANT_TYPE_TUPLE);
            g_variant_builder_open(&builder, G_VARIANT_TYPE("ao"));

            auto litems = items->getItems(package);
            for (auto item : *litems)
            {
                std::string encodedItem = DBusInterface::encodePath(item.first.c_str());
                std::string fullpath(path);
                fullpath += "/";
                fullpath += encodedItem;

                g_variant_builder_add_value(&builder, g_variant_new_object_path(fullpath.c_str()));
            }

            g_variant_builder_close(&builder);
            g_dbus_method_invocation_return_value(invocation, g_variant_builder_end(&builder));
        }
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

    static gboolean listPackages_staticHelper (proxyPay* proxy, GDBusMethodInvocation* invocation, gpointer user_data)
    {
        DBusInterfaceImpl* notthis = static_cast<DBusInterfaceImpl*>(user_data);
        return notthis->listPackages(invocation);
    }

    static gchar** subtreeEnumerate_staticHelper (GDBusConnection* bus, const gchar* sender, const gchar* object_path,
                                                  gpointer user_data)
    {
        DBusInterfaceImpl* notthis = static_cast<DBusInterfaceImpl*>(user_data);
        return notthis->subtreeEnumerate(object_path);
    }

    static GDBusInterfaceInfo** subtreeIntrospect_staticHelper (GDBusConnection* bus, const gchar* sender,
                                                                const gchar* object_path, const gchar* node, gpointer user_data)
    {
        DBusInterfaceImpl* notthis = static_cast<DBusInterfaceImpl*>(user_data);
        return notthis->subtreeIntrospect(object_path, node);
    }

    static const GDBusInterfaceVTable* subtreeDispatch_staticHelper (GDBusConnection* bus,
                                                                     const gchar* sender,
                                                                     const gchar* path,
                                                                     const gchar* interface,
                                                                     const gchar* node,
                                                                     gpointer* out_user_data,
                                                                     gpointer user_data);

    static void packageCall_staticHelper (GDBusConnection* connection, const gchar* sender, const gchar* path,
                                          const gchar* interface, const gchar* method, GVariant* params, GDBusMethodInvocation* invocation, gpointer user_data)
    {
        DBusInterfaceImpl* notthis = static_cast<DBusInterfaceImpl*>(user_data);
        return notthis->packageCall(sender, path, method, params, invocation);
    }
};

static const GDBusSubtreeVTable subtreeVtable =
{
    .enumerate = DBusInterfaceImpl::subtreeEnumerate_staticHelper,
    .introspect = DBusInterfaceImpl::subtreeIntrospect_staticHelper,
    .dispatch= DBusInterfaceImpl::subtreeDispatch_staticHelper
};

/* Export objects into the bus before we get a name */
void DBusInterfaceImpl::busAcquired (GDBusConnection* bus)
{
    serviceProxy = proxy_pay_skeleton_new();
    packageProxy = proxy_pay_package_skeleton_new();

    g_signal_connect(G_OBJECT(serviceProxy),
                     "handle-list-packages",
                     G_CALLBACK(listPackages_staticHelper),
                     this);

    g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(serviceProxy),
                                     bus,
                                     "/com/canonical/pay",
                                     NULL);

    g_dbus_connection_register_subtree(bus,
                                       "/com/canonical/pay",
                                       &subtreeVtable,
                                       G_DBUS_SUBTREE_FLAGS_DISPATCH_TO_UNENUMERATED_NODES,
                                       this,
                                       nullptr, /* free func */
                                       nullptr);
}

static const GDBusInterfaceVTable packageVtable =
{
    .method_call = DBusInterfaceImpl::packageCall_staticHelper,
    .get_property = nullptr,
    .set_property = nullptr
};

const GDBusInterfaceVTable* DBusInterfaceImpl::subtreeDispatch_staticHelper (GDBusConnection* bus,
                                                                             const gchar* sender,
                                                                             const gchar* path,
                                                                             const gchar* interface,
                                                                             const gchar* node,
                                                                             gpointer* out_user_data,
                                                                             gpointer user_data)
{
    *out_user_data = user_data;
    const GDBusInterfaceVTable* retval = nullptr;

    if (g_strcmp0(interface, "com.canonical.pay.package") == 0)
    {
        retval = &packageVtable;
    }

    return retval;
}



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

