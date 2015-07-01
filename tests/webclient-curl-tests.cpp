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
