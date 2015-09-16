/*
 * Copyright © 2014-2015 Canonical Ltd.
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
 * Authors:
 *   Ted Gould <ted@canonical.com>
 *   Charles Kerr <charles.kerr@canonical.com>
 */

#include <libpay/internal/package.h>

#include <common/bus-utils.h>

#include <gio/gio.h>

namespace Pay
{

namespace Internal
{

Package::Package (const std::string& packageid)
    : id(packageid)
    , thread([]{}, [this]{pkgProxy.reset();})
{
    // when item statuses change, update our internal cache
    statusChanged.connect([this](std::string itemid,
                               PayPackageItemStatus status,
                               uint64_t refundable_until)
    {
        g_debug("Updating itemStatusCache for '%s', timeout is: %lld",
                itemid.c_str(), refundable_until);
        itemStatusCache[itemid] = std::make_pair(status, refundable_until);
    });

    /* Fire up a glib thread to create the proxies.
       Block on it here so the proxies are ready before this ctor returns */
    const auto errorStr = thread.executeOnThread<std::string>([this]()
    {
        const auto encoded_id = BusUtils::encodePathElement(id);

        // create the pay-service proxy...
        GError* error = nullptr;
        std::string path = "/com/canonical/pay/" + encoded_id;
        pkgProxy = std::shared_ptr<proxyPayPackage>(
            proxy_pay_package_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                G_DBUS_PROXY_FLAGS_NONE,
                "com.canonical.pay",
                path.c_str(),
                thread.getCancellable().get(),
                &error),
            [](proxyPayPackage * proxy){g_clear_object(&proxy);}
        );
        if (error != nullptr) {
            const std::string tmp { error->message };
            g_clear_error(&error);
            return tmp;
        }
        g_signal_connect(pkgProxy.get(), "item-status-changed", G_CALLBACK(pkgProxySignal), this);

        return std::string(); // no error
    });

    if (!errorStr.empty())
    {
        throw std::runtime_error(errorStr);
    }

