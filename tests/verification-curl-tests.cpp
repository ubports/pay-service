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

#include <gtest/gtest.h>
#include "service/verification-curl.h"
#include "token-grabber-null.h"

struct VerificationCurlTests : public ::testing::Test
{
	protected:
		virtual void SetUp() {
			endpoint = "file://";
			endpoint += VERIFICATION_CURL_ENDPOINTS_DIR;

			device = "1234";
		}

		virtual void TearDown() {
		}

		std::string endpoint;
		std::string device;
};

TEST_F(VerificationCurlTests, InitTest) {
	auto token = std::make_shared<TokenGrabberNull>();
	auto verify = std::make_shared<Verification::CurlFactory>(token);
	EXPECT_NE(nullptr, verify);
	verify->setEndpoint(endpoint);
	verify.reset();
	EXPECT_EQ(nullptr, verify);
}

TEST_F(VerificationCurlTests, PurchaseItem) {
	auto token = std::make_shared<TokenGrabberNull>();
	auto verify = std::make_shared<Verification::CurlFactory>(token);
	ASSERT_NE(nullptr, verify);
	verify->setEndpoint(endpoint);

	std::string appid("good");
	std::string itemid("simple");

	auto item = verify->verifyItem(appid, itemid);
	ASSERT_NE(nullptr, item);

	Verification::Item::Status status = Verification::Item::Status::ERROR;
	item->verificationComplete.connect([&status] (Verification::Item::Status in_status) { status = in_status; });

	ASSERT_TRUE(item->run());
	usleep(20 * 1000);

	EXPECT_EQ(Verification::Item::Status::NOT_PURCHASED, status);

	/* Verify it can be run twice on the same item */
	status = Verification::Item::Status::ERROR;

	ASSERT_TRUE(item->run());
	usleep(20 * 1000);

	EXPECT_EQ(Verification::Item::Status::NOT_PURCHASED, status);

	/* Bad App ID */
	std::string badappid("bad");
	auto baditem = verify->verifyItem(badappid, itemid);
	ASSERT_NE(nullptr, item);

	Verification::Item::Status badstatus = Verification::Item::Status::ERROR;
	item->verificationComplete.connect([&badstatus] (Verification::Item::Status in_status) { badstatus = in_status; });

	ASSERT_TRUE(baditem->run());
	usleep(20 * 1000);

	EXPECT_EQ(Verification::Item::Status::ERROR, badstatus);
}

TEST_F(VerificationCurlTests, ClickScope) {
	auto token = std::make_shared<TokenGrabberNull>();
	auto verify = std::make_shared<Verification::CurlFactory>(token);
	ASSERT_NE(nullptr, verify);
	verify->setEndpoint(endpoint);

	std::string appid("click-scope");
	std::string itemid("package-name");

	auto item = verify->verifyItem(appid, itemid);
	ASSERT_NE(nullptr, item);

	Verification::Item::Status status = Verification::Item::Status::ERROR;
	item->verificationComplete.connect([&status] (Verification::Item::Status in_status) { status = in_status; });

	ASSERT_TRUE(item->run());
	usleep(20 * 1000);

	EXPECT_EQ(Verification::Item::Status::NOT_PURCHASED, status);
}

TEST_F(VerificationCurlTests, DeviceId) {
	auto token = std::make_shared<TokenGrabberNull>();
	auto verify = std::make_shared<Verification::CurlFactory>(token);
	ASSERT_NE(nullptr, verify);
	verify->setEndpoint(endpoint);
	verify->setDevice(device);

	std::string appid("good");
	std::string itemid("device-id");

	auto item = verify->verifyItem(appid, itemid);
	ASSERT_NE(nullptr, item);

	Verification::Item::Status status = Verification::Item::Status::ERROR;
	item->verificationComplete.connect([&status] (Verification::Item::Status in_status) { status = in_status; });

	ASSERT_TRUE(item->run());
	usleep(20 * 1000);

	EXPECT_EQ(Verification::Item::Status::NOT_PURCHASED, status);
}

TEST_F(VerificationCurlTests, testGetBaseUrl)
{
    const char *value = getenv(Verification::PAY_BASE_URL_ENVVAR.c_str());
    if (value != nullptr) {
        ASSERT_EQ(0, unsetenv(Verification::PAY_BASE_URL_ENVVAR.c_str()));
    }
    ASSERT_EQ(Verification::PAY_BASE_URL, Verification::CurlFactory::get_base_url());
    
}

TEST_F(VerificationCurlTests, testGetBaseUrlFromEnv)
{
    const std::string expected{"http://localhost:8080"};
    ASSERT_EQ(0, setenv(Verification::PAY_BASE_URL_ENVVAR.c_str(),
                        expected.c_str(), 1));
    ASSERT_EQ(expected, Verification::CurlFactory::get_base_url());
    ASSERT_EQ(0, unsetenv(Verification::PAY_BASE_URL_ENVVAR.c_str()));
}
