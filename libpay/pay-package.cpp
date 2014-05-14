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
#include <map>
#include <core/signal.h>

namespace Pay
{
class Package
{
    std::string id;
    /* NOTE: Using the shared_ptr here because gcc 4.7 map doesn't have emplace */
    std::map <std::pair<PayPackageItemObserver, void*>, std::shared_ptr<core::ScopedConnection>> observers;
    core::Signal<std::string, PayPackageItemStatus> itemChanged;
    std::map <std::string, PayPackageItemStatus> itemStatusCache;

public:
    Package (const char* packageid) : id(packageid)
    {
        /* Keeps item cache up-to-data as we get signals about it */
        itemChanged.connect([this](std::string itemid, PayPackageItemStatus status)
        {
            itemStatusCache[itemid] = status;
        });
    }

    PayPackageItemStatus itemStatus (const char* itemid)
    {
        try
        {
            return itemStatusCache[itemid];
        }
        catch (std::out_of_range range)
        {
            return PAY_PACKAGE_ITEM_STATUS_UNKNOWN;
        }
    }

    bool addItemObserver (PayPackageItemObserver observer, void* user_data)
    {
        /* Creates a connection to the signal for the observer and stores the connection
           object in the map so that we can remove it later, or it'll get disconnected
           when the whole object gets destroyed */
        std::pair<PayPackageItemObserver, void*> key(observer, user_data);
        auto connection = std::make_shared<core::ScopedConnection>(itemChanged.connect([this, observer, user_data] (
                                                                                           std::string itemid,
                                                                                           PayPackageItemStatus status)
        {
            observer(reinterpret_cast<PayPackage*>(this), itemid.c_str(), status, user_data);
        }));
        observers[key] = connection;
        return true;
    }

    bool removeItemObserver (PayPackageItemObserver observer, void* user_data)
    {
        std::pair<PayPackageItemObserver, void*> key(observer, user_data);
        observers.erase(key);
        return true;
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
    pkg->addItemObserver(observer, user_data);
}

int pay_package_item_observer_uninstall (PayPackage* package,
                                         PayPackageItemObserver observer,
                                         void* user_data)
{
    auto pkg = reinterpret_cast<Pay::Package*>(package);
    pkg->removeItemObserver(observer, user_data);
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
