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
 *
 * Authors:
 *   Charle Kerr <charles.kerr@canonical.com>
 */

#include "dbus-fixture.h"

#include <libpay/pay-item.h>
#include <libpay/pay-package.h>

static constexpr char const * BUS_NAME {"com.canonical.payments"};
static constexpr char const * GAME_NAME {"SwordsAndStacktraces.developer"};

struct IapTests: public DBusFixture
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
    }

    void TearDown() override
    {
        g_main_loop_unref(m_main_loop);

        DBusFixture::TearDown();

        g_clear_object(&m_test_bus);
    }

    void AddGame()
    {
        const auto now = time(nullptr);

        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE_VARDICT);
        g_variant_builder_add(&b, "{sv}", "acknowledged", g_variant_new_boolean(false));
        g_variant_builder_add(&b, "{sv}", "acknowledged_time", g_variant_new_uint64(0));
        g_variant_builder_add(&b, "{sv}", "description", g_variant_new_string("A Magic Sword."));
        g_variant_builder_add(&b, "{sv}", "price", g_variant_new_string("$1"));
        g_variant_builder_add(&b, "{sv}", "purchased_time", g_variant_new_uint64(now));
        g_variant_builder_add(&b, "{sv}", "sku", g_variant_new_string("magic_sword"));
        g_variant_builder_add(&b, "{sv}", "state", g_variant_new_string("approved"));
        g_variant_builder_add(&b, "{sv}", "title", g_variant_new_string("Magic Sword"));
        g_variant_builder_add(&b, "{sv}", "type", g_variant_new_string("unlockable"));
        auto sword_props = g_variant_builder_end(&b);

        g_variant_builder_init(&b, G_VARIANT_TYPE_VARDICT);
        g_variant_builder_add(&b, "{sv}", "acknowledged", g_variant_new_boolean(false));
        g_variant_builder_add(&b, "{sv}", "acknowledged_time", g_variant_new_uint64(0));
        g_variant_builder_add(&b, "{sv}", "description", g_variant_new_string("A Magic Shield."));
        g_variant_builder_add(&b, "{sv}", "price", g_variant_new_string("$1"));
        g_variant_builder_add(&b, "{sv}", "purchased_time", g_variant_new_uint64(0));
        g_variant_builder_add(&b, "{sv}", "sku", g_variant_new_string("magic_shield"));
        g_variant_builder_add(&b, "{sv}", "state", g_variant_new_string("available"));
        g_variant_builder_add(&b, "{sv}", "title", g_variant_new_string("Magic Shield"));
        g_variant_builder_add(&b, "{sv}", "type", g_variant_new_string("unlockable"));
        auto shield_props = g_variant_builder_end(&b);

        g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));
        g_variant_builder_add_value(&b, sword_props);
        g_variant_builder_add_value(&b, shield_props);
        auto props = g_variant_builder_end(&b);

        GVariant* args[] = { g_variant_new_string(GAME_NAME), props };

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


TEST_F(IapTests, GetItem)
{
    AddGame();

    auto package = pay_package_new(GAME_NAME);

    auto item = pay_package_get_item(package, "magic_sword");
    EXPECT_TRUE(item != nullptr);
    EXPECT_FALSE(pay_item_get_acknowledged(item));
    EXPECT_STREQ("A Magic Sword.", pay_item_get_description(item));
    EXPECT_STREQ("magic_sword", pay_item_get_sku(item));
    EXPECT_STREQ("$1", pay_item_get_price(item));
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_PURCHASED, pay_item_get_status(item));
    EXPECT_STREQ("Magic Sword", pay_item_get_title(item));
    EXPECT_EQ(PAY_ITEM_TYPE_UNLOCKABLE, pay_item_get_type(item));

    // cleanup
    pay_item_unref(item);
    pay_package_delete(package);
}

TEST_F(IapTests, GetPurchasedItems)
{
    AddGame();

    auto package = pay_package_new(GAME_NAME);
    auto items = pay_package_get_purchased_items(package);

    // test the results
    size_t n = 0;
    while (items && items[n]) {
        ++n;
    }

    ASSERT_EQ(1, n);
    auto item = items[0];
    EXPECT_FALSE(pay_item_get_acknowledged(item));
    EXPECT_STREQ("A Magic Sword.", pay_item_get_description(item));
    EXPECT_STREQ("magic_sword", pay_item_get_sku(item));
    EXPECT_STREQ("$1", pay_item_get_price(item));
    EXPECT_EQ(PAY_PACKAGE_ITEM_STATUS_PURCHASED, pay_item_get_status(item));
    EXPECT_STREQ("Magic Sword", pay_item_get_title(item));
    EXPECT_EQ(PAY_ITEM_TYPE_UNLOCKABLE, pay_item_get_type(item));

    // cleanup
    for (size_t i=0; i<n; ++i) {
        pay_item_unref(items[i]);
    }

    free(items);
    pay_package_delete(package);
}

TEST_F(IapTests, PurchaseItem)
{
    AddGame();

    auto package = pay_package_new(GAME_NAME);
    const char* sku = "magic_shield";
    const auto expected_status = PAY_PACKAGE_ITEM_STATUS_PURCHASED;

    // install a status observer
    StatusObserverData data;
    InstallStatusObserver(package, data);

    // start the purchase
    auto start_result = pay_package_item_start_purchase(package, sku);
    EXPECT_TRUE(start_result);

    // wait for the purchase to complete
    while (data.num_calls < 2) {
        g_usleep(G_USEC_PER_SEC/10);
    }

    // confirm that the item's status changed
    EXPECT_EQ(2, data.num_calls);
    EXPECT_EQ(package, data.package);
    EXPECT_EQ(sku, data.sku);
    EXPECT_EQ(expected_status, data.status);

    // now get the PayItem and test it
    auto item = pay_package_get_item(package, sku);
    EXPECT_STREQ(sku, pay_item_get_sku(item));
    EXPECT_EQ(expected_status, pay_item_get_status(item));
    EXPECT_NE(0, pay_item_get_purchased_time(item));
    g_clear_pointer(&item, pay_item_unref);

    // cleanup
    pay_package_delete(package);
}

TEST_F(IapTests, AcknowledgeItem)
{
    AddGame();

    auto package = pay_package_new(GAME_NAME);
    const char* sku = "magic_sword";

    // install a status observer
    StatusObserverData data;
    InstallStatusObserver(package, data);

    // start the purchase
    auto start_result = pay_package_item_start_acknowledge(package, sku);
    EXPECT_TRUE(start_result);

    // wait for the status observer to be called
    while (data.num_calls == 0) {
        g_usleep(G_USEC_PER_SEC/10);
    }

    // confirm that the item's status changed
    EXPECT_EQ(1, data.num_calls);
    EXPECT_EQ(package, data.package);
    EXPECT_EQ(sku, data.sku);

    // now get the PayItem and test it
    auto item = pay_package_get_item(package, sku);
    EXPECT_STREQ(sku, pay_item_get_sku(item));
    EXPECT_TRUE(pay_item_get_acknowledged(item));
    g_clear_pointer(&item, pay_item_unref);

    // cleanup
    pay_package_delete(package);
}
