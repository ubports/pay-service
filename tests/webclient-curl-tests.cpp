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

#include <gtest/gtest.h>
#include "service/webclient-curl.h"
#include "token-grabber-null.h"

#include <condition_variable>
#include <mutex>

#include <string>
#include <fstream>
#include <streambuf>

struct WebclientCurlTests : public ::testing::Test
{
protected:
    virtual void SetUp() {
        token = std::make_shared<TokenGrabberNull>();
    }

    virtual void TearDown() {
        token.reset();
    }

    TokenGrabber::Ptr token;

    void run_request(Web::Request::Ptr& req, Web::Response::Ptr& response, std::string& error)
    {
        std::mutex m;
        std::condition_variable cv;
        core::ScopedConnection fin = req->finished.connect([&cv,&response](Web::Response::Ptr r){
            response = r;
            cv.notify_all();
        });
        core::ScopedConnection err = req->error.connect([&cv,&error](std::string e){
            error = e;
            cv.notify_all();
        });
        std::unique_lock<std::mutex> lk(m);
        EXPECT_TRUE(req->run());
        const std::chrono::seconds timeout_duration{1};
        EXPECT_EQ(std::cv_status::no_timeout, cv.wait_for(lk, timeout_duration));
    }
};


TEST_F(WebclientCurlTests, InitTest) {
    auto factory = std::make_shared<Web::CurlFactory>(token);
    EXPECT_NE(nullptr, factory);
    factory.reset();
    EXPECT_EQ(nullptr, factory);
}

TEST_F(WebclientCurlTests, InitRequestTest) {
    auto factory = std::make_shared<Web::CurlFactory>(token);
    ASSERT_NE(nullptr, factory);

    auto request = factory->create_request("file:///tmp/unknown", false);
    EXPECT_NE(nullptr, request);
}

TEST_F(WebclientCurlTests, Post)
{
    auto factory = std::make_shared<Web::CurlFactory>(token);
    ASSERT_NE(nullptr, factory);

    auto request = factory->create_request("file:///tmp/unknown", false);
    request->set_post("name=com.ubuntu.developer.dev.appname");

    Web::Response::Ptr response;
    std::string error;
    run_request(request, response, error);

    // TODO: a fake webserver, e.g. tornado,
    // so that it can look for the POST and return
    // a real response that we can test here
    EXPECT_FALSE(error.empty());
    EXPECT_FALSE(response);
}
