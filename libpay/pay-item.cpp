/*
 * Copyright © 2015 Canonical Ltd.
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
 */

#include <libpay/pay-package.h>
#include <libpay/pay-item.h>
#include <libpay/internal/item.h>
#include <libpay/internal/package.h>

/***
****  Life Cycle
***/

void
pay_item_ref (PayItem* item)
{
    g_return_if_fail(item != nullptr);

    item->ref();
}

void
pay_item_unref (PayItem* item)
{
    g_return_if_fail(item != nullptr);

    item->unref();
}

/***
****  Properties
***/

bool
pay_item_get_acknowledged (const PayItem* item)
{
    g_return_val_if_fail(item != nullptr, false);

    return item->acknowledged();
}

time_t
pay_item_get_acknowledged_time (const PayItem* item)
{
    g_return_val_if_fail(item != nullptr, 0);

    return item->acknowledged_time();
}

const char*
pay_item_get_description (const PayItem* item)
{
    g_return_val_if_fail(item != nullptr, nullptr);

    return item->description().c_str();
}

const char*
pay_item_get_sku (const PayItem* item)
{
    g_return_val_if_fail(item != nullptr, nullptr);

    return item->sku().c_str();
}

const char*
pay_item_get_price (const PayItem* item)
{
    g_return_val_if_fail(item != nullptr, nullptr);

    return item->price().c_str();
}

time_t
pay_item_get_purchased_time (const PayItem* item)
{
    g_return_val_if_fail(item != nullptr, 0);

    return item->purchased_time();
}

PayPackageItemStatus
pay_item_get_status (const PayItem* item)
{
    g_return_val_if_fail(item != nullptr, PAY_PACKAGE_ITEM_STATUS_UNKNOWN);

    return item->status();
}

const char*
pay_item_get_title (const PayItem* item)
{
    g_return_val_if_fail(item != nullptr, nullptr);

    return item->title().c_str();
}

PayItemType
pay_item_get_type (const PayItem* item)
{
    g_return_val_if_fail(item != nullptr, PAY_ITEM_TYPE_UNKNOWN);

    return item->type();
}

/***
**** Item Enumerators
***/

PayItem**
pay_package_get_purchased_items (PayPackage* package)
{
    g_return_val_if_fail (package != nullptr, static_cast<PayItem**>(calloc(1,sizeof(PayItem*))));

    auto items = package->getPurchasedItems();
    const auto n = items.size();
    auto ret = static_cast<PayItem**>(calloc(n+1, sizeof(PayItem*))); // +1 to null terminate the array
    for (size_t i=0; i<n; i++)
    {
        auto& item = items[i];
        item->ref(); // caller must unref
        ret[i] = item.get();
    }
    return ret;
}

PayItem*
pay_package_get_item (PayPackage* package,
                      const char* sku)
{
    g_return_val_if_fail (package != nullptr, nullptr);
    g_return_val_if_fail (sku != nullptr, nullptr);
    g_return_val_if_fail (*sku != '\0', nullptr);

    PayItem* ret {};
    auto item = package->getItem(sku);
    if (item) {
        item->ref(); // caller must unref
        ret = item.get();
    }
    return ret; 
}

/***
**** Actions
***/

int
pay_item_start_purchase (PayPackage* package,
                         const char* sku)
{
    g_return_val_if_fail (package != nullptr, 0);
    g_return_val_if_fail (sku != nullptr, 0);
    g_return_val_if_fail (*sku != '\0', 0);

    return package->startItemPurchase(sku) ? 1 : 0;
}

int
pay_item_start_refund (PayPackage* package,
                       const char* sku)
{
    g_return_val_if_fail (package != nullptr, 0);
    g_return_val_if_fail (sku != nullptr, 0);
    g_return_val_if_fail (*sku != '\0', 0);

    return package->startItemRefund(sku) ? 1 : 0;
}
