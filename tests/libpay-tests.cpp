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
                                              "ret = [ dbus.ObjectPath('/com/canonical/pay/package') ]", /* python */
                                              nullptr); /* error */

        pkgobj = dbus_test_dbus_mock_get_object(mock, "/com/canonical/pay/package", "com.canonical.pay.package", nullptr);

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
    }
};

TEST_F(LibPayTests, InitTest)
{
    auto package = pay_package_new("package");
    pay_package_delete(package);
}

TEST_F(LibPayTests, ItemLifecycle)
{
    auto package = pay_package_new("package");

    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_UNKNOWN, pay_package_item_status(package, "item"));

    std::vector<std::pair<std::string, PayPackageItemStatus>> list;

    EXPECT_TRUE(pay_package_item_observer_install(package, [](PayPackage * pkg,
                                                              const char * itemid,
                                                              PayPackageItemStatus status,
                                                              void * user_data)
    {
        std::cout << "Status changed: " << itemid << " to: " << status << std::endl;
        auto list = reinterpret_cast<std::vector<std::pair<std::string, PayPackageItemStatus>> *>(user_data);
        std::pair<std::string, PayPackageItemStatus> pair(std::string(itemid), status);
        list->push_back(pair);
    }, &list));

    /* Wait for the thread to start on ARM */
    usleep(100000);

    GError* error = nullptr;
    dbus_test_dbus_mock_object_emit_signal(mock, pkgobj,
                                           "ItemStatusChanged",
                                           G_VARIANT_TYPE("(ss)"),
                                           g_variant_new("(ss)", "item", "verifying"),
                                           &error);
    EXPECT_EQ(nullptr, error);

    dbus_test_dbus_mock_object_emit_signal(mock, pkgobj,
                                           "ItemStatusChanged",
                                           G_VARIANT_TYPE("(ss)"),
                                           g_variant_new("(ss)", "item", "not purchased"),
                                           &error);
    EXPECT_EQ(nullptr, error);

    dbus_test_dbus_mock_object_emit_signal(mock, pkgobj,
                                           "ItemStatusChanged",
                                           G_VARIANT_TYPE("(ss)"),
                                           g_variant_new("(ss)", "item", "purchasing"),
                                           &error);
    EXPECT_EQ(nullptr, error);

    dbus_test_dbus_mock_object_emit_signal(mock, pkgobj,
                                           "ItemStatusChanged",
                                           G_VARIANT_TYPE("(ss)"),
                                           g_variant_new("(ss)", "item", "purchased"),
                                           &error);
    EXPECT_EQ(nullptr, error);

    /* Wait for the signal to make it over */
    usleep(100000);

    ASSERT_EQ(4, list.size());
    EXPECT_EQ("item", list[0].first);
    EXPECT_EQ("item", list[1].first);
    EXPECT_EQ("item", list[2].first);
    EXPECT_EQ("item", list[3].first);

    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_VERIFYING,     list[0].second);
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_NOT_PURCHASED, list[1].second);
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_PURCHASING,    list[2].second);
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_PURCHASED,     list[3].second);

    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_PURCHASED, pay_package_item_status(package, "item"));

    pay_package_delete(package);

    /* Let's make sure we stop getting events as well */
    list.clear();

    dbus_test_dbus_mock_object_emit_signal(mock, pkgobj,
                                           "ItemStatusChanged",
                                           G_VARIANT_TYPE("(ss)"),
                                           g_variant_new("(ss)", "item", "purchasing"),
                                           &error);
    EXPECT_EQ(nullptr, error);

    /* Wait for the signal to make it over */
    usleep(100000);

    EXPECT_EQ(0, list.size());
}

TEST_F(LibPayTests, ItemOperations)
{
    GError* error = nullptr;
    guint callcount = 0;
    auto package = pay_package_new("package");

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

    pay_package_delete(package);
}
