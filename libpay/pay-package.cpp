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

#include <common/glib-thread.h>
#include <common/bus-utils.h>

#include <libpay/proxy-package.h>

namespace Pay
{
class Package
{
    std::string id;
    std::string path;

    std::map <std::pair<PayPackageItemObserver, void*>, core::ScopedConnection> itemObservers;
    std::map <std::pair<PayPackageRefundObserver, void*>, core::ScopedConnection> refundObservers;

    core::Signal<std::string, PayPackageItemStatus, uint64_t> itemChanged;
    std::map <std::string, std::pair<PayPackageItemStatus, uint64_t>> itemStatusCache;

    GLib::ContextThread thread;
    std::shared_ptr<proxyPayPackage> proxy;

    constexpr static uint64_t expiretime{60}; // 60 seconds prior status is "expiring"

public:
    Package (const char* packageid)
        : id(packageid)
        , thread([] {}, [this] {proxy.reset();})
    {
        path = std::string("/com/canonical/pay/");
        path += BusUtils::encodePathElement(id);

        /* Keeps item cache up-to-data as we get signals about it */
        itemChanged.connect([this](std::string itemid,
                                   PayPackageItemStatus status,
                                   uint64_t refundable_until)
        {
            g_debug("Updating itemStatusCache for '%s', timeout is: %lld",
                    itemid.c_str(), refundable_until);
            itemStatusCache[itemid] = std::make_pair(status, refundable_until);
        });

        /* Connect in the proxy now that we've got all the signals setup, let the fun begin! */
        proxy = thread.executeOnThread<std::shared_ptr<proxyPayPackage>>([this]()
        {
            GError* error = nullptr;
            auto proxy = std::shared_ptr<proxyPayPackage>(proxy_pay_package_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                                                                   G_DBUS_PROXY_FLAGS_NONE,
                                                                                                   "com.canonical.pay",
                                                                                                   path.c_str(),
                                                                                                   thread.getCancellable().get(),
                                                                                                   &error),
                                                          [](proxyPayPackage * proxy) -> void
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

        if (!proxy)
        {
            throw std::runtime_error("Unable to build proxy for pay-service");
        }
    }

    ~Package (void)
    {
        thread.quit();
    }

    static void proxySignal (proxyPayPackage* /*proxy*/,
                             const gchar* itemid,
                             const gchar* statusstr,
                             guint64 refundable_until,
                             gpointer user_data)
    {
        Package* notthis = reinterpret_cast<Package*>(user_data);
        notthis->itemChanged(itemid,
                             statusFromString(statusstr),
                             refundable_until);
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
        else if (statusstr == "refunding")
        {
            return PAY_PACKAGE_ITEM_STATUS_REFUNDING;
        }
        else if (statusstr == "approved")
        {
            return PAY_PACKAGE_ITEM_STATUS_APPROVED;
        }
        else
        {
            return PAY_PACKAGE_ITEM_STATUS_UNKNOWN;
        }
    }

    PayPackageItemStatus itemStatus (const char* itemid) noexcept
    {
        try
        {
            return itemStatusCache[itemid].first;
        }
        catch (std::out_of_range range)
        {
            return PAY_PACKAGE_ITEM_STATUS_UNKNOWN;
        }
    }

    PayPackageRefundStatus refundStatus (const char* itemid) noexcept
    {
        try
        {
            auto entry = itemStatusCache[itemid];
            return calcRefundStatus(entry.first, entry.second);
        }
        catch (std::out_of_range range)
        {
            return PAY_PACKAGE_REFUND_STATUS_NOT_REFUNDABLE;
        }
    }

    inline PayPackageRefundStatus calcRefundStatus (PayPackageItemStatus item,
                                                    uint64_t refundtime)
    {
        g_debug("Checking refund status with timeout: %lld", refundtime);

        if (item != PAY_PACKAGE_ITEM_STATUS_PURCHASED)
        {
            return PAY_PACKAGE_REFUND_STATUS_NOT_PURCHASED;
        }

        const auto now = uint64_t(std::time(nullptr));

        if (refundtime < (now + 10u /* seconds */)) // Honestly, they can't refund this quickly anyway
        {
            return PAY_PACKAGE_REFUND_STATUS_NOT_REFUNDABLE;
        }
        if (refundtime < (now + expiretime))
        {
            return PAY_PACKAGE_REFUND_STATUS_WINDOW_EXPIRING;
        }
        return PAY_PACKAGE_REFUND_STATUS_REFUNDABLE;
    }

    bool addItemObserver (PayPackageItemObserver observer, void* user_data) noexcept
    {
        /* Creates a connection to the signal for the observer and stores the connection
           object in the map so that we can remove it later, or it'll get disconnected
           when the whole object gets destroyed */
        itemObservers.emplace(std::make_pair(observer, user_data), itemChanged.connect([this, observer, user_data] (
            std::string itemid,
            PayPackageItemStatus status,
            uint64_t /*refund*/)
        {
            observer(reinterpret_cast<PayPackage*>(this), itemid.c_str(), status, user_data);
        }));
        return true;
    }

    bool removeItemObserver (PayPackageItemObserver observer, void* user_data) noexcept
    {
        std::pair<PayPackageItemObserver, void*> key(observer, user_data);
        itemObservers.erase(key);
        return true;
    }

    bool addRefundObserver (PayPackageRefundObserver observer, void* user_data) noexcept
    {
        refundObservers.emplace(std::make_pair(observer, user_data), itemChanged.connect([this, observer, user_data] (
            std::string itemid,
            PayPackageItemStatus status,
            uint64_t refund)
        {
            observer(reinterpret_cast<PayPackage*>(this), itemid.c_str(), calcRefundStatus(status, refund), user_data);
        }));
        return true;
    }

    bool removeRefundObserver (PayPackageRefundObserver observer, void* user_data) noexcept
    {
        std::pair<PayPackageRefundObserver, void*> key(observer, user_data);
        refundObservers.erase(key);
        return true;
    }

    template <void (*startFunc)(proxyPayPackage*, const gchar*, GCancellable*, GAsyncReadyCallback, gpointer), gboolean (*finishFunc) (proxyPayPackage*, GAsyncResult*, GError**)>
    bool startBase (const char* itemid) noexcept
    {
        std::promise<bool> promise;
        thread.executeOnThread([this, itemid, &promise]()
        {
            startFunc(proxy.get(),
            itemid,
            thread.getCancellable().get(), /* cancellable */
            [](GObject * obj, GAsyncResult * res, gpointer user_data) -> void
            {
                auto promise = reinterpret_cast<std::promise<bool> *>(user_data);
                GError* error = nullptr;

                finishFunc(PROXY_PAY_PACKAGE(obj), res, &error);

                if (error != nullptr)
                {
                    if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
                    {
                        std::cerr << "Error from service: " << error->message << std::endl;
                    }
                    g_clear_error(&error);
                    promise->set_value(false);
                }
                else {
                    promise->set_value(true);
                }
            },
            &promise);
        });

        auto future = promise.get_future();
        future.wait();
        return future.get();
    }

    bool startVerification (const char* itemid) noexcept
    {
        g_debug("%s %s", G_STRFUNC, itemid);

        auto ok = startBase<&proxy_pay_package_call_verify_item,
                            &proxy_pay_package_call_verify_item_finish> (itemid);

        g_debug("%s returning %d", G_STRFUNC, (int)ok);
        return ok;
    }

    bool startPurchase (const char* itemid) noexcept
    {
        g_debug("%s %s", G_STRFUNC, itemid);

        auto ok = startBase<&proxy_pay_package_call_purchase_item,
                            &proxy_pay_package_call_purchase_item_finish> (itemid);

        g_debug("%s returning %d", G_STRFUNC, (int)ok);
        return ok;
    }

    bool startRefund (const char* itemid) noexcept
    {
        g_debug("%s %s", G_STRFUNC, itemid);

        auto ok = startBase<&proxy_pay_package_call_refund_item,
                            &proxy_pay_package_call_refund_item_finish> (itemid);

        g_debug("%s returning %d", G_STRFUNC, (int)ok);
        return ok;
    }

};


} // ns Pay


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
    g_return_val_if_fail(package_name != nullptr, nullptr);