    if (!pkgProxy)
    {
        throw std::runtime_error("Unable to build proxy for pay-service");
    }
}

Package::~Package ()
{
    thread.quit();
}

PayPackageItemStatus
Package::itemStatus (const std::string& itemid) noexcept
{
    try
    {
        return itemStatusCache[itemid].first;
    }
    catch (std::out_of_range& /*range*/)
    {
        return PAY_PACKAGE_ITEM_STATUS_UNKNOWN;
    }
}

PayPackageRefundStatus
Package::refundStatus (const std::string& itemid) noexcept
{
    try
    {
        auto entry = itemStatusCache[itemid];
        return calcRefundStatus(entry.first, entry.second);
    }
    catch (std::out_of_range& /*range*/)
    {
        return PAY_PACKAGE_REFUND_STATUS_NOT_REFUNDABLE;
    }
}

PayPackageRefundStatus
Package::calcRefundStatus (PayPackageItemStatus item,
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

/***
****  Observers
***/

template <typename Collection>
bool
Package::removeObserver(Collection& collection, const typename Collection::key_type& key)
{
    bool removed = false;
    auto it = collection.find(key);
    if (it != collection.end())
    {
        collection.erase(it);
        removed = true;
    }
    return removed;
}

bool
Package::addStatusObserver (PayPackageItemObserver observer, void* user_data) noexcept
{
    /* Creates a connection to the signal for the observer and stores the connection
       object in the map so that we can remove it later, or it'll get disconnected
       when the whole object gets destroyed */
    statusObservers.emplace(std::make_pair(observer, user_data), statusChanged.connect([this, observer, user_data] (
        std::string itemid,
        PayPackageItemStatus status,
        uint64_t /*refund*/)
    {
        observer(reinterpret_cast<PayPackage*>(this), itemid.c_str(), status, user_data);
    }));
    return true;
}

bool
Package::removeStatusObserver (PayPackageItemObserver observer, void* user_data) noexcept
{
    return removeObserver(statusObservers, std::make_pair(observer, user_data));
}

bool
Package::addRefundObserver (PayPackageRefundObserver observer, void* user_data) noexcept
{
    refundObservers.emplace(std::make_pair(observer, user_data), statusChanged.connect([this, observer, user_data] (
        std::string itemid,
        PayPackageItemStatus status,
        uint64_t refund)
    {
        observer(reinterpret_cast<PayPackage*>(this), itemid.c_str(), calcRefundStatus(status, refund), user_data);
    }));
    return true;
}

bool
Package::removeRefundObserver (PayPackageRefundObserver observer, void* user_data) noexcept
{
    return removeObserver(refundObservers, std::make_pair(observer, user_data));
}

/***
****
***/

namespace // helper functions
{

PayPackageItemStatus status_from_string(const std::string& str)
{
    if (str == "purchased")
        return PAY_PACKAGE_ITEM_STATUS_PURCHASED;

    if (str == "not purchased")
        return PAY_PACKAGE_ITEM_STATUS_NOT_PURCHASED;

    if (str == "verifying")
        return PAY_PACKAGE_ITEM_STATUS_VERIFYING;

    if (str == "purchasing")
        return PAY_PACKAGE_ITEM_STATUS_PURCHASING;

    if (str == "refunding")
        return PAY_PACKAGE_ITEM_STATUS_REFUNDING;

    if (str == "approved")
        return PAY_PACKAGE_ITEM_STATUS_APPROVED;

    return PAY_PACKAGE_ITEM_STATUS_UNKNOWN;
}

} // anonymous namespace

/***
****
***/

bool
Package::startVerification (const std::string& itemid) noexcept
{
    g_debug("%s %s", G_STRFUNC, itemid.c_str());

    auto ok = startBase<proxyPayPackage,
                        &proxy_pay_package_call_verify_item,
                        &proxy_pay_package_call_verify_item_finish> (pkgProxy, itemid);

    g_debug("%s returning %d", G_STRFUNC, int(ok));
    return ok;
}

bool
Package::startPurchase (const std::string& itemid) noexcept
{
    g_debug("%s %s", G_STRFUNC, itemid.c_str());

    auto ok = startBase<proxyPayPackage,
                        &proxy_pay_package_call_purchase_item,
                        &proxy_pay_package_call_purchase_item_finish> (pkgProxy, itemid);

    g_debug("%s returning %d", G_STRFUNC, int(ok));
    return ok;
}

bool
Package::startRefund (const std::string& itemid) noexcept
{
    g_debug("%s %s", G_STRFUNC, itemid.c_str());

    auto ok = startBase<proxyPayPackage,
                        &proxy_pay_package_call_refund_item,
                        &proxy_pay_package_call_refund_item_finish> (pkgProxy, itemid);

    g_debug("%s returning %d", G_STRFUNC, int(ok));
    return ok;
}

void
Package::pkgProxySignal (proxyPayPackage* /*proxy*/,
                         const gchar* itemid,
                         const gchar* statusstr,
                         guint64 refundable_until,
                         gpointer user_data)
{
    auto notthis = static_cast<Package*>(user_data);
    notthis->statusChanged(itemid,
                           status_from_string(statusstr),
                           refundable_until);
}

template <typename BusProxy,
          void (*startFunc)(BusProxy*, const gchar*, GCancellable*, GAsyncReadyCallback, gpointer),
          gboolean (*finishFunc)(BusProxy*, GAsyncResult*, GError**)>
bool Package::startBase (const std::shared_ptr<BusProxy>& bus_proxy, const std::string& item_id) noexcept
{
    auto async_ready = [](GObject * obj, GAsyncResult * res, gpointer user_data) -> void
    {
        auto prom = static_cast<std::promise<bool>*>(user_data);
        GError* error = nullptr;
        finishFunc(reinterpret_cast<BusProxy*>(obj), res, &error);
        if ((error != nullptr) && !g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        {
            std::cerr << "Error from service: " << error->message << std::endl;
        }
        prom->set_value(error == nullptr);
        g_clear_error(&error);
    };

    std::promise<bool> promise;
    thread.executeOnThread([this, bus_proxy, item_id, &async_ready, &promise]()
    {
        startFunc(bus_proxy.get(),
        item_id.c_str(),
        thread.getCancellable().get(), // GCancellable
        async_ready,
        &promise);
    });

    auto future = promise.get_future();
    future.wait();
    auto call_succeeded = future.get();
    return call_succeeded;
}

} // namespace Internal

} // namespace Pay

