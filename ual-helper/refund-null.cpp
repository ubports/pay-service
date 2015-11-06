/*
 * Copyright © 2015 Canonical Ltd.
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
 */

#include "refund-null.h"

namespace Refund
{

class NullItem : public Item
{
public:

    NullItem() =default;
    ~NullItem() =default;

    bool run (void) override
    {
        return false;
    }
};


bool
NullFactory::running ()
{
    return false;
}

Item::Ptr
NullFactory::refund (const std::string& /*appid*/, const std::string& /*itemid*/)
{
    return std::make_shared<NullItem>();
}

} // ns Refund