    try
    {
        Pay::Package* ret = new Pay::Package(package_name);
        return reinterpret_cast<PayPackage*>(ret);
    }
    catch (std::runtime_error error)
    {
        return nullptr;
    }
}

void pay_package_delete (PayPackage* package)
{
    g_return_if_fail(package != nullptr);

    auto pkg = reinterpret_cast<Pay::Package*>(package);
    delete pkg;
}

PayPackageItemStatus pay_package_item_status (PayPackage* package,
                                              const char* itemid)
{
    g_return_val_if_fail(package != nullptr, PAY_PACKAGE_ITEM_STATUS_UNKNOWN);
    g_return_val_if_fail(itemid != nullptr, PAY_PACKAGE_ITEM_STATUS_UNKNOWN);

    auto pkg = reinterpret_cast<Pay::Package*>(package);
    return pkg->itemStatus(itemid);
}

int pay_package_item_is_refundable (PayPackage* package,
                                    const char* itemid)
{
    auto status = pay_package_refund_status(package, itemid);
    return (status == PAY_PACKAGE_REFUND_STATUS_REFUNDABLE ||
            status == PAY_PACKAGE_REFUND_STATUS_WINDOW_EXPIRING);
}

PayPackageRefundStatus pay_package_refund_status (PayPackage* package,
                                                  const char* itemid)
{
    g_return_val_if_fail(package != nullptr, PAY_PACKAGE_REFUND_STATUS_NOT_REFUNDABLE);
    g_return_val_if_fail(itemid != nullptr, PAY_PACKAGE_REFUND_STATUS_NOT_REFUNDABLE);

    auto pkg = reinterpret_cast<Pay::Package*>(package);
    return pkg->refundStatus(itemid);
}

int pay_package_item_observer_install (PayPackage* package,
                                       PayPackageItemObserver observer,
                                       void* user_data)
{
    g_return_val_if_fail(package != nullptr, 0);
    g_return_val_if_fail(observer != nullptr, 0);

    auto pkg = reinterpret_cast<Pay::Package*>(package);
    return pkg->addItemObserver(observer, user_data);
}

int pay_package_item_observer_uninstall (PayPackage* package,
                                         PayPackageItemObserver observer,
                                         void* user_data)
{
    g_return_val_if_fail(package != nullptr, 0);
    g_return_val_if_fail(observer != nullptr, 0);

    auto pkg = reinterpret_cast<Pay::Package*>(package);
    return pkg->removeItemObserver(observer, user_data);
}

int pay_package_refund_observer_install (PayPackage* package,
                                         PayPackageRefundObserver observer,
                                         void* user_data)
{
    g_return_val_if_fail(package != nullptr, 0);
    g_return_val_if_fail(observer != nullptr, 0);

    auto pkg = reinterpret_cast<Pay::Package*>(package);
    return pkg->addRefundObserver(observer, user_data);
}

int pay_package_refund_observer_uninstall (PayPackage* package,
                                           PayPackageRefundObserver observer,
                                           void* user_data)
{
    g_return_val_if_fail(package != nullptr, 0);
    g_return_val_if_fail(observer != nullptr, 0);

    auto pkg = reinterpret_cast<Pay::Package*>(package);
    return pkg->removeRefundObserver(observer, user_data);
}

int pay_package_item_start_verification (PayPackage* package,
                                         const char* itemid)
{
    g_return_val_if_fail(package != nullptr, 0);
    g_return_val_if_fail(itemid != nullptr, 0);

    auto pkg = reinterpret_cast<Pay::Package*>(package);
    return pkg->startVerification(itemid);
}

int pay_package_item_start_purchase (PayPackage* package,
                                     const char* itemid)
{
    g_return_val_if_fail(package != nullptr, 0);
    g_return_val_if_fail(itemid != nullptr, 0);

    auto pkg = reinterpret_cast<Pay::Package*>(package);
    return pkg->startPurchase(itemid);
}

int pay_package_item_start_refund (PayPackage* package,
                                   const char* itemid)
{
    g_return_val_if_fail(package != nullptr, 0);
    g_return_val_if_fail(itemid != nullptr, 0);

    auto pkg = reinterpret_cast<Pay::Package*>(package);
    return pkg->startRefund(itemid);
}
