/*
 * Copyright © 2014 Canonical Ltd.
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

#include "verification-curl.h"

#include <thread>

#include <curl/curl.h>
#include <curl/easy.h>

namespace Verification
{

class CurlItem : public Item
{
public:
    CurlItem (std::string& app, std::string& item, std::string& endpoint) : exec(nullptr)
    {
        url = (endpoint + "/" + app + "-" + item);
        handle = curl_easy_init();

        /* Helps with threads */
        curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, curlWrite);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, this);
    }

    ~CurlItem (void)
    {
        if (exec->joinable())
        {
            exec->join();
        }

        curl_easy_cleanup(handle);
    }

    virtual bool run (void)
    {
        transferBuffer.clear();

        /* Do the execution in another thread so we can wait on the
           network socket. */
        exec = std::make_shared<std::thread>([this]()
        {
            auto status = curl_easy_perform(handle);

            if (status == CURLE_OK)
            {
                /* TODO: Clearly we need to be a bit more sophisticated here */
                verificationComplete(Status::NOT_PURCHASED);
            }
            else
            {
                std::cerr << "CURL error '" << curl_easy_strerror(status) << "' from URL '" << url << "'" << std::endl;
                verificationComplete(Status::ERROR);
            }
        });

        return true;
    }
private:
    CURL* handle;
    std::string transferBuffer;
    std::shared_ptr<std::thread> exec;
    std::string url;

    /* This is the callback from cURL as it does the transfer. We're
       pretty simple in that we're just putting it into a string. */
    static size_t curlWrite (void* buffer, size_t size, size_t nmemb, void* user_data)
    {
        auto datasize = size * nmemb;
        //std::cout << "Got data: " << datasize << std::endl;
        CurlItem* item = static_cast<CurlItem*>(user_data);
        item->transferBuffer.append(static_cast<char*>(buffer), datasize);
        return datasize;
    }
};

CurlFactory::CurlFactory () :
    endpoint("https://launchpad.net")
{
    /* TODO: We should check to see if we have networking someday */
    curl_global_init(CURL_GLOBAL_SSL);
}

CurlFactory::~CurlFactory ()
{
    curl_global_cleanup();
}

bool
CurlFactory::running ()
{
    /* TODO: Check if we have networking */
    return true;
}

Item::Ptr
CurlFactory::verifyItem (std::string& appid, std::string& itemid)
{
    return std::make_shared<CurlItem>(appid, itemid, endpoint);
}

void
CurlFactory::setEndpoint (std::string& in_endpoint)
{
    endpoint = in_endpoint;
}

} // ns Verification
