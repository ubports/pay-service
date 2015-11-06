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
 * Authored by: Ted Gould <ted@canonical.com>
 */

#ifndef PAY_TYPES_H
#define PAY_TYPES_H 1

#pragma GCC visibility push(default)

#ifdef __cplusplus
extern "C" {
#endif


typedef struct PayItem_ PayItem;

typedef struct PayPackage_ PayPackage;



/**
 * PayPackageItemStatus:
 *
 * The states that an purchased item can be in.
 */
typedef enum
{
    /*< prefix=PAY_PACKAGE_ITEM_STATUS */
    PAY_PACKAGE_ITEM_STATUS_UNKNOWN,       /*< nick=unknown */
    PAY_PACKAGE_ITEM_STATUS_VERIFYING,     /*< nick=verifying */
    PAY_PACKAGE_ITEM_STATUS_PURCHASED,     /*< nick=purchased */
    PAY_PACKAGE_ITEM_STATUS_PURCHASING,    /*< nick=purchasing */
    PAY_PACKAGE_ITEM_STATUS_NOT_PURCHASED, /*< nick=not-purchased */
    PAY_PACKAGE_ITEM_STATUS_REFUNDING,     /*< nick=refunding */
    PAY_PACKAGE_ITEM_STATUS_APPROVED       /*< nick=approved */
} PayPackageItemStatus;

/**
 * PayItemType:
 *
 * The type of the product.
 */
typedef enum
{
    /*< prefix=PAY_PRODUCT_TYPE */
    PAY_ITEM_TYPE_UNKNOWN = -1,   /*< nick=unknown */
    PAY_ITEM_TYPE_CONSUMABLE = 0, /*< nick=consumable */
    PAY_ITEM_TYPE_UNLOCKABLE = 1  /*< nick=unlockable */
} PayItemType;

/**
 * PayPackageRefundStatus:
 *
 * The states of refundable an item  an be in.
 */
typedef enum
{
    /*< prefix=PAY_PACKAGE_REFUND_STATUS */
    PAY_PACKAGE_REFUND_STATUS_REFUNDABLE,      /*< nick=refundable */
    PAY_PACKAGE_REFUND_STATUS_NOT_REFUNDABLE,  /*< nick=not-refundable */
    PAY_PACKAGE_REFUND_STATUS_NOT_PURCHASED,   /*< nick=not-purchased */
    PAY_PACKAGE_REFUND_STATUS_WINDOW_EXPIRING  /*< nick=window-expiring */
} PayPackageRefundStatus;



/**
 * PayPackageItemObserver:
 *
 * Function to call when an item changes state for the
 * package it's registered for.
 */
typedef void (*PayPackageItemObserver) (PayPackage* package,
                                        const char* sku,
                                        PayPackageItemStatus status,
                                        void* user_data);

/**
 * PayPackageItemRefundableObserver:
 *
 * Function to call when an item changes whether it is
 * refundable or not.
 */
typedef void (*PayPackageRefundObserver) (PayPackage* package,
                                          const char* sku,
                                          PayPackageRefundStatus status,
                                          void* user_data);


#ifdef __cplusplus
}
#endif

#pragma GCC visibility pop

#endif /* PAY_TYPES_H */
