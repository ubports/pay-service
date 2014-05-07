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

#include "purchase-null.h"

namespace Purchase
{

class NullItem : public Item
{
public:
    typedef std::shared_ptr<Item> Ptr;

    NullItem (std::string& in_appid, std::string& in_itemid) :
        appid(in_appid), itemid(in_itemid)
    {
    }
    virtual bool run (void)
    {
        std::cout << "Running" << std::endl;
    };

private:
    std::string appid;
    std::string itemid;
};

Item::Ptr
NullFactory::purchaseItem (std::string& appid, std::string& itemid)
{
    return std::make_shared<NullItem>(appid, itemid);
}

bool
NullFactory::running ()
{
    return false;
}

} // ns Purchase

