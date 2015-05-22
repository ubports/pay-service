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

#include "item-memory.h"

#include <algorithm>
#include <core/signal.h>
#include <memory>

namespace Item
{

class MemoryItem : public Item
{
public:
    MemoryItem (std::string& in_app,
                std::string& in_id,
                Verification::Factory::Ptr& in_vfactory,
                Refund::Factory::Ptr& in_rfactory,
                Purchase::Factory::Ptr& in_pfactory) :
        app(in_app),
        id(in_id),
        vfactory(in_vfactory),
        rfactory(in_rfactory),
        pfactory(in_pfactory),
        vitem(nullptr),
        pitem(nullptr),
        status(Item::Status::UNKNOWN)
    {
        /* We init into the unknown state and then wait for someone
           to ask us to do something about it. */
    }

    std::string& getApp (void)
    {
        return app;
    }

    std::string& getId (void) override
    {
        return id;
    }

    Item::Status getStatus (void) override
    {
        return status;
    }

    bool verify (void) override
    {
        if (!vfactory->running())
        {
            return false;
        }

        if (vitem == nullptr)
        {
            vitem = vfactory->verifyItem(app, id);
        }

        if (vitem == nullptr)
        {
            /* Uhg, failed */
            return false;
        }

        /* New verification instance, tell the world! */
        setStatus(Item::Status::VERIFYING);

        /* When the verification item has run it's course we need to
           update our status */
        /* NOTE: This will execute on the verification item's thread */
        vitem->verificationComplete.connect([this](Verification::Item::Status status)
        {
            switch (status)
            {
                case Verification::Item::PURCHASED:
                    setStatus(Item::Status::PURCHASED);
                    break;
                case Verification::Item::NOT_PURCHASED:
                    setStatus(Item::Status::NOT_PURCHASED);
                    break;
                case Verification::Item::APPROVED:
                    setStatus(Item::Status::APPROVED);
                    break;
                case Verification::Item::ERROR:
                default: /* Fall through, an error is same as status we don't know */
                    setStatus(Item::Status::UNKNOWN);
                    break;
            }
        });

        return vitem->run();
    }

    bool refund (void) override
    {
        if (!rfactory->running())
        {
            return false;
        }

        if (ritem == nullptr)
        {
            ritem = rfactory->refund(app, id);

            if (ritem == nullptr)
            {
                return false;
            }
        }

        const auto cached_status = getStatus();

        setStatus(Item::Status::REFUNDING);

        ritem->finished.connect([this, cached_status](bool success)
        {
            auto new_status = success ? Status::NOT_PURCHASED : cached_status;
            setStatus(new_status);
        });

        return ritem->run();
    }

    bool purchase (void) override
    {
        /* First check to see if a purchase makes sense */
        if (status == PURCHASED)
        {
            return true;
        }

        if (pitem == nullptr)
        {
            pitem = pfactory->purchaseItem(app, id);
            if (pitem == nullptr)
            {
                /* Uhg, failed */
                return false;
            }

            pitem->purchaseComplete.connect([this](Purchase::Item::Status status)
            {
                /* Verifying on each time the purchase UI runs right now because
                   we're not getting reliable status back from them. */
                if (!verify())
                {
                    setStatus(Item::Status::NOT_PURCHASED);
                }
                return;
            });
        }

        /* New purchase instance, tell the world! */
        setStatus(Item::Status::PURCHASING);

        return pitem->run();
    }

    typedef std::shared_ptr<MemoryItem> Ptr;
    core::Signal<Item::Status> statusChanged;

private:
    void setStatus (Item::Status in_status)
    {
        std::unique_lock<std::mutex> ul(status_mutex);
        bool signal = (status != in_status);

        status = in_status;
        ul.unlock();

        if (signal)
            /* NOTE: in_status here as it's on the stack and the status
               that this signal should be associated with */
        {
            statusChanged(in_status);
        }
    }

    /***** Only set at init *********/
    /* Item ID */
    std::string id;
    /* Application ID */
    std::string app;
    Verification::Factory::Ptr vfactory;
    Refund::Factory::Ptr rfactory;
    Purchase::Factory::Ptr pfactory;

    /****** std::shared_ptr<> is threadsafe **********/
    /* Verification item if we're in the state of verifying or null otherwise */
    Verification::Item::Ptr vitem;
    /* Refund item if we're in the state of refunding or null otherwise */
    Refund::Item::Ptr ritem;
    /* Purchase item if we're in the state of purchasing or null otherwise */
    Purchase::Item::Ptr pitem;

    /****** status is protected with it's own mutex *******/
    std::mutex status_mutex;
    Item::Status status;
};

std::list<std::string>
MemoryStore::listApplications (void)
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

std::shared_ptr<std::map<std::string, Item::Ptr>>
                                               MemoryStore::getItems (std::string& application)
{
    auto app = data[application];

    if (app == nullptr)
    {
        app = std::make_shared<std::map<std::string, Item::Ptr>>();
        data[application] = app;
    }

    return app;
}

Item::Ptr
MemoryStore::getItem (std::string& application, std::string& itemid)
{
    if (verificationFactory == nullptr)
    {
        return Item::Ptr(nullptr);
    }

    if (purchaseFactory == nullptr)
    {
        return Item::Ptr(nullptr);
    }

    auto app = getItems(application);
    Item::Ptr item = (*app)[itemid];

    if (item == nullptr)
    {
        auto mitem = std::make_shared<MemoryItem>(application,
                                                  itemid,
                                                  verificationFactory,
                                                  refundFactory,
                                                  purchaseFactory);

        mitem->statusChanged.connect([this, mitem](Item::Status status)
        {
            itemChanged(mitem->getApp(), mitem->getId(), status);
        });

        item = std::dynamic_pointer_cast<Item, MemoryItem>(mitem);
        (*app)[itemid] = item;
    }

    return item;
}

} // namespace Item
