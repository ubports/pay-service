/*
 * Copyright © 2014-2015 Canonical Ltd.
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

#include "verification-http.h"

#include <json/json.h>
#include <time.h>


namespace Verification
{

static time_t parse_iso_utc_timestamp(const std::string& isotime)
{
    if (isotime.empty()) {
        return 0;
    }

    struct tm time_parts;

    memset(&time_parts, 0, sizeof(struct tm));
    strptime(isotime.c_str(), "%Y-%m-%dT%H:%M:%OS%z", &time_parts);
    return mktime(&time_parts);
}

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
        /* There are two forms here for appid + itemid:
           1. if appid is 'click-scope', we're verifying the package itself which is in 'item'
           2. otherwise, we're verifying an iap where app is the package and item is the item
           Yes, this is confusing. :-) */
        if (app != "click-scope")
        {
            request = cpa->getItemInfo(app/*package*/, item/*item*/);
        }
        else
        {
            request = cpa->getPackageInfo(item/*package*/, false);
        }

        // Ensure we get JSON back
        request->set_header("Accept", "application/json");
        request->finished.connect([this](Web::Response::Ptr response)
        {
            if (response->is_success ())
            {
                Json::Reader reader(Json::Features::strictMode());
                Json::Value root;
                reader.parse(response->body(), root);

                if (app == "click-scope")
                {
                    if  (root.isObject() && root.isMember("state"))
                    {
                        auto state = root["state"].asString();
                        uint64_t refundable_until{0};
                        if (root.isMember("refundable_until"))
                        {
                            auto tmp_r = root["refundable_until"].asString();
                            std::cerr << "DEBUG: refundable_until: " << tmp_r.c_str() << std::endl;
                            refundable_until = parse_iso_utc_timestamp(tmp_r);
                            std::cerr << "DEBUG: parsed_refundable: " << refundable_until << std::endl;
                        }
                        verificationComplete(Status::PURCHASED,
                                             refundable_until);
                    }
                }
                else
                {
                    if (root.isObject() && root.isMember("state"))
                    {
                        auto state = root["state"].asString();
                        if (state == "available")
                        {
                            verificationComplete(Status::NOT_PURCHASED, 0);
                        }
                        else if (state == "purchased")
                        {
                            verificationComplete(Status::PURCHASED, 0);
                        }
                        else if (state == "approved")
                        {
                            verificationComplete(Status::APPROVED, 0);
                        }
                        else
                        {
                            verificationComplete(Status::ERROR, 0);
                        }
                    }
                }
            }
            else
            {
                verificationComplete(Status::NOT_PURCHASED, 0);
            }
        });
        request->error.connect([this](std::string error)
        {
            std::cerr << "Error verifying item '" << error << std::endl;
            verificationComplete(Status::ERROR, 0);
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
HttpFactory::verifyItem (const std::string& appid, const std::string& itemid)
{
    return std::make_shared<HttpItem>(appid, itemid, cpa);
}


} // ns Verification

