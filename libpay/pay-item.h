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


#ifndef LIBPAY_ITEM_H
#define LIBPAY_ITEM_H 1

#include <libpay/pay-types.h>

#include <time.h> /* time_t */

#pragma GCC visibility push(default)

#ifdef __cplusplus
extern "C" {
#endif

/***
****  PayItem Accessors
***/

void                 pay_item_ref                   (PayItem*);

void                 pay_item_unref                 (PayItem*);

bool                 pay_item_get_acknowledged      (const PayItem* item);

time_t               pay_item_get_acknowledged_time (const PayItem* item);

const char*          pay_item_get_description       (const PayItem* item);

const char*          pay_item_get_price             (const PayItem* item);

time_t               pay_item_get_purchased_time    (const PayItem* item);

const char*          pay_item_get_sku               (const PayItem* item);

PayPackageItemStatus pay_item_get_status            (const PayItem* item);

const char*          pay_item_get_title             (const PayItem* item);

PayItemType          pay_item_get_type              (const PayItem* item);


/***
**** Item Enumerators
***/

/**
 * pay_package_get_purchased_items:
 * @package: Package whose purchased items are to be retrieved
 *
 * When done, the caller should unref each PayItem
 * with pay_item_unref() and free the array with free().
 *
 * Return value: a NULL-terminated array of PayItems
 */
PayItem** pay_package_get_purchased_items (PayPackage* package);

/**
 * pay_package_get_item:
 * @package: Package whose item is to be retrieved
 * @sku: The item's sku
 *
 * If a match is found, then when done the caller should
 * unref it with pay_item_unref().
 *
 * Return value: a reffed PayItem, or NULL if no match was found
 */
PayItem*  pay_package_get_item            (PayPackage* package,
                                           const char* sku);

/***
**** Actions
***/
                                          
int pay_item_start_purchase    (PayPackage* package,
                                const char* sku);

#ifdef __cplusplus
}
#endif

#pragma GCC visibility pop

#endif /* LIBPAY_ITEM_H */
