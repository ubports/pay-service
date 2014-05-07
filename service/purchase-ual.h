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

#include "purchase-factory.h"

#ifndef PURCHASE_UAL_HPP__
#define PURCHASE_UAL_HPP__ 1

namespace Purchase
{

class UalFactory : public Factory
{
public:
    virtual bool running ();
    virtual Item::Ptr purchaseItem (std::string& appid, std::string& itemid);

    typedef std::shared_ptr<UalFactory> Ptr;
};

} // ns Purchase

#endif /* PURCHASE_UAL_HPP__ */
