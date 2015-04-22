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
    HttpItem (std::string& app,
              std::string& item,
              std::string& endpoint,
              std::string& device,
              Web::Factory::Ptr webclient) :
        client(webclient)
    {
        url = endpoint;

        if (app != "click-scope")
        {
            url += "/" + app;
        }
        // TODO: Needs regression testing, and redirect support.
        url += "/" + item;
        if (app == "click-scope"
                && url.find("file:///") == std::string::npos)
        {
            url += "/";
        }

        if (!device.empty())
        {
            url += "?device=" + device;
        }

    }

    virtual bool run (void)
    {
        auto request = client->create_request(url, "GET", true, "");
        /* Ensure we get JSON back */
        request->set_header("Accept", "application/json");
        request->finished.connect([this](Web::Response::Ptr response)
            {
                if (response->is_success ()) {
                    verificationComplete(Status::PURCHASED);
                } else {
                    verificationComplete(Status::NOT_PURCHASED);
                }
            });
        request->error.connect([this](std::string error)
            {
                std::cerr << "Error verifying item '" << error << "' at URL '" << url << "'" << std::endl;
                verificationComplete(Status::ERROR);
            });
        request->run();

        return true;
    }
private:
    std::string url;
    Web::Factory::Ptr client;
};

/*********************
 * HttpFactory
 *********************/

HttpFactory::HttpFactory (Web::Factory::Ptr in_factory) :
    wfactory(in_factory)
{
    // TODO: We should probably always assemble the URL when needed.
    endpoint = get_base_url() + PAY_API_ROOT + PAY_PURCHASES_PATH;
}

bool
HttpFactory::running ()
{
    return wfactory->running();
}

Item::Ptr
HttpFactory::verifyItem (std::string& appid, std::string& itemid)
{
    return std::make_shared<HttpItem>(appid, itemid, endpoint, device, wfactory);
}

void
HttpFactory::setEndpoint (std::string& in_endpoint)
{
    endpoint = in_endpoint;
}

void
HttpFactory::setDevice (std::string& in_device)
{
    device = in_device;
}

std::string
HttpFactory::get_base_url ()
{
    const char* env_url = getenv(PAY_BASE_URL_ENVVAR.c_str());
    if (env_url != nullptr)
    {
        return env_url;
    }
    return PAY_BASE_URL;
}

} // ns Verification

