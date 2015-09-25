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

#include <libpay/pay-package.h>

#include <chrono>

static constexpr char const * BUS_NAME {"com.canonical.payments"};

struct LibpayPackageTests: public DBusFixture
{
    void wait_for_store_service()
    {
        auto on_name_appeared = [](GDBusConnection*, const char*, const char*, gpointer gloop) {
            g_main_loop_quit(static_cast<GMainLoop*>(gloop));
        };
        auto watch_name_tag = g_bus_watch_name(G_BUS_TYPE_SESSION,
                                               BUS_NAME,
                                               G_BUS_NAME_WATCHER_FLAGS_NONE,
                                               on_name_appeared,
                                               nullptr,
                                               m_main_loop,
                                               nullptr);
        g_main_loop_run(m_main_loop);
        g_bus_unwatch_name(watch_name_tag);
    }

protected:

    GMainLoop* m_main_loop {};
    GTestDBus* m_test_bus {};

    void BeforeBusSetUp() override
    {
        // use a fake bus
        m_test_bus = g_test_dbus_new(G_TEST_DBUS_NONE);
        g_test_dbus_up(m_test_bus);

        // start the store
        const gchar* child_argv[] = { "python3", "-m", "dbusmock", "--template", STORE_TEMPLATE_PATH, nullptr };
        GError* error = nullptr;
        g_spawn_async(nullptr, (gchar**)child_argv, nullptr, G_SPAWN_SEARCH_PATH, nullptr, nullptr, nullptr, &error);
        g_assert_no_error(error);
    }

    void SetUp() override
    {
        DBusFixture::SetUp();

        m_main_loop = g_main_loop_new(nullptr, false);

        wait_for_store_service();

        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));
        auto props = g_variant_builder_end(&b);

        GVariant* args[] = { g_variant_new_string("click-scope"), props };

        GError *error {};
        auto v = g_dbus_connection_call_sync(m_bus,
                                             BUS_NAME,
                                             "/com/canonical/pay/store",
                                             "com.canonical.pay.storemock",
                                             "AddStore",
                                             g_variant_new_tuple(args, G_N_ELEMENTS(args)),
                                             nullptr,
                                             G_DBUS_CALL_FLAGS_NONE,
                                             -1,
                                             nullptr,
                                             &error);
        g_assert_no_error(error);
        g_clear_pointer(&v, g_variant_unref);
    }

    void TearDown() override
    {
        g_main_loop_unref(m_main_loop);

        DBusFixture::TearDown();

        g_clear_object(&m_test_bus);
    }

    struct StatusObserverData
    {
        PayPackage* package;
        std::string sku;
        PayPackageItemStatus status = PAY_PACKAGE_ITEM_STATUS_UNKNOWN;
        bool triggered = false;
    };

    void InstallStatusObserver(PayPackage* package,
                               StatusObserverData& data)
    {
        auto observer = [](PayPackage* package,
                           const char* sku,
                           PayPackageItemStatus status,
                           void* vdata)
        {
            auto data = static_cast<StatusObserverData*>(vdata);
            data->package = package;
            data->sku = sku;
            data->status = status;
            data->triggered = true;
        };
        auto install_result = pay_package_item_observer_install (package, observer, &data);
        EXPECT_TRUE(install_result);
    }
};

TEST_F(LibpayPackageTests, InitTest)
{
    auto package = pay_package_new("package-name");
    pay_package_delete(package);
}

TEST_F(LibpayPackageTests, PurchaseItem)
{
    GError* error = nullptr;
    guint callcount = 0;
    auto package = pay_package_new("click-scope");

    // install a status observer
    StatusObserverData data;
    InstallStatusObserver(package, data);

    const char* sku = "item";
    EXPECT_TRUE(pay_package_item_start_purchase(package, sku));

    // wait for the call to complete
    while (!data.triggered)
        g_usleep(G_USEC_PER_SEC/10);

    // confirm that the item's status changed
    EXPECT_TRUE(data.triggered);
    EXPECT_EQ(package, data.package);
    EXPECT_EQ(sku, data.sku);
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_PURCHASED, data.status);

    // cleanup
    pay_package_delete(package);
}

TEST_F(LibpayPackageTests, RefundItem)
{
    GError* error = nullptr;
    guint callcount = 0;
    auto package = pay_package_new("click-scope");

    // install a status observer
    StatusObserverData data;
    InstallStatusObserver(package, data);

    const char* sku = "item";
    EXPECT_TRUE(pay_package_item_start_refund(package, sku));

    // wait for the call to complete
    while (!data.triggered)
        g_usleep(G_USEC_PER_SEC/10);

    // confirm that the item's status changed
    EXPECT_TRUE(data.triggered);
    EXPECT_EQ(package, data.package);
    EXPECT_EQ(sku, data.sku);
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_NOT_PURCHASED, data.status);

    // cleanup
    pay_package_delete(package);
}

TEST_F(LibpayPackageTests, VerifyItem)
{
    GError* error = nullptr;
    guint callcount = 0;
    auto package = pay_package_new("click-scope");

    // install a status observer
    StatusObserverData data;
    InstallStatusObserver(package, data);

    const char* sku = "item";
    EXPECT_TRUE(pay_package_item_start_verification(package, sku));

    // wait for the call to complete
    while (!data.triggered)
        g_usleep(G_USEC_PER_SEC/10);

    // confirm that the item's status changed
    EXPECT_TRUE(data.triggered);
    EXPECT_EQ(package, data.package);
    EXPECT_EQ(sku, data.sku);
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_NOT_PURCHASED, data.status);

    // cleanup
    pay_package_delete(package);
}
