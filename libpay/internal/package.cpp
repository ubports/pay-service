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
    , thread([]{}, [this]{storeProxy.reset();})
{
    // when item statuses change, update our internal cache
    statusChanged.connect([this](const std::string& sku,
                                 PayPackageItemStatus status,
                                 uint64_t refundable_until)
    {
        g_debug("Updating itemStatusCache for '%s', timeout is: %lld",
                sku.c_str(), refundable_until);
        itemStatusCache[sku] = std::make_pair(status, refundable_until);
    });

    /* Fire up a glib thread to create the proxies.
       Block on it here so the proxies are ready before this ctor returns */
    const auto errorStr = thread.executeOnThread<std::string>([this]()
    {
        const auto encoded_id = BusUtils::encodePathElement(id);

        GError* error = nullptr;
        // create the pay-service-ng proxy...
        std::string path = "/com/canonical/pay/store/" + encoded_id;
        storeProxy = std::shared_ptr<proxyPayStore>(
            proxy_pay_store_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                G_DBUS_PROXY_FLAGS_NONE,
                "com.canonical.payments",
                path.c_str(),
                thread.getCancellable().get(),
                &error),
            [](proxyPayStore * store){g_clear_object(&store);}
        );
        if (error != nullptr) {
            const std::string tmp { error->message };
            g_clear_error(&error);
            return tmp;
        }

        return std::string(); // no error
    });

    if (!errorStr.empty())
    {
        throw std::runtime_error(errorStr);
    }

    if (!storeProxy)
    {
        throw std::runtime_error("Unable to build proxy for pay-service");
    }
}

Package::~Package ()
{
    thread.quit();
}

PayPackageItemStatus
Package::itemStatus (const std::string& sku) noexcept
{
    try
    {
        return itemStatusCache[sku].first;
    }
    catch (std::out_of_range& /*range*/)
    {
        return PAY_PACKAGE_ITEM_STATUS_UNKNOWN;
    }
}

