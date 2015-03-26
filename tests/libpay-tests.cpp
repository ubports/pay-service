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

#include <gio/gio.h>
#include <gtest/gtest.h>
#include <libdbustest/dbus-test.h>

#include <libpay/pay-package.h>

#include <chrono>

struct LibPayTests : public ::testing::Test
{
protected:
    DbusTestService* service = nullptr;
    DbusTestDbusMock* mock = nullptr;
    DbusTestDbusMockObject* obj = nullptr;
    DbusTestDbusMockObject* pkgobj = nullptr;
    GDBusConnection* bus = nullptr;

    virtual void SetUp()
    {
        service = dbus_test_service_new(nullptr);

        mock = dbus_test_dbus_mock_new("com.canonical.pay");

        obj = dbus_test_dbus_mock_get_object(mock, "/com/canonical/pay", "com.canonical.pay", nullptr);

        dbus_test_dbus_mock_object_add_method(mock, obj,
                                              "ListPackages",
                                              nullptr,
                                              G_VARIANT_TYPE("ao"), /* out */
                                              "ret = [ dbus.ObjectPath('/com/canonical/pay/package_2Dname') ]", /* python */
                                              nullptr); /* error */

        pkgobj = dbus_test_dbus_mock_get_object(mock, "/com/canonical/pay/package_2Dname", "com.canonical.pay.package", nullptr);

        dbus_test_dbus_mock_object_add_method(mock, pkgobj,
                                              "VerifyItem",
                                              G_VARIANT_TYPE_STRING,
                                              nullptr, /* out */
                                              "", /* python */
                                              nullptr); /* error */

        dbus_test_dbus_mock_object_add_method(mock, pkgobj,
                                              "PurchaseItem",
                                              G_VARIANT_TYPE_STRING,
                                              nullptr, /* out */
                                              "", /* python */
                                              nullptr); /* error */

        dbus_test_dbus_mock_object_add_method(mock, pkgobj,
                                              "RefundItem",
                                              G_VARIANT_TYPE_STRING,
                                              nullptr, /* out */
                                              "", /* python */
                                              nullptr); /* error */

        dbus_test_service_add_task(service, DBUS_TEST_TASK(mock));

        dbus_test_service_start_tasks(service);

        bus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
        g_dbus_connection_set_exit_on_close(bus, FALSE);
        g_object_add_weak_pointer(G_OBJECT(bus), (gpointer*)&bus);
    }

    virtual void TearDown()
    {
        g_clear_object(&mock);
        g_clear_object(&service);

        g_object_unref(bus);

        unsigned int cleartry = 0;
        while (bus != nullptr && cleartry < 100)
        {
            g_usleep(100000);
            while (g_main_pending())
            {
                g_main_iteration(TRUE);
            }
            cleartry++;
        }

        ASSERT_LT(cleartry, 100);
    }
};

TEST_F(LibPayTests, InitTest)
{
    auto package = pay_package_new("package-name");
    pay_package_delete(package);
}

TEST_F(LibPayTests, ItemLifecycle)
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
    dbus_test_dbus_mock_object_emit_signal(mock, pkgobj,
                                           "ItemStatusChanged",
                                           G_VARIANT_TYPE("(sst)"),
                                           g_variant_new("(sst)", "item", "verifying", 0),
                                           &error);
    ASSERT_EQ(nullptr, error);

    dbus_test_dbus_mock_object_emit_signal(mock, pkgobj,
                                           "ItemStatusChanged",
                                           G_VARIANT_TYPE("(sst)"),
                                           g_variant_new("(sst)", "item", "not purchased", 0),
                                           &error);
    ASSERT_EQ(nullptr, error);

    dbus_test_dbus_mock_object_emit_signal(mock, pkgobj,
                                           "ItemStatusChanged",
                                           G_VARIANT_TYPE("(sst)"),
                                           g_variant_new("(sst)", "item", "purchasing", 0),
                                           &error);
    ASSERT_EQ(nullptr, error);

    dbus_test_dbus_mock_object_emit_signal(mock, pkgobj,
                                           "ItemStatusChanged",
                                           G_VARIANT_TYPE("(sst)"),
                                           g_variant_new("(sst)", "item", "purchased", 0),
                                           &error);
    ASSERT_EQ(nullptr, error);

    guint64 refundtime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() + std::chrono::hours{1});
    dbus_test_dbus_mock_object_emit_signal(mock, pkgobj,
                                           "ItemStatusChanged",
                                           G_VARIANT_TYPE("(sst)"),
                                           g_variant_new("(sst)", "item", "purchased", refundtime),
                                           &error);
    ASSERT_EQ(nullptr, error);

    guint64 shorttime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() + std::chrono::seconds{30});
    dbus_test_dbus_mock_object_emit_signal(mock, pkgobj,
                                           "ItemStatusChanged",
                                           G_VARIANT_TYPE("(sst)"),
                                           g_variant_new("(sst)", "item", "purchased", shorttime),
                                           &error);
    ASSERT_EQ(nullptr, error);

    guint64 expiredtime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() - std::chrono::minutes{1});
    dbus_test_dbus_mock_object_emit_signal(mock, pkgobj,
                                           "ItemStatusChanged",
                                           G_VARIANT_TYPE("(sst)"),
                                           g_variant_new("(sst)", "item", "purchased", expiredtime),
                                           &error);
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

    dbus_test_dbus_mock_object_emit_signal(mock, pkgobj,
                                           "ItemStatusChanged",
                                           G_VARIANT_TYPE("(sst)"),
                                           g_variant_new("(sst)", "item", "purchasing", 0),
                                           &error);
    EXPECT_EQ(nullptr, error);

    /* Wait for the signal to make it over */
    usleep(100000);

    EXPECT_EQ(0, statusList.size());
    EXPECT_EQ(0, refundList.size());

    pay_package_delete(package);
}

TEST_F(LibPayTests, ItemOperations)
{
    GError* error = nullptr;
    guint callcount = 0;
    auto package = pay_package_new("package-name");

    EXPECT_TRUE(pay_package_item_start_verification(package, "item"));

    /* Wait for the call to make it over */
    usleep(100000);

    auto calls = dbus_test_dbus_mock_object_get_method_calls(mock, pkgobj,
                                                             "VerifyItem",
                                                             &callcount,
                                                             &error);

    ASSERT_EQ(nullptr, error);
    ASSERT_EQ(1, callcount);
    EXPECT_TRUE(g_variant_equal(calls[0].params, g_variant_new("(s)", "item")));


    EXPECT_TRUE(pay_package_item_start_purchase(package, "item2"));

    /* Wait for the call to make it over */
    usleep(100000);

    calls = dbus_test_dbus_mock_object_get_method_calls(mock, pkgobj,
                                                        "PurchaseItem",
                                                        &callcount,
                                                        &error);

    ASSERT_EQ(nullptr, error);
    ASSERT_EQ(1, callcount);
    EXPECT_TRUE(g_variant_equal(calls[0].params, g_variant_new("(s)", "item2")));

    EXPECT_TRUE(pay_package_item_start_refund(package, "item3"));

    /* Wait for the call to make it over */
    usleep(100000);

    calls = dbus_test_dbus_mock_object_get_method_calls(mock, pkgobj,
                                                        "RefundItem",
                                                        &callcount,
                                                        &error);

    ASSERT_EQ(nullptr, error);
    ASSERT_EQ(1, callcount);
    EXPECT_TRUE(g_variant_equal(calls[0].params, g_variant_new("(s)", "item3")));

    pay_package_delete(package);
}
