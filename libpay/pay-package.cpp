/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Ted Gould <ted@canonical.com>
 */

#include "pay-package.h"
#include <string>
#include <map>
#include <thread>
#include <core/signal.h>
#include <gio/gio.h>
#include <mutex>

#include "proxy-package.h"
#include "glib-thread.h"

namespace Pay
{
class Package
{
    std::string id;
    std::string path;
    /* NOTE: Using the shared_ptr here because gcc 4.7 map doesn't have emplace */
    std::map <std::pair<PayPackageItemObserver, void*>, std::shared_ptr<core::ScopedConnection>> observers;
    core::Signal<std::string, PayPackageItemStatus> itemChanged;
    std::map <std::string, PayPackageItemStatus> itemStatusCache;
    std::mutex context_mutex;

    GLib::ContextThread thread;
    std::shared_ptr<proxyPayPackage> proxy;

public:
    Package (const char* packageid)
        : id(packageid)
        , thread([] {}, [this] {proxy.reset();})
    {
        path = std::string("/com/canonical/pay/");
        path += encodePath(id);

        /* Keeps item cache up-to-data as we get signals about it */
        itemChanged.connect([this](std::string itemid, PayPackageItemStatus status)
        {
            itemStatusCache[itemid] = status;
        });


        proxy = thread.executeOnThread<std::shared_ptr<proxyPayPackage>>([this]
        {

            GError* error = nullptr;
            auto proxy = std::shared_ptr<proxyPayPackage>(proxy_pay_package_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
            G_DBUS_PROXY_FLAGS_NONE,
            "com.canonical.pay",
            path.c_str(),
            nullptr,
            &error),
            [](proxyPayPackage * proxy)
            {
                g_clear_object(&proxy);
            });

            if (error != nullptr)
            {
                throw std::runtime_error(error->message);
            }

            g_signal_connect(proxy.get(), "item-status-changed", G_CALLBACK(proxySignal), this);

            return proxy;
        });
    }

    ~Package (void)
    {
        thread.quit();
    }

    static void proxySignal (proxyPayPackage* proxy, const gchar* itemid, const gchar* statusstr, gpointer user_data)
    {
        Package* notthis = reinterpret_cast<Package*>(user_data);
        notthis->itemChanged(itemid, statusFromString(statusstr));
    }

    inline static PayPackageItemStatus statusFromString (std::string statusstr)
    {
        if (statusstr == "purchased")
        {
            return PAY_PACKAGE_ITEM_STATUS_PURCHASED;
        }
        else if (statusstr == "not purchased")
        {
            return PAY_PACKAGE_ITEM_STATUS_NOT_PURCHASED;
        }
        else if (statusstr == "verifying")
        {
            return PAY_PACKAGE_ITEM_STATUS_VERIFYING;
        }
        else if (statusstr == "purchasing")
        {
            return PAY_PACKAGE_ITEM_STATUS_PURCHASING;
        }
        else
        {
            return PAY_PACKAGE_ITEM_STATUS_UNKNOWN;
        }
    }

    PayPackageItemStatus itemStatus (const char* itemid)
    {
        try
        {
            return itemStatusCache[itemid];
        }
        catch (std::out_of_range range)
        {
            return PAY_PACKAGE_ITEM_STATUS_UNKNOWN;
        }
    }

    bool addItemObserver (PayPackageItemObserver observer, void* user_data)
    {
        /* Creates a connection to the signal for the observer and stores the connection
           object in the map so that we can remove it later, or it'll get disconnected
           when the whole object gets destroyed */
        std::pair<PayPackageItemObserver, void*> key(observer, user_data);
        auto connection = std::make_shared<core::ScopedConnection>(itemChanged.connect([this, observer, user_data] (
                                                                                           std::string itemid,
                                                                                           PayPackageItemStatus status)
        {
            observer(reinterpret_cast<PayPackage*>(this), itemid.c_str(), status, user_data);
        }));
        observers[key] = connection;
        return true;
    }

    bool removeItemObserver (PayPackageItemObserver observer, void* user_data)
    {
        std::pair<PayPackageItemObserver, void*> key(observer, user_data);
        observers.erase(key);
        return true;
    }

    bool startVerification (const char* itemid)
    {
        std::string itemidcopy(itemid);
        thread.executeOnThread([this, itemidcopy]
        {
            proxy_pay_package_call_verify_item(proxy.get(),
            itemidcopy.c_str(),
            nullptr, /* cancellable */
            [](GObject * obj, GAsyncResult * res, gpointer user_data) -> void {
                GError* error = nullptr;
                proxy_pay_package_call_verify_item_finish(PROXY_PAY_PACKAGE(obj),
                res,
                &error);

                if (error != nullptr)
                {
                    std::cerr << "Error from dbus message to service: " << error->message << std::endl;
                    g_clear_error(&error);
                }
            },
            nullptr);

        });
    }

    bool startPurchase (const char* itemid)
    {
        std::string itemidcopy(itemid);
        thread.executeOnThread([this, itemidcopy]
        {
            proxy_pay_package_call_purchase_item(proxy.get(),
            itemidcopy.c_str(),
            nullptr, /* cancellable */
            [](GObject * obj, GAsyncResult * res, gpointer user_data) -> void {
                GError* error = nullptr;
                proxy_pay_package_call_purchase_item_finish(PROXY_PAY_PACKAGE(obj),
                res,
                &error);

                if (error != nullptr)
                {
                    std::cerr << "Error from dbus message to service: " << error->message << std::endl;
                    g_clear_error(&error);
                }
            },
            nullptr);

        });
    }

    std::string
    encodePath (const std::string& input)
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
};



}; // ns Pay


/**************************
 * Public API
 **************************/
struct PayPackage_
{
    int dummy;
};

PayPackage*
pay_package_new (const char* package_name)
{
    Pay::Package* ret = new Pay::Package(package_name);
    return reinterpret_cast<PayPackage*>(ret);
}

void pay_package_delete (PayPackage* package)
{
    auto pkg = reinterpret_cast<Pay::Package*>(package);
    delete pkg;
}

PayPackageItemStatus pay_package_item_status (PayPackage* package,
                                              const char* itemid)
{

    auto pkg = reinterpret_cast<Pay::Package*>(package);
    return pkg->itemStatus(itemid);
}

int pay_package_item_observer_install (PayPackage* package,
                                       PayPackageItemObserver observer,
                                       void* user_data)
{
    auto pkg = reinterpret_cast<Pay::Package*>(package);
    return pkg->addItemObserver(observer, user_data);
}

int pay_package_item_observer_uninstall (PayPackage* package,
                                         PayPackageItemObserver observer,
                                         void* user_data)
{
    auto pkg = reinterpret_cast<Pay::Package*>(package);
    return pkg->removeItemObserver(observer, user_data);
}

int pay_package_item_start_verification (PayPackage* package,
                                         const char* itemid)
{
    auto pkg = reinterpret_cast<Pay::Package*>(package);
    return pkg->startVerification(itemid);
}

int pay_package_item_start_purchase (PayPackage* package,
                                     const char* itemid)
{
    auto pkg = reinterpret_cast<Pay::Package*>(package);
    return pkg->startPurchase(itemid);
}
