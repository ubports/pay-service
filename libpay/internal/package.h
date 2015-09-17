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

#pragma once

#include <libpay/pay-item.h>
#include <libpay/pay-package.h>
#include <libpay/proxy-package.h>
#include <libpay/proxy-store.h>

#include <libpay/internal/item.h>

#include <common/glib-thread.h>

#include <core/signal.h>

#include <map>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace Pay
{

namespace Internal
{

class Package
{
    const std::string id;

    std::map <std::pair<PayPackageItemObserver, void*>, core::ScopedConnection> statusObservers;
    std::map <std::pair<PayPackageRefundObserver, void*>, core::ScopedConnection> refundObservers;

    core::Signal<std::string, PayPackageItemStatus, uint64_t> statusChanged;
    std::map <std::string, std::pair<PayPackageItemStatus, uint64_t>> itemStatusCache;
    void updateStatus(const std::string& sku, PayPackageItemStatus);

    GLib::ContextThread thread;
    std::shared_ptr<proxyPayPackage> pkgProxy;
    std::shared_ptr<proxyPayStore> storeProxy;

    constexpr static uint64_t expiretime{60}; // 60 seconds prior status is "expiring"

    template<typename Collection>
    bool removeObserver(Collection& collection, const typename Collection::key_type& key);

    static void pkgProxySignal (proxyPayPackage* /*proxy*/,
                                const gchar* sku,
                                const gchar* statusstr,
                                guint64 refundable_until,
                                gpointer user_data);

    template <typename BusProxy,
              void (*startFunc)(BusProxy*, const gchar*, GCancellable*, GAsyncReadyCallback, gpointer),
              gboolean (*finishFunc)(BusProxy*, GAsyncResult*, GError**)>
    bool startBase (const std::shared_ptr<BusProxy>& bus_proxy, const std::string& sku) noexcept;

    template<typename BusProxy,
             void (*startFunc)(proxyPayStore*, const gchar*, GCancellable*, GAsyncReadyCallback, gpointer),
             gboolean (*finishFunc)(proxyPayStore*, GVariant**, GAsyncResult*, GError**)>
    bool startStoreAction(const std::shared_ptr<BusProxy>& bus_proxy, const std::string& sku) noexcept;

public:
    explicit Package (const std::string& packageid);
    ~Package();

    PayPackageItemStatus itemStatus (const std::string& sku) noexcept;

    PayPackageRefundStatus refundStatus (const std::string& sku) noexcept;

    PayPackageRefundStatus calcRefundStatus (PayPackageItemStatus item, uint64_t refundtime);

    bool addStatusObserver    (PayPackageItemObserver observer, void* user_data) noexcept;
    bool removeStatusObserver (PayPackageItemObserver observer, void* user_data) noexcept;
    bool addRefundObserver    (PayPackageRefundObserver observer, void* user_data) noexcept;
    bool removeRefundObserver (PayPackageRefundObserver observer, void* user_data) noexcept;

    bool startVerification    (const std::string& sku) noexcept;
    bool startPurchase        (const std::string& sku) noexcept;
    bool startRefund          (const std::string& sku) noexcept;
    bool startAcknowledge     (const std::string& sku) noexcept;

    std::shared_ptr<PayItem> getItem(const std::string& sku) noexcept;

    std::vector<std::shared_ptr<PayItem>> getPurchasedItems() noexcept;
};

} // namespace Internal

} // namespace Pay

struct PayPackage_: public Pay::Internal::Package
{
    explicit PayPackage_(const std::string& package_name): Pay::Internal::Package(package_name) {}
};


