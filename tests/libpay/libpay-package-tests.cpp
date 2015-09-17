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

#include "dbus-fixture.h"
#include "mock-package-service.h"

#include <libpay/pay-package.h>

#include <chrono>

struct LibpayPackageTests: public DBusFixture
{
protected:

    MockPackageService* packageService = nullptr;

    void BeforeBusSetUp() override
    {
        packageService = new MockPackageService{};
    }

    void BeforeBusTearDown() override
    {
        delete packageService;
        packageService = nullptr;
    }
};

TEST_F(LibpayPackageTests, InitTest)
{
    auto package = pay_package_new("package-name");
    pay_package_delete(package);
}

TEST_F(LibpayPackageTests, ItemLifecycle)
{
    auto package = pay_package_new("package-name");

    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_UNKNOWN, pay_package_item_status(package, "item"));

    /* Install a status observer */
    std::vector<std::pair<std::string, PayPackageItemStatus>> statusList;
    auto statusfunc = [](PayPackage * pkg,
                         const char * itemid,
                         PayPackageItemStatus status,
                         void * user_data)
    {
        std::cout << "Status changed: " << itemid << " to: " << status << std::endl;
        auto list = reinterpret_cast<std::vector<std::pair<std::string, PayPackageItemStatus>> *>(user_data);
        std::pair<std::string, PayPackageItemStatus> pair(std::string(itemid), status);
        list->push_back(pair);
    };
    EXPECT_TRUE(pay_package_item_observer_install(package, statusfunc, &statusList));

    /* Install a refund observer */
    std::vector<std::pair<std::string, PayPackageRefundStatus>> refundList;
    auto refundfunc = [](PayPackage * pkg,
                         const char * itemid,
                         PayPackageRefundStatus status,
                         void * user_data)
    {
        std::cout << "        refund: " << itemid << " to: " << status << std::endl;
        auto list = reinterpret_cast<std::vector<std::pair<std::string, PayPackageRefundStatus>> *>(user_data);
        std::pair<std::string, PayPackageRefundStatus> pair(std::string(itemid), status);
        list->push_back(pair);
    };
    EXPECT_TRUE(pay_package_refund_observer_install(package, refundfunc, &refundList));

    /* Wait for the thread to start on ARM */
    usleep(100000);

    GError* error = nullptr;
    packageService->emit_item_status_changed("item", "verifying", 0, &error);
    ASSERT_EQ(nullptr, error);

    packageService->emit_item_status_changed("item", "not purchased", 0, &error);
    ASSERT_EQ(nullptr, error);

    packageService->emit_item_status_changed("item", "purchasing", 0, &error);
    ASSERT_EQ(nullptr, error);

    packageService->emit_item_status_changed("item", "purchased", 0, &error);
    ASSERT_EQ(nullptr, error);

    guint64 refundtime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() + std::chrono::hours{1});
    packageService->emit_item_status_changed("item", "purchased", refundtime, &error);
    ASSERT_EQ(nullptr, error);

    guint64 shorttime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() + std::chrono::seconds{30});
    packageService->emit_item_status_changed("item", "purchased", shorttime, &error);
    ASSERT_EQ(nullptr, error);

    guint64 expiredtime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() - std::chrono::minutes{1});
    packageService->emit_item_status_changed("item", "purchased", expiredtime, &error);
    ASSERT_EQ(nullptr, error);

    /* Wait for the signal to make it over */
    usleep(100000);

    /* Check stati */
    ASSERT_EQ(7, statusList.size());
    EXPECT_EQ("item", statusList[0].first);
    EXPECT_EQ("item", statusList[1].first);
    EXPECT_EQ("item", statusList[2].first);
    EXPECT_EQ("item", statusList[3].first);
    EXPECT_EQ("item", statusList[4].first);
    EXPECT_EQ("item", statusList[5].first);
    EXPECT_EQ("item", statusList[6].first);

    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_VERIFYING,     statusList[0].second);
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_NOT_PURCHASED, statusList[1].second);
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_PURCHASING,    statusList[2].second);
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_PURCHASED,     statusList[3].second);
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_PURCHASED,     statusList[4].second);
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_PURCHASED,     statusList[5].second);
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_PURCHASED,     statusList[6].second);

    /* Check refund stati */
    ASSERT_EQ(7, refundList.size());
    EXPECT_EQ("item", refundList[0].first);
    EXPECT_EQ("item", refundList[1].first);
    EXPECT_EQ("item", refundList[2].first);
    EXPECT_EQ("item", refundList[3].first);
    EXPECT_EQ("item", refundList[4].first);
    EXPECT_EQ("item", refundList[5].first);
    EXPECT_EQ("item", refundList[6].first);

    EXPECT_EQ(PAY_PACKAGE_REFUND_STATUS_NOT_PURCHASED,   refundList[0].second);
    EXPECT_EQ(PAY_PACKAGE_REFUND_STATUS_NOT_PURCHASED,   refundList[1].second);
    EXPECT_EQ(PAY_PACKAGE_REFUND_STATUS_NOT_PURCHASED,   refundList[2].second);
    EXPECT_EQ(PAY_PACKAGE_REFUND_STATUS_NOT_REFUNDABLE,  refundList[3].second);
    EXPECT_EQ(PAY_PACKAGE_REFUND_STATUS_REFUNDABLE,      refundList[4].second);
    EXPECT_EQ(PAY_PACKAGE_REFUND_STATUS_WINDOW_EXPIRING, refundList[5].second);
    EXPECT_EQ(PAY_PACKAGE_REFUND_STATUS_NOT_REFUNDABLE,  refundList[6].second);

    /* Check the accessors */
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_PURCHASED, pay_package_item_status(package, "item"));
    EXPECT_EQ(PAY_PACKAGE_REFUND_STATUS_NOT_REFUNDABLE, pay_package_refund_status(package, "item"));
    EXPECT_FALSE(pay_package_item_is_refundable(package, "item"));

    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_UNKNOWN, pay_package_item_status(package, "not an item"));
    EXPECT_EQ(PAY_PACKAGE_REFUND_STATUS_NOT_PURCHASED, pay_package_refund_status(package, "not an item"));
    EXPECT_FALSE(pay_package_item_is_refundable(package, "not an item"));

    /* Remove the callbacks */
    EXPECT_TRUE(pay_package_item_observer_uninstall(package, statusfunc, &statusList));
    EXPECT_TRUE(pay_package_refund_observer_uninstall(package, refundfunc, &refundList));

    /* Let's make sure we stop getting events as well */
    statusList.clear();
    refundList.clear();

    packageService->emit_item_status_changed("item", "purchasing", 0, &error);
    EXPECT_EQ(nullptr, error);

    /* Wait for the signal to make it over */
    usleep(100000);

    EXPECT_EQ(0, statusList.size());
    EXPECT_EQ(0, refundList.size());

    pay_package_delete(package);
}

