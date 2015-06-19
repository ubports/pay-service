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

#include <chrono>
#include <condition_variable>
#include <mutex>

#include <gtest/gtest.h>

#include "token-grabber-null.h"
#include "service/refund-http.h"
#include "service/webclient-curl.h"

// TODO(charles): Look for common ground for refactoring this and VerificationHttpTests together to avoid duplicate code

struct RefundHttpTests : public ::testing::Test
{
    protected:
        virtual void SetUp() {
            web_request.expected_headers = std::map<std::string,std::string>{ { "Accept", "application/json" } };
            web_request.expected_endpoint = "https://myapps.developer.ubuntu.com/api/2.0/click/refunds/";
        }

        virtual void TearDown() {
        }

        struct Request {
            std::map<std::string,std::string> expected_headers;
            std::string expected_endpoint;
            std::string fake_endpoint;
        };

        Request web_request;

        static Refund::Factory::Ptr create_vfactory(Request& req,
                                                          const std::string& device_id="")
        {
            if (!device_id.empty())
                req.expected_headers["X-Device-Id"] = device_id;

            // create a wfactory that will test the expected url/headers and inject our own values
            auto token = std::make_shared<TokenGrabberNull>();
            auto wfactory = std::make_shared<Web::CurlFactory>(token);
            wfactory->setPreWebHook([req](std::string& url, std::map<std::string,std::string>& headers){
                // confirm that we /would have/ used the values we expected...
                for (auto& kv : req.expected_headers) {
                    EXPECT_EQ(1, headers.count(kv.first));
                    EXPECT_EQ(kv.second, headers[kv.first]);
                }
                EXPECT_EQ(0, url.find(req.expected_endpoint));
                // ...and replace it with our fake, s.t. we read from the test sandbox
                url = req.fake_endpoint;
            });

            auto cpa = std::make_shared<Web::ClickPurchasesApi>(wfactory);
            if (!device_id.empty())
                cpa->setDevice(device_id);

            return std::make_shared<Refund::HttpFactory>(cpa);
        }

        /* Run the item, blocking the thread until finished or timeout.
                   Timeout triggers a failed gtest-expect */
        static void run_item(Refund::Item::Ptr item,
                             std::function<void(bool)> on_completed)
        {
            std::mutex m;
            std::condition_variable cv;
            auto func = [&on_completed, &cv](bool success) {
                on_completed(success);
                cv.notify_all();
            };
            core::ScopedConnection connection = item->finished.connect(func);
            std::unique_lock<std::mutex> lk(m);
            EXPECT_TRUE(item->run());
            const std::chrono::seconds timeout_duration{1};
            EXPECT_EQ(std::cv_status::no_timeout, cv.wait_for(lk, timeout_duration));
        }

        /* Run the item and test its final status */
        void run_item_and_test_result(Refund::Item::Ptr item,
                                      bool expected_success)
        {
            bool success;
            auto on_completed = [&success](bool s){success = s;};
            run_item(item, on_completed);
            EXPECT_EQ(expected_success, success);
        }

        /* Run the item but ignore its final status;
                   e.g. if our test is about the HTTP headers or url */
        void run_item(Refund::Item::Ptr item) 
        {
            run_item(item, [](bool){});
        }
};

TEST_F(RefundHttpTests, SimpleTests)
{
    const std::string endpoints_dir = std::string("file://") + REFUND_CURL_ENDPOINTS_DIR;

    const struct {
        std::string package_name;
        std::string item_name;
        std::string url;
        bool expected_result;
    } tests[] = {
        // this payload contains a true success flag
        { "click-scope", "com.ubuntu.developer.dev.appname", endpoints_dir+"/success.json", true },
        // this payload contains a false success flag
        { "click-scope", "com.ubuntu.developer.dev.appname", endpoints_dir+"/fail.json", false },
        // this file can't be read and so we should fail gracefully with success: false
        { "click-scope", "com.ubuntu.developer.dev.appname", "file:///dev/null", false }
    };

    for (const auto& test : tests)
    {
        web_request.fake_endpoint = test.url;
        auto vfactory = create_vfactory(web_request);
        auto item = vfactory->refund(test.package_name, test.item_name);
        run_item_and_test_result(item, test.expected_result);
    }
}
