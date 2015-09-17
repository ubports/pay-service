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

#include <libpay/pay-package.h>
#include <libpay/internal/package.h>

PayPackage*
pay_package_new (const char* package_name)
{
    g_return_val_if_fail(package_name != nullptr, nullptr);
    g_return_val_if_fail(*package_name != '\0', nullptr);

    try
    {
        return new PayPackage(package_name);
    }
    catch (std::runtime_error& /*error*/)
    {
        return nullptr;
    }
}

void pay_package_delete (PayPackage* package)
{
    g_return_if_fail(package != nullptr);

    delete package;
}

PayPackageItemStatus pay_package_item_status (PayPackage* package,
                                              const char* itemid)
{
    g_return_val_if_fail(package != nullptr, PAY_PACKAGE_ITEM_STATUS_UNKNOWN);
    g_return_val_if_fail(itemid != nullptr, PAY_PACKAGE_ITEM_STATUS_UNKNOWN);

    return package->itemStatus(itemid);
}

int pay_package_item_is_refundable (PayPackage* package,
                                    const char* itemid)
{
    const auto status = pay_package_refund_status(package, itemid);
    return (status == PAY_PACKAGE_REFUND_STATUS_REFUNDABLE ||
            status == PAY_PACKAGE_REFUND_STATUS_WINDOW_EXPIRING);
}

PayPackageRefundStatus pay_package_refund_status (PayPackage* package,
                                                  const char* itemid)
{
    g_return_val_if_fail(package != nullptr, PAY_PACKAGE_REFUND_STATUS_NOT_REFUNDABLE);
    g_return_val_if_fail(itemid != nullptr, PAY_PACKAGE_REFUND_STATUS_NOT_REFUNDABLE);

    return package->refundStatus(itemid);
}

int pay_package_item_observer_install (PayPackage* package,
                                       PayPackageItemObserver observer,
                                       void* user_data)
{
    g_return_val_if_fail(package != nullptr, 0);
    g_return_val_if_fail(observer != nullptr, 0);

    return package->addStatusObserver(observer, user_data) ? 1 : 0;
}

int pay_package_item_observer_uninstall (PayPackage* package,
                                         PayPackageItemObserver observer,
                                         void* user_data)
{
    g_return_val_if_fail(package != nullptr, 0);
    g_return_val_if_fail(observer != nullptr, 0);

    return package->removeStatusObserver(observer, user_data) ? 1 : 0;
}

int pay_package_refund_observer_install (PayPackage* package,
                                         PayPackageRefundObserver observer,
                                         void* user_data)
{
    g_return_val_if_fail(package != nullptr, 0);
    g_return_val_if_fail(observer != nullptr, 0);

    return package->addRefundObserver(observer, user_data) ? 1 : 0;
}

int pay_package_refund_observer_uninstall (PayPackage* package,
                                           PayPackageRefundObserver observer,
                                           void* user_data)
{
    g_return_val_if_fail(package != nullptr, 0);
    g_return_val_if_fail(observer != nullptr, 0);

    return package->removeRefundObserver(observer, user_data) ? 1 : 0;
}

int pay_package_item_start_verification (PayPackage* package,
                                         const char* itemid)
{
    g_return_val_if_fail(package != nullptr, 0);
    g_return_val_if_fail(itemid != nullptr, 0);
    g_return_val_if_fail(*itemid != '\0', 0);

    return package->startVerification(itemid);
}

int pay_package_item_start_purchase (PayPackage* package,
                                     const char* itemid)
{
    g_return_val_if_fail(package != nullptr, 0);
    g_return_val_if_fail(itemid != nullptr, 0);
    g_return_val_if_fail(*itemid != '\0', 0);

    return package->startPurchase(itemid);
}

int pay_package_item_start_refund (PayPackage* package,
                                   const char* itemid)
{
    g_return_val_if_fail(package != nullptr, 0);
    g_return_val_if_fail(itemid != nullptr, 0);
    g_return_val_if_fail(*itemid != '\0', 0);

    return package->startRefund(itemid);
}

int pay_package_item_start_acknowledge (PayPackage* package,
                                        const char* sku)
{
    g_return_val_if_fail (package != nullptr, 0);
    g_return_val_if_fail (sku != nullptr, 0);
    g_return_val_if_fail (*sku != '\0', 0);

    return package->startAcknowledge(sku) ? 1 : 0;
}