PayPackageRefundStatus
Package::refundStatus (const std::string& sku) noexcept
{
    try
    {
        auto entry = itemStatusCache[sku];
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
        const std::string& sku,
        PayPackageItemStatus status,
        uint64_t /*refund*/)
    {
        observer(reinterpret_cast<PayPackage*>(this), sku.c_str(), status, user_data);
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
        const std::string& sku,
        PayPackageItemStatus status,
        uint64_t refund)
    {
        observer(reinterpret_cast<PayPackage*>(this), sku.c_str(), calcRefundStatus(status, refund), user_data);
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

PayItemType type_from_string(const std::string& str)
{
    if (str == "consumable")
        return PAY_ITEM_TYPE_CONSUMABLE;

    if (str == "unlockable")
        return PAY_ITEM_TYPE_UNLOCKABLE;

    return PAY_ITEM_TYPE_UNKNOWN;
}

std::shared_ptr<PayItem> create_pay_item_from_variant(GVariant* item_properties)
{
    std::shared_ptr<PayItem> item;

    // test the inputs
    g_return_val_if_fail(item_properties != nullptr, item);
    g_return_val_if_fail(g_variant_is_of_type(item_properties, G_VARIANT_TYPE_VARDICT), item);

    // make sure we've got a valid sku to construct the PayItem with
    const char* sku {};
    g_variant_lookup(item_properties, "sku", "&s", &sku);
    if (sku == nullptr)
    {
        g_variant_lookup(item_properties, "package_name", "&s", &sku);
    }
    g_return_val_if_fail(sku != nullptr, item);
    g_return_val_if_fail(*sku != '\0', item);

    auto pay_item_deleter = [](PayItem* p){p->unref();};
    item.reset(new PayItem(sku), pay_item_deleter);

    // now loop through the dict to build the PayItem's properties
    GVariantIter iter;
    gchar* key;
    GVariant* value;
    g_variant_iter_init(&iter, item_properties);
    while (g_variant_iter_loop(&iter, "{sv}", &key, &value))
    {
        if (!g_strcmp0(key, "acknowledged_timestamp"))
        {
            item->set_acknowledged_time(g_variant_get_uint64(value));
        }
        else if (!g_strcmp0(key, "description"))
        {
            item->set_description(g_variant_get_string(value, nullptr));
        }
        else if (!g_strcmp0(key, "sku") || !g_strcmp0(key, "package_name"))
        {
            // no-op; we handled the sku/package_name first
        }
        else if (!g_strcmp0(key, "price"))
        {
            item->set_price(g_variant_get_string(value, nullptr));
        }
        else if (!g_strcmp0(key, "purchased_time") || !g_strcmp0(key, "completed_timestamp"))
        {
            item->set_purchased_time(g_variant_get_uint64(value));
        }
        else if (!g_strcmp0(key, "state"))
        {
            auto state = g_variant_get_string(value, nullptr);
            if (!g_strcmp0(state, "purchased") ||
                !g_strcmp0(state, "approved"))
            {
                item->set_status(PAY_PACKAGE_ITEM_STATUS_PURCHASED);
            }
            else
            {
                item->set_status(PAY_PACKAGE_ITEM_STATUS_NOT_PURCHASED);
            }
        }
        else if (!g_strcmp0(key, "type"))
        {
            item->set_type(type_from_string(g_variant_get_string(value, nullptr)));
        }
        else if (!g_strcmp0(key, "title"))
        {
            item->set_title(g_variant_get_string(value, nullptr));
        }
        else
        {
            auto valstr = g_variant_print(value, true);
            g_warning("Unhandled item property '%s': '%s'", key, valstr);
            g_free(valstr);
        }
    }

    return item;
}

} // anonymous namespace

/***
****  IAP
***/

std::shared_ptr<PayItem>
Package::getItem(const std::string& sku) noexcept
{
    struct CallbackData
    {
        GVariant* properties {};
        std::promise<bool> promise;

        ~CallbackData()
        {
            g_clear_pointer(&properties, g_variant_unref);
        }
    };

    CallbackData data;

    auto on_async_ready = [](GObject* o, GAsyncResult* res, gpointer gdata)
    {
        auto data = static_cast<CallbackData*>(gdata);
        GError* error = nullptr;
        proxy_pay_store_call_get_item_finish(PROXY_PAY_STORE(o), &data->properties, res, &error);
        if ((error != nullptr) && !g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        {
            std::cerr << "Error getting item: " << error->message << std::endl;
        }

        data->promise.set_value(error == nullptr);
        g_clear_error(&error);
    };

    auto thread_func = [this, sku, &on_async_ready, &data]()
    {
        proxy_pay_store_call_get_item(storeProxy.get(),
                                      sku.c_str(),
                                      thread.getCancellable().get(), // GCancellable
                                      on_async_ready,
                                      &data);
    };
    thread.executeOnThread(thread_func);

    auto future = data.promise.get_future();
    future.wait();
    return create_pay_item_from_variant(data.properties);
}

std::vector<std::shared_ptr<PayItem>>
Package::getPurchasedItems() noexcept
{
    struct CallbackData
    {
        GVariant* v {};
        std::promise<bool> promise;

        ~CallbackData()
        {
            g_clear_pointer(&v, g_variant_unref);
        }
    };

    CallbackData data;

    auto on_async_ready = [](GObject* o, GAsyncResult* res, gpointer gdata)
    {
        auto data = static_cast<CallbackData*>(gdata);

        GError* error {};
        proxy_pay_store_call_get_purchased_items_finish(PROXY_PAY_STORE(o), &data->v, res, &error);
        if ((error != nullptr) && !g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        {
            std::cerr << "Error getting purchased items: " << error->message << std::endl;
        }

        data->promise.set_value(error == nullptr);
        g_clear_error(&error);
    };

    auto thread_func = [this, &on_async_ready, &data]()
    {
        proxy_pay_store_call_get_purchased_items(storeProxy.get(),
                                                 thread.getCancellable().get(), // GCancellable
                                                 on_async_ready,
                                                 &data);
    };
    thread.executeOnThread(thread_func);
    auto future = data.promise.get_future();
    future.wait();

    auto v = data.v;
    std::vector<std::shared_ptr<PayItem>> items;
    GVariantIter iter;
    g_variant_iter_init(&iter, v);
    GVariant* child;
    while ((child = g_variant_iter_next_value(&iter)))
    {
        auto item = create_pay_item_from_variant(child);
        if (item)
        {
            items.push_back(item);
        }
        g_variant_unref(child);
    }

    return items;
}

/**
 * We call com.canonical.pay.store's Purchase, Refund, and Acknowledge
 * items in nearly identical ways: make the call asynchronously, and
 * when the service responds, update our status cache with the returned item.
 *
 * This method folds together the common code for these actions.
 */
template<typename BusProxy,
         gboolean (*finish_func)(BusProxy*, GVariant**, GAsyncResult*, GError**)>
bool Package::startStoreAction(const std::shared_ptr<BusProxy>& bus_proxy,
                               const gchar* function_name,
                               GVariant* params,
                               gint timeout_msec) noexcept
{
    auto on_async_ready = [](GObject* o, GAsyncResult* res, gpointer gself)
    {
        GError* error {};
        GVariant* v {};
        finish_func(reinterpret_cast<BusProxy*>(o), &v, res, &error);
        if (error == nullptr)
        {
            auto item = create_pay_item_from_variant(v);
            if (item)
            {
                const auto sku = item->sku();
                const auto status = item->status();
                uint64_t refund_timeout{0};

                auto rv = g_variant_lookup_value(v, "refundable_until",
                                                 G_VARIANT_TYPE_UINT64);
                if (rv != nullptr)
                {
                    refund_timeout = g_variant_get_uint64(rv);
                    g_variant_unref(rv);
                }
                static_cast<Package*>(gself)->statusChanged(sku, status, refund_timeout);
            }
        }
        else if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        {
           std::cerr << "Error calling method: " << error->message << std::endl;
        }
        g_clear_error(&error);
        g_clear_pointer(&v, g_variant_unref);
    };

    thread.executeOnThread([this, bus_proxy, function_name, params,
                            timeout_msec, &on_async_ready]()
    {
        g_dbus_proxy_call(G_DBUS_PROXY(bus_proxy.get()),
                          function_name,
                          params,
                          G_DBUS_CALL_FLAGS_NONE,
                          timeout_msec,
                          thread.getCancellable().get(), // GCancellable
                          on_async_ready,
                          this);
    });

    return true;
}

/***
****
***/

bool
Package::startVerification (const std::string& sku) noexcept
{
    g_debug("%s %s", G_STRFUNC, sku.c_str());

    auto ok = startStoreAction<proxyPayStore,
                               &proxy_pay_store_call_get_item_finish> (
        storeProxy,
        "GetItem",
        g_variant_new("(s)", sku.c_str()),
        -1);

    g_debug("%s returning %d", G_STRFUNC, int(ok));
    return ok;
}

bool
Package::startPurchase (const std::string& sku) noexcept
{
    g_debug("%s %s", G_STRFUNC, sku.c_str());

    statusChanged(sku, PAY_PACKAGE_ITEM_STATUS_PURCHASING, 0);

    auto ok = startStoreAction<proxyPayStore,
                               &proxy_pay_store_call_purchase_item_finish> (
        storeProxy,
        "PurchaseItem",
        g_variant_new("(s)", sku.c_str()),
        300 * G_USEC_PER_SEC);

    g_debug("%s returning %d", G_STRFUNC, int(ok));
    return ok;
}

bool
Package::startRefund (const std::string& sku) noexcept
{
    g_debug("%s %s", G_STRFUNC, sku.c_str());

    auto ok = startStoreAction<proxyPayStore,
                               &proxy_pay_store_call_refund_item_finish> (
        storeProxy,
        "RefundItem",
        g_variant_new("(s)", sku.c_str()),
        -1);

    g_debug("%s returning %d", G_STRFUNC, int(ok));
    return ok;
}

bool
Package::startAcknowledge (const std::string& sku) noexcept
{
    g_debug("%s %s", G_STRFUNC, sku.c_str());

    auto ok = startStoreAction<proxyPayStore,
                               &proxy_pay_store_call_acknowledge_item_finish> (
        storeProxy,
        "AcknowledgeItem",
        g_variant_new("(s)", sku.c_str()),
        -1);

    g_debug("%s returning %d", G_STRFUNC, int(ok));
    return ok;
}

template <typename BusProxy,
          void (*startFunc)(BusProxy*, const gchar*, GCancellable*, GAsyncReadyCallback, gpointer),
          gboolean (*finishFunc)(BusProxy*, GAsyncResult*, GError**)>
bool Package::startBase (const std::shared_ptr<BusProxy>& bus_proxy, const std::string& sku) noexcept
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
    thread.executeOnThread([this, bus_proxy, sku, &async_ready, &promise]()
    {
        startFunc(bus_proxy.get(),
        sku.c_str(),
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

