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

#include <list>
#include <string>
#include <memory>
#include <map>

#include <core/signal.h>

#ifndef ITEM_INTERFACE_HPP__
#define ITEM_INTERFACE_HPP__ 1

namespace Item
{

class Item
{
public:
    enum Status
    {
        UNKNOWN,
        VERIFYING,
        NOT_PURCHASED,
        PURCHASED
    };

    virtual std::string& getId (void) = 0;
    virtual Status getStatus (void) = 0;
    virtual bool verify (void) = 0;

    typedef std::shared_ptr<Item> Ptr;
};

class Store
{
public:
    virtual std::list<std::string> listApplications (void) = 0;
    virtual std::shared_ptr<std::map<std::string, Item::Ptr>> getItems (std::string& application) = 0;
    virtual Item::Ptr getItem (std::string& application, std::string& item) = 0;

    typedef std::shared_ptr<Store> Ptr;

    core::Signal<std::string&, std::string&, Item::Status> itemChanged;
};

} // namespace Item

#endif // ITEM_INTERFACE_HPP__

