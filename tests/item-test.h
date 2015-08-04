/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Ted Gould <ted.gould@canonical.com>
 */

#include <algorithm>
#include <string>
#include "../service/item-interface.h"

namespace Item
{

class TestItem : public Item
{
    const std::string _id;
    const std::string _app;
    uint64_t refund_timeout;
    Status status = Status::UNKNOWN;
    bool verifyResult = false;
    bool refundResult = false;
    bool purchaseResult = false;

public:
    TestItem (const std::string& app, const std::string& id) :
        _id(id),
        _app(app)
    {
    }

    const std::string& getId (void) override
    {
        return _id;
    }

    const std::string& getApp (void)
    {
        return _app;
    }

    Item::Status getStatus (void) override
    {
        return status;
    }

    uint64_t getRefundExpiry (void) override
    {
        return refund_timeout;
    }

    bool verify (void) override
    {
        return verifyResult;
    }

    bool refund (void) override
    {
        return verifyResult;
    }

    bool purchase (void) override
    {
        return purchaseResult;
    }

    void test_setStatus (Item::Status instatus, bool signal)
    {
        status = instatus;
        if (signal)
        {
            statusChanged(instatus, getRefundExpiry());
        }
    }

    core::Signal<Item::Status, uint64_t> statusChanged;
};

class TestStore : public Store
{
    std::map<std::string, std::shared_ptr<std::map<std::string, Item::Ptr>>> data;
public:
    std::list<std::string> listApplications (void) override
    {
        std::list<std::string> apps;

        std::transform(data.begin(),
                       data.end(),
                       std::back_inserter(apps),
                       [](const std::pair<std::string, std::shared_ptr<std::map<std::string, Item::Ptr>>>& pair)
        {
            return pair.first;
        });

        return apps;
    }

    std::shared_ptr<std::map<std::string, Item::Ptr>> getItems (const std::string& application) override
    {
        auto app = data[application];

        if (app == nullptr)
        {
            app = std::make_shared<std::map<std::string, Item::Ptr>>();
            data[application] = app;
        }

        return app;
    }

    Item::Ptr getItem (const std::string& application, const std::string& itemid) override
    {
        auto app = getItems(application);
        Item::Ptr item = (*app)[itemid];

        if (item == nullptr)
        {
            auto titem = std::make_shared<TestItem>(application, itemid);
            titem->statusChanged.connect([this, titem](Item::Status status,
                                                       uint64_t timeout)
            {
                itemChanged(titem->getApp(), titem->getId(), status, timeout);
            });
            item = std::dynamic_pointer_cast<Item, TestItem>(titem);
            (*app)[itemid] = item;
        }

        return item;
    }

    typedef std::shared_ptr<TestStore> Ptr;
};

}; // ns Item
