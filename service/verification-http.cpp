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

namespace Verification
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
                verificationComplete(Status::PURCHASED);
            }
            else
            {
                verificationComplete(Status::NOT_PURCHASED);
            }
        });
        request->error.connect([this](std::string error)
        {
            std::cerr << "Error verifying item '" << error << std::endl;
            verificationComplete(Status::ERROR);
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
