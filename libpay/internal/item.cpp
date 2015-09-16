/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libpay/internal/item.h>

#include <glib.h>

namespace Pay
{

namespace Internal
{

Item::Item(const std::string& sku):
    m_sku(sku)
{
}

void Item::ref()
{
    ++m_ref_count;
}

void Item::unref()
{
    if (!--m_ref_count)
        delete this;
}

bool Item::acknowledged() const
{
    return m_acknowledged;
}

time_t Item::acknowledged_time() const
{
    return m_acknowledged_time;
}

const std::string& Item::description() const
{
    return m_description;
}

const std::string& Item::sku() const
{
    return m_sku;
}

const std::string& Item::price() const
{
    return m_price;
}

time_t Item::purchased_time() const
{
    return m_purchased_time;
}

PayPackageItemStatus Item::status() const
{
    return m_status;
}

const std::string& Item::title() const
{
    return m_title;
}

PayItemType Item::type() const
{
    return m_type;
}

} // namespace Internal

} // namespace Pay
