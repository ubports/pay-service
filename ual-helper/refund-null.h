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

#include "refund-factory.h"

#ifndef REFUND_NULL_HPP__
#define REFUND_NULL_HPP__ 1

namespace Refund
{

class NullFactory : public Factory
{
public:
    bool running () override;
    Item::Ptr refund (const std::string& appid, const std::string& itemid) override;
    typedef std::shared_ptr<NullFactory> Ptr;
};

} // ns Web

#endif /* REFUND_NULL_HPP__ */
