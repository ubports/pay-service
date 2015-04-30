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

#include <chrono>
#include <condition_variable>
#include <mutex>

#include <gtest/gtest.h>

#include "token-grabber-null.h"
#include "service/verification-http.h"
#include "service/webclient-curl.h"

struct VerificationHttpTests : public ::testing::Test
{
	protected:
		virtual void SetUp() {
			web_request.expected_headers = std::map<std::string,std::string>{ { "Accept", "application/json" } };
			web_request.expected_endpoint = "https://software-center.ubuntu.com/api/2.0/click/purchases/";
			web_request.fake_endpoint = std::string("file://") + VERIFICATION_CURL_ENDPOINTS_DIR + '/';
		}

		virtual void TearDown() {
		}

		struct Request {
			std::map<std::string,std::string> expected_headers;
			std::string expected_endpoint;
			std::string fake_endpoint;
		};

		Request web_request;

		static Verification::Factory::Ptr create_vfactory(Request& req,
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
				url = req.fake_endpoint + url.substr(req.expected_endpoint.size());
			});

			auto cpa = std::make_shared<Web::ClickPurchasesApi>(wfactory);
			if (!device_id.empty())
				cpa->setDevice(device_id);

			return std::make_shared<Verification::HttpFactory>(cpa);
		}

		/* Run the item, blocking the thread until finished or timeout.
                   Timeout triggers a failed gtest-expect */
		static void run_item(Verification::Item::Ptr item,
		                     std::function<void(Verification::Item::Status)> on_completed)
		{
			std::mutex m;
			std::condition_variable cv;
			auto func = [&on_completed, &cv](Verification::Item::Status status) {
				on_completed(status);
				cv.notify_all();
			};
			core::ScopedConnection connection = item->verificationComplete.connect(func);
			std::unique_lock<std::mutex> lk(m);
			EXPECT_TRUE(item->run());
			const std::chrono::milliseconds timeout_duration{20};
			EXPECT_EQ(std::cv_status::no_timeout, cv.wait_for(lk, timeout_duration));
		}

		/* Run the item and test its final status */
		void run_item_expecting_status(Verification::Item::Ptr item,
		                               Verification::Item::Status expected_status)
		{
			Verification::Item::Status status;
			auto on_completed = [&status](Verification::Item::Status s){status = s;};
			run_item(item, on_completed);
			EXPECT_EQ(expected_status, status);
		}

		/* Run the item but ignore its final status;
                   e.g. if our test is about the HTTP headers or url */
		void run_item(Verification::Item::Ptr item) 
		{
			run_item(item, [](Verification::Item::Status){});
		}
};

TEST_F(VerificationHttpTests, Verify)
{
	const struct {
		const std::string appid;
		const std::string itemid;
		Verification::Item::Status expected_status;
	} tests[] = {
		{ "good", "simple", Verification::Item::Status::NOT_PURCHASED },
		{ "bad", "simple", Verification::Item::Status::ERROR },
		{ "click-scope", "package-name", Verification::Item::Status::NOT_PURCHASED }
	};

	for (auto& test : tests)
	{
		auto vfactory = create_vfactory(web_request);
		auto item = vfactory->verifyItem(test.appid, test.itemid);

		Verification::Item::Status status;
		auto on_completed = [&status](Verification::Item::Status s){status = s;};
		run_item(item, on_completed);
		EXPECT_EQ(test.expected_status, status);
	}
}

TEST_F(VerificationHttpTests, DeviceId)
{
	auto vfactory = create_vfactory(web_request, "1234");
	run_item (vfactory->verifyItem("appid", "itemid"));
}


TEST_F(VerificationHttpTests, EnvironmentVariableNotSet)
{
	const char* key {"PURCHASES_BASE_URL"};
	ASSERT_TRUE((getenv(key)==nullptr) || unsetenv(key)); // ensure it's unset

	auto vfactory = create_vfactory(web_request);
	run_item (vfactory->verifyItem("appid", "itemid"));
}

TEST_F(VerificationHttpTests, EnvironmentVariableSet)
{
	const char* key {"PURCHASES_BASE_URL"};
	web_request.expected_endpoint = "http://localhost:8080";
	ASSERT_EQ(0, setenv(key, web_request.expected_endpoint.c_str(), 1)); // ensure it's set

	auto vfactory = create_vfactory(web_request);
	run_item (vfactory->verifyItem("appid", "itemid"));
}
