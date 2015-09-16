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

#ifndef PAY_PACKAGE_H
#define PAY_PACKAGE_H 1

#pragma GCC visibility push(default)

#ifdef __cplusplus
extern "C" {
#endif

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
 * PayPackageItemObserver:
 *
 * Function to call when an item changes state for the
 * package it's registered for.
 */
typedef void (*PayPackageItemObserver) (PayPackage* package,
                                        const char* itemid,
                                        PayPackageItemStatus status,
                                        void* user_data);

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
 * PayPackageItemRefundableObserver:
 *
 * Function to call when an item changes whether it is
 * refundable or not.
 */
typedef void (*PayPackageRefundObserver) (PayPackage* package,
                                          const char* itemid,
                                          PayPackageRefundStatus status,
                                          void* user_data);

/**
 * pay_package_new:
 * @package_name: name of package that the items are related to
 *
 * Allocates a package object to get information on the items
 * that are related to that package.
 *
 * Return value: (transfer full): Object to interact with items
 *     for the package.
 */
PayPackage* pay_package_new (const char* package_name);

/**
 * pay_package_delete:
 * @package: package object to free
 *
 * Frees the resources associated with the package object, should be
 * done when the application is finished with them.
 */
void pay_package_delete (PayPackage* package);

/**
 * pay_package_item_status:
 * @package: Package the item is related to
 * @itemid: ID of the item
 *
 * Checks the status of an individual item.
 *
 * Return value: The status of the item on the local pay service
 */
PayPackageItemStatus pay_package_item_status (PayPackage* package,
                                              const char* itemid);

/**
 * pay_package_item_is_refundable:
 * @package: Package the item is related to
 * @itemid: ID of the item
 *
 * Checks whether it is refundable. Check with the status
 * and makes sure it is REFUNDABLE.
 *
 * Return value: Non-zero if the item is refundable
 */
int pay_package_item_is_refundable (PayPackage* package,
                                    const char* itemid);

/**
 * pay_package_refund_status:
 * @package: Package the item is related to
 * @itemid: ID of the item
 *
 * Checks the refund status of an individual item.
 *
 * Return value: The refund status of the item
 */
PayPackageRefundStatus pay_package_refund_status (PayPackage* package,
                                                  const char* itemid);

/**
 * pay_package_item_observer_install:
 * @package: Package to watch items on
 * @observer: Function to call if items change state
 * @user_data: Data to pass to @observer
 *
 * Registers a function to be called if an item changes state. This
 * can be used to know when an item is being verified and completes
 * the step or if it is purchased. All state changes are reported.
 *
 * Return value: zero when fails to install
 */
int pay_package_item_observer_install (PayPackage* package,
                                       PayPackageItemObserver observer,
                                       void* user_data);
/**
 * pay_package_item_observer_uninstall:
 * @package: Package to remove watch from
 * @observer: Function to call if items change state
 * @user_data: Data to pass to @observer
 *
 * Stops an observer from getting called.
 *
 * Return value: zero when fails to uninstall
 */
int pay_package_item_observer_uninstall (PayPackage* package,
                                         PayPackageItemObserver observer,
                                         void* user_data);

/**
 * pay_package_refund_observer_install:
 * @package: Package to watch items on
 * @observer: Function to call if items changes refund staus
 * @user_data: Data to pass to @observer
 *
 * Registers a function to call if the items refund status
 * changes. This can be used to know when it is no longer
 * refundable or when it is about to become unrefundable.
 *
 * Return value: zero when fails to install
 */
int pay_package_refund_observer_install (PayPackage* package,
                                         PayPackageRefundObserver observer,
                                         void* user_data);
/**
 * pay_package_refund_observer_uninstall:
 * @package: Package to remove watch from
 * @observer: Function to call if items change state
 * @user_data: Data to pass to @observer
 *
 * Stops a refund observer from getting called.
 *
 * Return value: zero when fails to uninstall
 */
int pay_package_refund_observer_uninstall (PayPackage* package,
                                           PayPackageRefundObserver observer,
                                           void* user_data);


/**
 * pay_package_item_start_verification:
 * @package: package to verify item for
 * @itemid: ID of the item to verify
 *
 * Asks the pay service to ask the server to verify
 * the status of an item. It will go on the network and
 * request the status, and update the state of the item
 * appropriately. Most users of this API will want to set
 * up an observer to see the state changes.
 *
 * Return value: zero when unable to make request to pay service
 */
int pay_package_item_start_verification (PayPackage* package,
                                         const char* itemid);

/**
 * pay_package_item_start_purchase:
 * @package: package to purchase item for
 * @itemid: ID of the item to purchase
 *
 * Requests that the pay-service start the process of purchasing
 * the item specified by @itemid. This requires launching UI elements
 * that will cover the application requesting the payment. When
 * the UI determines that the purchase is complete, or the user
 * terminates the pay action the UI will be dismissed and the status
 * of the item will be updated.
 *
 * Return value: zero when unable to make request to pay service
 */
int pay_package_item_start_purchase (PayPackage* package,
                                     const char* itemid);

/**
 * pay_package_item_start_refund:
 * @package: package the item was purchased for
 * @itemid: ID of the item to refund
 *
 * Requests that the pay-service start the process of refunding
 * the item specified by @itemid.
 *
 * Return value: zero when unable to make request to pay service
 */
int pay_package_item_start_refund (PayPackage* package,
                                   const char* itemid);

#ifdef __cplusplus
}
#endif

#pragma GCC visibility pop

#endif /* PAY_PACKAGE_H */
