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

time_t
pay_item_get_acknowledged_timestamp (const PayItem* item)
{
    g_return_val_if_fail(item != nullptr, 0);

    return item->acknowledged_timestamp();
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
pay_item_get_completed_timestamp (const PayItem* item)
{
    g_return_val_if_fail(item != nullptr, 0);

    return item->completed_timestamp();
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

uint64_t
pay_item_get_purchase_id (const PayItem* item)
{
    g_return_val_if_fail(item != nullptr, 0);

    return item->purchase_id();
}