TEST_F(LibpayPackageTests, ItemOperations)
{
    GError* error = nullptr;
    guint callcount = 0;
    auto package = pay_package_new("package-name");

    EXPECT_TRUE(pay_package_item_start_verification(package, "item"));

    /* Wait for the call to make it over */
    usleep(100000);

    auto calls = packageService->get_method_calls("VerifyItem", &callcount, &error);
    ASSERT_EQ(nullptr, error);
    ASSERT_EQ(1, callcount);
    auto expected_v = g_variant_new("(s)", "item");
    EXPECT_TRUE(g_variant_equal(expected_v, calls[0].params));
    g_clear_pointer(&expected_v, g_variant_unref);

/* FIXME(charles) re-enable this test after we get the test harness
 * running the old and new servers side-by-side */
#if 0
    EXPECT_TRUE(pay_package_item_start_purchase(package, "item2"));

    /* Wait for the call to make it over */
    usleep(100000);
    calls = packageService->get_method_calls("PurchaseItem", &callcount, &error);

    ASSERT_EQ(nullptr, error);
    ASSERT_EQ(1, callcount);
    expected_v = g_variant_new("(s)", "item2");
    EXPECT_TRUE(g_variant_equal(expected_v, calls[0].params));
    g_clear_pointer(&expected_v, g_variant_unref);

    EXPECT_TRUE(pay_package_item_start_refund(package, "item3"));

    /* Wait for the call to make it over */
    usleep(100000);

    calls = packageService->get_method_calls("RefundItem", &callcount, &error);

    ASSERT_EQ(nullptr, error);
    ASSERT_EQ(1, callcount);
    expected_v = g_variant_new("(s)", "item3");
    EXPECT_TRUE(g_variant_equal(expected_v, calls[0].params));
    g_clear_pointer(&expected_v, g_variant_unref);
#endif

    pay_package_delete(package);
}
