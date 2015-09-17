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

#pragma once

#include <libpay/pay-item.h>
#include <libpay/pay-package.h>

#include <ctime>
#include <string>

namespace Pay
{

namespace Internal
{

class Item
{
    const std::string m_sku;
    std::string m_description;
    std::string m_price;
    std::string m_title;
    PayItemType m_type = PAY_ITEM_TYPE_UNKNOWN;
    PayPackageItemStatus m_status = PAY_PACKAGE_ITEM_STATUS_UNKNOWN;
    int m_ref_count = 1;
    bool m_acknowledged = false;
    time_t m_acknowledged_time = 0;
    time_t m_purchased_time = 0;

public:

    /** life cycle **/

    explicit Item (const std::string& sku): m_sku(sku) {}
    ~Item() =default;

    void ref() {++m_ref_count;}
    void unref() {
        if (!--m_ref_count)
            delete this;
    }

    /** accessors **/

    bool acknowledged() const {return m_acknowledged;}
    time_t acknowledged_time() const {return m_acknowledged_time;}
    const std::string& description() const {return m_description;}
    const std::string& sku() const {return m_sku;}
    const std::string& price() const {return m_price;}
    time_t purchased_time() const {return m_purchased_time;}
    PayPackageItemStatus status() const {return m_status;}
    const std::string& title() const {return m_title;}
    PayItemType type() const {return m_type;}

    /** setters **/

    void set_acknowledged(bool val) {m_acknowledged = val;}
    void set_acknowledged_time(time_t val) {m_acknowledged_time = val;}
    void set_description(const std::string& val) {m_description = val;}
    void set_price(const std::string& val) {m_price = val;}
    void set_purchased_time(time_t val) {m_purchased_time = val;}
    void set_status(PayPackageItemStatus val) {m_status = val;}
    void set_title(const std::string& val) {m_title = val;}
    void set_type(PayItemType val) {m_type = val;}

    bool operator<(const Item& that) const {return sku() < that.sku();}
};

} // namespace Internal

} // namespace Pay

struct PayItem_: public Pay::Internal::Item
{
    explicit PayItem_(const std::string& product_id): Pay::Internal::Item(product_id) {}
};

