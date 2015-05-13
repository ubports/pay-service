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

#include <json/json.h> // jsoncpp

#include "refund-http.h"

namespace Refund
{

class HttpItem : public Item
{
public:
    HttpItem(const std::string& app_in,
             const std::string& item_in,
             Web::ClickPurchasesApi::Ptr cpa_in):
        app {app_in},
        item {item_in},
    cpa {cpa_in}
    {
    }

    virtual bool run (void)
    {
        if (app == "click-scope")
        {
            request = cpa->refundPackage(item/*package*/);
        }
        else
        {
            // TODO: sku refunds aren't defined in click-purchase-api yet...
            finished(false);
            return false;
        }

        // Ensure we get JSON back
        request->set_header("Accept", "application/json");
        request->finished.connect([this](Web::Response::Ptr response)
        {
            Json::Reader reader (Json::Features::strictMode());
            Json::Value root;
            bool success = false;

            if (reader.parse(response->body(), root) &&
                    root.isObject() &&
                    root.isMember("success"))
            {
                success = root["success"].asBool();
            }

            finished(success);
        });
        request->error.connect([this](std::string error)
        {
            std::cerr << "Error verifying item '" << error << std::endl;
            finished(false);
        });
        request->run();

        return true;
    }

private:
    std::string app;
    std::string item;
    Web::ClickPurchasesApi::Ptr cpa;
    Web::Request::Ptr request;
};

/*********************
 * HttpFactory
 *********************/

HttpFactory::HttpFactory (Web::ClickPurchasesApi::Ptr in_cpa):
    cpa {in_cpa}
{
}

bool
HttpFactory::running()
{
    return cpa->running();
}

Item::Ptr
HttpFactory::refund (const std::string& appid, const std::string& itemid)
{
    return std::make_shared<HttpItem>(appid, itemid, cpa);
}


} // ns Refund

