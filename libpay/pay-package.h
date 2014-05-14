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
typedef enum
{
    PAY_PACKAGE_ITEM_STATUS_UNKNOWN,
    PAY_PACKAGE_ITEM_STATUS_VERIFYING,
    PAY_PACKAGE_ITEM_STATUS_PURCHASED,
    PAY_PACKAGE_ITEM_STATUS_PURCHASING,
    PAY_PACKAGE_ITEM_STATUS_NOT_PURCHASED
} PayPackageItemStatus;

typedef void (*PayPackageItemObserver) (PayPackage* package,
                                        const char* itemid,
                                        PayPackageItemStatus status,
                                        void* user_data);

PayPackage* pay_package_new (const char* package_name);
void pay_package_delete (PayPackage* package);
PayPackageItemStatus pay_package_item_status (PayPackage* package,
                                              const char* itemid);
int pay_package_item_observer_install (PayPackage* package,
                                       PayPackageItemObserver observer,
                                       void* user_data);
int pay_package_item_observer_uninstall (PayPackage* package,
                                         PayPackageItemObserver observer,
                                         void* user_data);

int pay_package_item_start_verification (PayPackage* package,
                                         const char* itemid);
int pay_package_item_start_purchase (PayPackage* package,
                                     const char* itemid);

#pragma GCC visibility pop

#ifdef __cplusplus
}
#endif

#endif /* PAY_PACKAGE_H */
