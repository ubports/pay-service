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

#include <stdint.h> /* uint64_t */
#include <time.h> /* time_t */

#pragma GCC visibility push(default)

#ifdef __cplusplus
extern "C" {
#endif

/***
****  PayItem Accessors
***/

void                 pay_item_ref                        (PayItem*);

void                 pay_item_unref                      (PayItem*);

const char*          pay_item_get_description            (const PayItem* item);

const char*          pay_item_get_price                  (const PayItem* item);

const char*          pay_item_get_sku                    (const PayItem* item);

PayPackageItemStatus pay_item_get_status                 (const PayItem* item);

time_t               pay_item_get_completed_timestamp    (const PayItem* item);

time_t               pay_item_get_acknowledged_timestamp (const PayItem* item);

const char*          pay_item_get_title                  (const PayItem* item);

PayItemType          pay_item_get_type                   (const PayItem* item);

uint64_t             pay_item_get_purchase_id            (const PayItem* item);


#ifdef __cplusplus
}
#endif

#pragma GCC visibility pop

#endif /* LIBPAY_ITEM_H */
