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
 *
 * Authors:
 *   Charles Kerr <charles.kerr@canonical.com>
 */

#include <core/signal.h>

#ifndef REFUND_FACTORY_HPP__
#define REFUND_FACTORY_HPP__ 1

namespace Refund
{

class Item
{

public:
    virtual bool run (void) =0;

    typedef std::shared_ptr<Item> Ptr;

    core::Signal<bool> finished;
};

class Factory
{
public:
    virtual ~Factory() =default;

    virtual bool running () =0;
    virtual Item::Ptr refund (const std::string& appid, const std::string& itemid) =0;

    typedef std::shared_ptr<Factory> Ptr;
};

} // ns Refund

#endif /* REFUND_FACTORY_HPP__ */
