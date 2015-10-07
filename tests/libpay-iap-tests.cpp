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

#include <map>

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

    struct IAP {
        uint64_t completed_timestamp;
        uint64_t acknowledged_timestamp;
        uint64_t purchase_id;
        const char* state;

        const char* price;
        PayItemType type;

        const char* sku;
        const char* title;
        const char* description;
    };

    void CompareItemToIAP(const IAP& expected, const PayItem* item)
    {
        EXPECT_TRUE(item != nullptr);
        EXPECT_EQ(expected.acknowledged_timestamp, pay_item_get_acknowledged_timestamp(item));
        EXPECT_EQ(expected.completed_timestamp, pay_item_get_completed_timestamp(item));
        EXPECT_STREQ(expected.description, pay_item_get_description(item));
        EXPECT_STREQ(expected.price, pay_item_get_price(item));
        EXPECT_STREQ(expected.sku, pay_item_get_sku(item));
        EXPECT_STREQ(expected.title, pay_item_get_title(item));
        EXPECT_EQ(expected.type, pay_item_get_type(item));
    }

    std::map<std::string,IAP>& get_game_iaps()
    {
        static std::map<std::string,IAP> items;

        if (items.empty())
        {
            const uint64_t now = time(nullptr);
            const IAP tmp[] = {
                {      0,     0,   0, "available", "$1", PAY_ITEM_TYPE_UNLOCKABLE, "sword",  "Sword",  "A Sword." },
                { now-10,     0, 100, "approved",  "$1", PAY_ITEM_TYPE_UNLOCKABLE, "shield", "Shield", "A Shield." },
                { now-10, now-9, 101, "purchased", "$1", PAY_ITEM_TYPE_UNLOCKABLE, "amulet", "Amulet", "An Amulet." }
            };
            for (const auto& item : tmp)
                items[item.sku] = item;
        }

        return items;
    }

    void AddGame()
    {
        GVariantBuilder bitems;
        g_variant_builder_init(&bitems, G_VARIANT_TYPE("aa{sv}"));
        for(const auto it : get_game_iaps()) {
            const auto& item = it.second;
            GVariantBuilder bitem;
            g_variant_builder_init(&bitem, G_VARIANT_TYPE_VARDICT);
            struct {
                const char* key;
                GVariant* value;
            } entries[] = {
                { "completed_timestamp", g_variant_new_uint64(item.completed_timestamp) },
                { "completed_timestamp", g_variant_new_uint64(item.completed_timestamp) },
                { "acknowledged_timestamp", g_variant_new_uint64(item.acknowledged_timestamp) },
                { "description", g_variant_new_string(item.description) },
                { "price", g_variant_new_string(item.price) },
                { "purchase_id", g_variant_new_uint64(item.purchase_id) },
                { "sku", g_variant_new_string(item.sku) },
                { "state", g_variant_new_string(item.state) },
                { "title", g_variant_new_string(item.title) },
                { "type", g_variant_new_string(item.type==PAY_ITEM_TYPE_UNLOCKABLE ? "unlockable" : "consumable") }
            };
            for (const auto& entry : entries)
                g_variant_builder_add(&bitem, "{sv}", entry.key, entry.value);
            g_variant_builder_add_value(&bitems, g_variant_builder_end(&bitem));
        }
        auto props = g_variant_builder_end(&bitems);

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

    for(const auto it : get_game_iaps())
    {
        const auto& iap = it.second;
        auto item = pay_package_get_item(package, iap.sku);
        EXPECT_TRUE(item != nullptr);
        CompareItemToIAP(iap, item);
        pay_item_unref(item);
    }

    pay_package_delete(package);
}

TEST_F(IapTests, GetPurchasedItems)
{
    AddGame();

    // calculate what we expect
    std::map<std::string,IAP> purchased;
    for(const auto it : get_game_iaps()) {
        const auto& iap = it.second;
        if (!g_strcmp0(iap.state,"approved") || !g_strcmp0(iap.state,"purchased"))
            purchased[iap.sku] = iap;
    }

    auto package = pay_package_new(GAME_NAME);
    auto items = pay_package_get_purchased_items(package);

    // test the results
    size_t i = 0;
    while (items && items[i]) {
        auto& item = items[i];
        auto it = purchased.find(pay_item_get_sku(item));
        ASSERT_NE(it, purchased.end());
        ASSERT_NE(0, pay_item_get_purchase_id(item));
        CompareItemToIAP(it->second, item);
        purchased.erase(it);
        pay_item_unref(item);
        ++i;
    }
    EXPECT_TRUE(purchased.empty());

    free(items);
    pay_package_delete(package);
}

TEST_F(IapTests, PurchaseItem)
{
    AddGame();

    auto package = pay_package_new(GAME_NAME);
    const char* sku = "sword";
    // precondition: confirm the item being tested isn't purchased yet
    ASSERT_STREQ("available", get_game_iaps()[sku].state);
    const auto expected_status = PAY_PACKAGE_ITEM_STATUS_APPROVED;

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
    EXPECT_NE(0, pay_item_get_completed_timestamp(item));
    ASSERT_NE(0, pay_item_get_purchase_id(item));
    g_clear_pointer(&item, pay_item_unref);

    // cleanup
    pay_package_delete(package);
}

TEST_F(IapTests, AcknowledgeItem)
{
    AddGame();

    auto package = pay_package_new(GAME_NAME);
    const char* sku = "shield";
    // precondition: confirm the item being tested is approved but not acked
    ASSERT_STREQ("approved", get_game_iaps()[sku].state);

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
    EXPECT_NE(0, pay_item_get_acknowledged_timestamp(item));
    ASSERT_NE(0, pay_item_get_purchase_id(item));
    g_clear_pointer(&item, pay_item_unref);

    // cleanup
    pay_package_delete(package);
}

TEST_F(IapTests, NoStore)
{
    auto package = pay_package_new(GAME_NAME);

    auto items = pay_package_get_purchased_items (package);
    ASSERT_TRUE(items != nullptr);
    ASSERT_TRUE(items[0] == nullptr);

    auto item = pay_package_get_item (package, "twizzle.twazzle.twozzle.twome");
    ASSERT_FALSE(item);

    // cleanup
    free(items);
    pay_package_delete(package);
}
