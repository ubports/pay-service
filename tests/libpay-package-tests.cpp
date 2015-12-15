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

        const guint64 now = time(nullptr);
        const struct {
            const char * sku;
            const char * state;
            guint64 refundable_until;
        } prefab_apps[] = {
            { "available_app",       "available", 0 },           // not purchased
            { "newly_purchased_app", "purchased", now+(60*14) }, // purchased, refund window open
            { "old_purchased_app",   "purchased", now-(60*30) }  // purchased, refund window closed
        };
        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));
        for (const auto& app : prefab_apps) {
            GVariantBuilder app_props;
            g_variant_builder_init(&app_props, G_VARIANT_TYPE_VARDICT);
            const struct {
                const char* key;
                GVariant* value;
            } entries[] = {
                { "sku", g_variant_new_string(app.sku) },
                { "state", g_variant_new_string(app.state) },
                { "refundable_until", g_variant_new_uint64(app.refundable_until) }
            };
            for (const auto& entry : entries) 
                g_variant_builder_add(&app_props, "{sv}", entry.key, entry.value);
            g_variant_builder_add_value(&b, g_variant_builder_end(&app_props));
        }
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
        uint64_t num_calls = 0;
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
            data->num_calls++;
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
    auto package = pay_package_new("click-scope");
    const char* sku {"available_app"};

    // pre-purchase tests
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_NOT_PURCHASED, pay_package_item_status(package, sku));
    EXPECT_EQ(PAY_PACKAGE_REFUND_STATUS_NOT_PURCHASED, pay_package_refund_status(package, sku));

    // install a status observer
    StatusObserverData data;
    InstallStatusObserver(package, data);

    EXPECT_TRUE(pay_package_item_start_purchase(package, sku));

    // wait for the call to complete
    while (data.num_calls < 2) {
        g_usleep(G_USEC_PER_SEC/10);
    }

    // confirm that the item's status changed
    EXPECT_EQ(2, data.num_calls);
    EXPECT_EQ(package, data.package);
    EXPECT_EQ(sku, data.sku);
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_PURCHASED, data.status);
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_PURCHASED, pay_package_item_status(package, sku));

    // cleanup
    pay_package_delete(package);
}

TEST_F(LibpayPackageTests, PurchaseItemCancelled)
{
    auto package = pay_package_new("click-scope");

    // install a status observer
    StatusObserverData data;
    InstallStatusObserver(package, data);

    const char* sku = "cancel";
    EXPECT_TRUE(pay_package_item_start_purchase(package, sku));

    // wait for the call to complete
    while (data.num_calls < 2) {
        g_usleep(G_USEC_PER_SEC/10);
    }

    // confirm that the item's status changed
    EXPECT_EQ(2, data.num_calls);
    EXPECT_EQ(package, data.package);
    EXPECT_EQ(sku, data.sku);
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_NOT_PURCHASED, data.status);

    // cleanup
    pay_package_delete(package);
}

TEST_F(LibpayPackageTests, PurchaseItemError)
{
    auto package = pay_package_new("click-scope");

    // install a status observer
    StatusObserverData data;
    InstallStatusObserver(package, data);

    const char* sku = "denied";
    EXPECT_TRUE(pay_package_item_start_purchase(package, sku));

    // wait for the call to complete
    while (data.num_calls < 2) {
        g_usleep(G_USEC_PER_SEC/10);
    }

    // confirm that the item's status changed
    EXPECT_EQ(2, data.num_calls);
    EXPECT_EQ(package, data.package);
    EXPECT_EQ(sku, data.sku);
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_UNKNOWN, data.status);

    // cleanup
    pay_package_delete(package);
}

TEST_F(LibpayPackageTests, RefundItem)
{
    auto package = pay_package_new("click-scope");
    const char* sku {"newly_purchased_app"};

    // pre-refund tests
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_PURCHASED, pay_package_item_status(package, sku));
    EXPECT_EQ(PAY_PACKAGE_REFUND_STATUS_REFUNDABLE, pay_package_refund_status(package, sku));

    // install a status observer
    StatusObserverData data;
    InstallStatusObserver(package, data);

    EXPECT_TRUE(pay_package_item_start_refund(package, sku));

    // wait for the call to complete
    while (!data.num_calls) {
        g_usleep(G_USEC_PER_SEC/10);
    }

    // confirm that the item's status changed
    EXPECT_EQ(1, data.num_calls);
    EXPECT_EQ(package, data.package);
    EXPECT_EQ(sku, data.sku);
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_NOT_PURCHASED, data.status);
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_NOT_PURCHASED, pay_package_item_status(package, sku));
    EXPECT_EQ(PAY_PACKAGE_REFUND_STATUS_NOT_PURCHASED, pay_package_refund_status(package, sku));

    // cleanup
    pay_package_delete(package);
}

TEST_F(LibpayPackageTests, VerifyItem)
{
    auto package = pay_package_new("click-scope");

    // install a status observer
    StatusObserverData data;
    InstallStatusObserver(package, data);

    const char* sku = "item";
    EXPECT_TRUE(pay_package_item_start_verification(package, sku));

    // wait for the call to complete
    while (!data.num_calls) {
        g_usleep(G_USEC_PER_SEC/10);
    }

    // confirm that the item's status changed
    EXPECT_EQ(1, data.num_calls);
    EXPECT_EQ(package, data.package);
    EXPECT_EQ(sku, data.sku);
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_NOT_PURCHASED, data.status);

    // cleanup
    pay_package_delete(package);
}

TEST_F(LibpayPackageTests, ColdCacheStatus)
{
    auto package = pay_package_new("click-scope");

    const struct {
        const char * sku;
        PayPackageItemStatus expected_item_status;
        PayPackageRefundStatus expected_refund_status;
    } tests[] = {
        { "available_app",       PAY_PACKAGE_ITEM_STATUS_NOT_PURCHASED,  PAY_PACKAGE_REFUND_STATUS_NOT_PURCHASED },
        { "newly_purchased_app", PAY_PACKAGE_ITEM_STATUS_PURCHASED,      PAY_PACKAGE_REFUND_STATUS_REFUNDABLE },
        { "old_purchased_app",   PAY_PACKAGE_ITEM_STATUS_PURCHASED,      PAY_PACKAGE_REFUND_STATUS_NOT_REFUNDABLE }
    };

    for (const auto& test : tests) {
        EXPECT_EQ(test.expected_item_status, pay_package_item_status(package, test.sku));
        EXPECT_EQ(test.expected_refund_status, pay_package_refund_status(package, test.sku));
    }

    // cleanup
    pay_package_delete(package);
}

