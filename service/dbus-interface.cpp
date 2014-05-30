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
    core::Signal<> connectionReady;
    GQuark errorQuark;

    /* Allocated on thread, and cleaned up there */
    GMainLoop* loop;
    proxyPay* serviceProxy;
    proxyPayPackage* packageProxy;
    GDBusConnection* bus;
    GCancellable* cancel;
    guint subtree_registration;

    /* Allocates a thread to do dbus work */
    DBusInterfaceImpl (const Item::Store::Ptr& in_items) :
        items(in_items),
        cancel(g_cancellable_new()),
        loop(nullptr),
        bus(nullptr),
        serviceProxy(nullptr),
        packageProxy(nullptr),
        errorQuark(g_quark_from_static_string("dbus-interface-impl")),
        subtree_registration(0)
    { }

    void run ()
    {
        t = std::thread([this]()
        {
            GMainContext* context = g_main_context_new();
            g_debug("Setting main loop");
            loop = g_main_loop_new(context, FALSE);
            g_debug("Main loop set");

            g_main_context_push_thread_default(context);

            core::ScopedConnection itemupdate(items->itemChanged.connect([this](std::string pkg, std::string item,
                                                                                Item::Item::Status status)
            {
                if (bus == nullptr)
                {
                    return;
                }

                std::string encodedpkg = DBusInterface::encodePath(pkg);
                std::string path("/com/canonical/pay/");
                path += encodedpkg;

                const char* strstatus = Item::Item::statusString(status);

                g_dbus_connection_emit_signal(bus,
                                              nullptr, /* dest */
                                              path.c_str(),
                                              "com.canonical.pay.package",
                                              "ItemStatusChanged",
                                              g_variant_new("(ss)", item.c_str(), strstatus),
                                              nullptr);
            }));

            if (cancel != nullptr && !g_cancellable_is_cancelled(cancel))
            {
                g_bus_own_name(G_BUS_TYPE_SESSION,
                               "com.canonical.pay",
                               G_BUS_NAME_OWNER_FLAGS_NONE,
                               busAcquired_staticHelper,
                               nameAcquired_staticHelper,
                               nameLost_staticHelper,
                               this,
                               nullptr /* free func for this */);
            }

            if (cancel != nullptr && !g_cancellable_is_cancelled(cancel))
            {
                g_main_loop_run(loop);
            }

            if (serviceProxy != nullptr)
            {
                g_dbus_interface_skeleton_unexport(G_DBUS_INTERFACE_SKELETON(serviceProxy));
            }
            if (subtree_registration != 0 && bus != nullptr)
            {
                g_dbus_connection_unregister_subtree(bus, subtree_registration);
                subtree_registration =0;
            }

            g_clear_object(&serviceProxy);
            g_clear_object(&packageProxy);

            g_clear_object(&bus);
            g_clear_pointer(&loop, g_main_loop_unref);
            g_clear_pointer(&context, g_main_context_unref);
        });
    }

    ~DBusInterfaceImpl ()
    {
        g_cancellable_cancel(cancel);
        g_clear_object(&cancel);

        if (loop != nullptr)
        {
            g_debug("Quitting main loop");
            g_main_loop_quit(loop);
        }

        if (t.joinable())
        {
            g_debug("Waiting on thread");
            t.join();
        }
    }

    void busAcquired (GDBusConnection* bus);

    /* Signal up that we're ready on the interface side of things */
    void nameAcquired ()
    {
        connectionReady();
    }

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
        auto packages = items->listApplications();
        for (auto package : packages)
        {
            std::string encoded = DBusInterface::encodePath(package);
            gchar* val = g_strdup(encoded.c_str());
            g_array_append_val(nodes, val);
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
            g_variant_builder_open(&builder, G_VARIANT_TYPE("a(ss)"));

            auto litems = items->getItems(package);
            for (auto item : *litems)
            {
                g_variant_builder_open(&builder, G_VARIANT_TYPE("(ss)"));

                g_variant_builder_add_value(&builder, g_variant_new_string(item.first.c_str()));
                g_variant_builder_add_value(&builder, g_variant_new_string(Item::Item::statusString(item.second->getStatus())));

                g_variant_builder_close(&builder);
            }

            g_variant_builder_close(&builder);
            g_dbus_method_invocation_return_value(invocation, g_variant_builder_end(&builder));
        }
        else if (g_strcmp0(method, "VerifyItem") == 0)
        {
            GVariant* vitemid = g_variant_get_child_value(params, 0);
            std::string itemid(g_variant_get_string(vitemid, NULL));
            g_variant_unref(vitemid);

            auto item = items->getItem(package, itemid);
            if (item->verify())
            {
                g_dbus_method_invocation_return_value(invocation, NULL);
            }
            else
            {
                g_dbus_method_invocation_return_error(invocation, errorQuark, 1, "Unable to verify item '%s'", itemid.c_str());
            }
        }
        else if (g_strcmp0(method, "PurchaseItem") == 0)
        {
            GVariant* vitemid = g_variant_get_child_value(params, 0);
            std::string itemid(g_variant_get_string(vitemid, NULL));
            g_variant_unref(vitemid);

            auto item = items->getItem(package, itemid);
            if (item->purchase())
            {
                g_dbus_method_invocation_return_value(invocation, NULL);
            }
            else
            {
                g_dbus_method_invocation_return_error(invocation, errorQuark, 2, "Unable to purchase item '%s'", itemid.c_str());
            }
        }
    }

    /**************************************
     * Static Helpers, C language binding
     **************************************/
    static void busAcquired_staticHelper (GDBusConnection* inbus, const gchar* name, gpointer user_data)
    {
        DBusInterfaceImpl* notthis = static_cast<DBusInterfaceImpl*>(user_data);
        notthis->busAcquired(inbus);
    }

    static void nameAcquired_staticHelper (GDBusConnection* bus, const gchar* name, gpointer user_data)
    {
        DBusInterfaceImpl* notthis = static_cast<DBusInterfaceImpl*>(user_data);
        notthis->nameAcquired();
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
void DBusInterfaceImpl::busAcquired (GDBusConnection* inbus)
{
    if (inbus == nullptr)
    {
        return;
    }

    bus = reinterpret_cast<GDBusConnection*>(g_object_ref(inbus));

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

    subtree_registration = g_dbus_connection_register_subtree(bus,
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
    impl->connectionReady.connect([this]()
    {
        connectionReady();
    });

    impl->run();
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

