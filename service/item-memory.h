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

#include "item-interface.h"
#include "verification-factory.h"
#include "purchase-factory.h"
#include <memory>
#include <iostream>
#include <map>

namespace Item
{

class MemoryStore : public Store
{
public:
    MemoryStore (const Verification::Factory::Ptr& factory, const Purchase::Factory::Ptr& pfactory) :
        verificationFactory(factory),
        purchaseFactory(pfactory)
    {
        if (verificationFactory == nullptr)
        {
            throw std::invalid_argument("factory");
        }
    }
    std::list<std::string> listApplications (void);
    std::shared_ptr<std::map<std::string, Item::Ptr>> getItems (std::string& application);
    Item::Ptr getItem (std::string& application, std::string& itemid);

private:
    std::map<std::string, std::shared_ptr<std::map<std::string, Item::Ptr>>> data;
    Verification::Factory::Ptr verificationFactory;
    Purchase::Factory::Ptr purchaseFactory;
};

} // namespace Item
