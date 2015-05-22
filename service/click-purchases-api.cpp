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

#include "click-purchases-api.h"

namespace Web
{

class ClickPurchasesApi::Impl
{
public:

    Impl(Factory::Ptr wfactory):
        m_wfactory(wfactory)
    {
    }

    ~Impl() =default;

    bool running()
    {
        return m_wfactory->running();
    }

    void setDevice(const std::string& device_id)
    {
        m_device_id = device_id;
    }

    Request::Ptr getItemInfo(const std::string& package_name,
                             const std::string& sku)
    {
        // https://developer.staging.ubuntu.com/docs/api/iap.html#retrieve-item-details-by-sku
        auto url = get_inventory_url() + '/' + package_name +
                   "/items/by-sku/" + sku;

        auto req = m_wfactory->create_request(url, true);
        maybe_add_device_header(req);
        return req;
    }

    Request::Ptr getPackageInfo(const std::string& package_name,
                                bool include_item_purchases)
    {
        // https://developer.staging.ubuntu.com/docs/api/click-purchases.html#get-subscription-details

        auto url = get_purchases_url() + '/' + package_name;

        if (include_item_purchases)
        {
            url += "?include_item_purchases=true";
        }

        auto req = m_wfactory->create_request(url, true);
        maybe_add_device_header(req);
        return req;
    }

    Request::Ptr refundPackage(const std::string& package_name)
    {
        auto req = m_wfactory->create_request(get_refund_url(), true);
        maybe_add_device_header(req);
        req->set_post("name=" + package_name);
        return req;
    }

private:

    void maybe_add_device_header(Request::Ptr request)
    {
        if (!m_device_id.empty())
        {
            request->set_header("X-Device-Id", m_device_id);
        }
    }


    // get the refund URL; e.g. https://myapps.developer.ubuntu.com/api/2.0/click/refunds/
    std::string get_refund_url() const
    {
        std::string url {get_base_url()};
        url += PAY_API_ROOT;
        url += "/refunds/";
        return url;
    }

    // get the purchases URL; e.g. https://myapps.developer.ubuntu.com/api/2.0/click/purchases
    std::string get_purchases_url() const
    {
        std::string url {get_base_url()};
        url += PAY_API_ROOT;
        url += "/purchases";
        return url;
    }

    // get the inventory URL: e.g. https://myapps.developer.ubuntu.com/packages
    std::string get_inventory_url() const
    {
        std::string url {get_base_url()};
        url += "/packages";
        return url;
    }

    static const char* get_base_url()
    {
        static constexpr char const* BASE_URL_ENVVAR = "PURCHASES_BASE_URL";
        static constexpr char const* DEFAULT_BASE = "https://myapps.developer.ubuntu.com";

        const char* env = getenv(BASE_URL_ENVVAR);
        return env && *env ? env : DEFAULT_BASE;
    }

    static constexpr char const* PAY_API_ROOT = "/api/2.0/click";

    Factory::Ptr m_wfactory;
    std::string m_device_id;
};

/***
****
***/

ClickPurchasesApi::ClickPurchasesApi(Factory::Ptr wfactory):
    impl {new Impl{wfactory}}
{
}

ClickPurchasesApi::~ClickPurchasesApi()
{
}

bool
ClickPurchasesApi::running()
{
    return impl->running();
}

Request::Ptr
ClickPurchasesApi::getPackageInfo(const std::string& package_name,
                                  bool include_item_purchases)
{
    return impl->getPackageInfo(package_name,
                                include_item_purchases);
}

Request::Ptr
ClickPurchasesApi::getItemInfo(const std::string& package_name,
                               const std::string& sku)
{
    return impl->getItemInfo(package_name, sku);
}

Request::Ptr
ClickPurchasesApi::refundPackage(const std::string& package_name)
{
    return impl->refundPackage(package_name);
}

void
ClickPurchasesApi::setDevice(const std::string& device_id)
{
    impl->setDevice(device_id);
}

} // namespace Web
