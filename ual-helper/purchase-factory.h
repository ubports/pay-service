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

#include <core/signal.h>

#ifndef PURCHASE_FACTORY_HPP__
#define PURCHASE_FACTORY_HPP__ 1

namespace Purchase
{

class Item
{
public:
    enum Status
    {
        ERROR,
        NOT_PURCHASED,
        PURCHASED
    };

    virtual bool run (void) = 0;

    typedef std::shared_ptr<Item> Ptr;

    core::Signal<Status> purchaseComplete;
};

class Factory
{
public:
    virtual ~Factory() = default;

    virtual Item::Ptr purchaseItem (std::string& appid, std::string& itemid) = 0;

    typedef std::shared_ptr<Factory> Ptr;
};

} // ns Purchase

#endif /* PURCHASE_FACTORY_HPP__ */
