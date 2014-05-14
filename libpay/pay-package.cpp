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

#include "pay-package.h"
#include <string>

namespace Pay
{
class Package
{
    std::string id;

public:
    Package (const char* packageid) : id(packageid) { }

    PayPackageItemStatus itemStatus (const char* itemid)
    {
    }

    bool startVerification (const char* itemid)
    {
    }

    bool startPurchase (const char* itemid)
    {
    }
};



}; // ns Pay


/**************************
 * Public API
 **************************/
struct PayPackage_
{
    int dummy;
};

PayPackage*
pay_package_new (const char* package_name)
{
    Pay::Package* ret = new Pay::Package(package_name);
    return reinterpret_cast<PayPackage*>(ret);
}

void pay_package_delete (PayPackage* package)
{
    auto pkg = reinterpret_cast<Pay::Package*>(package);
    delete pkg;
}

PayPackageItemStatus pay_package_item_status (PayPackage* package,
                                              const char* itemid)
{

    auto pkg = reinterpret_cast<Pay::Package*>(package);
    return pkg->itemStatus(itemid);
}

int pay_package_item_observer_install (PayPackage* package,
                                       PayPackageItemObserver observer,
                                       void* user_data)
{
    auto pkg = reinterpret_cast<Pay::Package*>(package);
}

int pay_package_item_observer_uninstall (PayPackage* package,
                                         PayPackageItemObserver observer,
                                         void* user_data)
{
    auto pkg = reinterpret_cast<Pay::Package*>(package);
}

int pay_package_item_start_verification (PayPackage* package,
                                         const char* itemid)
{

    auto pkg = reinterpret_cast<Pay::Package*>(package);
    return pkg->startVerification(itemid);
}

int pay_package_item_start_purchase (PayPackage* package,
                                     const char* itemid)
{
    auto pkg = reinterpret_cast<Pay::Package*>(package);
    return pkg->startPurchase(itemid);
}
