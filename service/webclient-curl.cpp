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
 */

#include "webclient-curl.h"

#include <string>
#include <thread>

#include <curl/curl.h>
#include <curl/easy.h>

namespace Web
{

class CurlResponse : public Response {
public:
    CurlResponse (int status, std::string body) :
        _body(body),
        _status(status)
    {
    }

    virtual std::string& body ()
    {
        return _body;
    }

    virtual bool is_success ()
    {
        return _status == 200 ? true : false;
    }

private:
    std::string _body;
    int _status;
};


class CurlRequest : public Request
{
public:
    CurlRequest (const std::string& url,
                 const std::string& method,
                 bool sign,
                 const std::string& postdata,
                 TokenGrabber::Ptr token) :
        stop(false),
        _url(url),
        _method(method),
        _sign(sign),
        _postData(postdata),
        _token(token)
    {
    }

    ~CurlRequest (void)
    {
        stopThread();
    }

    void stopThread (void)
    {
        stop = true;

        if (exec.joinable())
        {
            exec.join();
        }
    }

    virtual bool run (void)
    {
        stopThread();
        transferBuffer.clear();
        stop = false;

        /* Do the execution in another thread so we can wait on the
           network socket. */
        exec = std::thread([this]()
        {
            std::string authheader;
            CURL* handle = curl_easy_init();

            /* Helps with threads */
            curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
            curl_easy_setopt(handle, CURLOPT_URL, _url.c_str());
            curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, curlWrite);
            curl_easy_setopt(handle, CURLOPT_WRITEDATA, this);

            if (_sign) {
                /* Sign the request */
                auto auth = _token->signUrl(_url, _method);
                if (!auth.empty())
                {
                    set_header("Authorization", auth);
                }
            }

            /* Actually set them in cURL */
            if (curlHeaders != nullptr)
            {
                curl_easy_setopt(handle, CURLOPT_HTTPHEADER, curlHeaders);
            }

            /**** Do it! ****/
            auto status = curl_easy_perform(handle);

            if (status == CURLE_OK)
            {
                long responsecode = 0;
                curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &responsecode);
                
                finished(std::make_shared<CurlResponse>(responsecode,
                                                        transferBuffer));
            }
            else
            {
                error(curl_easy_strerror(status));
            }

            /* Clean up headers */
            if (curlHeaders != nullptr)
            {
                curl_slist_free_all (curlHeaders) ;
                curlHeaders = nullptr;
            }

            curl_easy_cleanup(handle);
        });

        return true;
    }

    virtual void set_header (const std::string& key,
                             const std::string& value)
    {
        std::string headerText = key;
        headerText += + ": ";
        headerText += value;
        curlHeaders = curl_slist_append(curlHeaders, headerText.c_str());
    }

private:
    std::string transferBuffer;
    std::thread exec;
    bool stop;

    struct curl_slist* curlHeaders = NULL;

    std::string _url;
    std::string _method;
    bool _sign;
    std::string _postData;
    TokenGrabber::Ptr _token;

    /* This is the callback from cURL as it does the transfer. We're
       pretty simple in that we're just putting it into a string. */
    static size_t curlWrite (void* buffer, size_t size, size_t nmemb,
                             void* user_data)
    {
        auto datasize = size * nmemb;
        CurlRequest* request = static_cast<CurlRequest*>(user_data);
        if (request->stop)
        {
            std::cout << "cURL transaction stopped prematurely" << std::endl;
            return 0;
        }
        request->transferBuffer.append(static_cast<char*>(buffer), datasize);
        return datasize;
    }
};


/*********************
 * CurlFactory
 *********************/

CurlFactory::CurlFactory (TokenGrabber::Ptr token) :
    tokenGrabber(token)
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

Request::Ptr
CurlFactory::create_request (const std::string& url,
                             const std::string& method,
                             bool sign,
                             const std::string& data)
{
    return std::make_shared<CurlRequest>(url, method, sign, data, tokenGrabber);
}

} // ns Web
