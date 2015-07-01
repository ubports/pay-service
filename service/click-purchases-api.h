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

#ifndef CLICK_PURCHASES_API__
#define CLICK_PURCHASES_API__ 1

#include "webclient-factory.h"

#include <string>
#include <memory> // std::unique_ptr

namespace Web
{

class ClickPurchasesApi
{
public:
    ClickPurchasesApi(Web::Factory::Ptr wfactory);
    virtual ~ClickPurchasesApi();
    bool running();

    Request::Ptr getPackageInfo(const std::string& package_name,
                                bool include_item_purchases);

    Request::Ptr getItemInfo(const std::string& package_name,
                             const std::string& sku);

    Request::Ptr refundPackage(const std::string& package_name);

    void setDevice(const std::string& device_id);

    typedef std::shared_ptr<ClickPurchasesApi> Ptr;

private:

    friend class Impl;
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // ns Web

#endif // CLICK_PURCHASES_API__
