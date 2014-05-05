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

struct VerificationCurlTests : public ::testing::Test
{
	protected:
		virtual void SetUp() {
			endpoint = "file://";
			endpoint += VERIFICATION_CURL_ENDPOINTS_DIR;
		}

		virtual void TearDown() {
		}

		std::string endpoint;
};

TEST_F(VerificationCurlTests, InitTest) {
	auto verify = std::make_shared<Verification::CurlFactory>();
	EXPECT_NE(nullptr, verify);
	verify->setEndpoint(endpoint);
	verify.reset();
	EXPECT_EQ(nullptr, verify);
}
