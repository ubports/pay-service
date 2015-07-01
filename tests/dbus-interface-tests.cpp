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
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#include <service/dbus-interface.h>
#include <service/item-null.h>
#include <service/proxy-service.h>
#include <service/proxy-package.h>

#include <core/posix/signal.h>

#include <core/testing/fork_and_run.h>
#include <core/testing/cross_process_sync.h>

#include <gio/gio.h>
#include <libdbustest/dbus-test.h>

#include <gtest/gtest.h>

#include "item-test.h"
#include <thread>

#include "glib-thread.h"

struct DbusInterfaceTests : public ::testing::Test
{
	std::shared_ptr<GLib::ContextThread> thread;
	std::shared_ptr<DbusTestService> service;

	virtual void SetUp()
	{
		thread = std::make_shared<GLib::ContextThread>(
			[]() {},
			[this]() { service.reset(); }
		);

		service = thread->executeOnThread<std::shared_ptr<DbusTestService>>([]() {
			auto service = std::shared_ptr<DbusTestService>(
				dbus_test_service_new(nullptr), 
				[](DbusTestService * service) { g_clear_object(&service); });

			if (!service)
				return service;

			dbus_test_service_start_tasks(service.get());

			return service;
		});

		ASSERT_NE(nullptr, service);
	}

	virtual void TearDown()
	{
		thread.reset();
	}
};

TEST_F(DbusInterfaceTests, BasicAllocation)
{
    auto store = std::make_shared<Item::NullStore>();
    DBusInterface* dbus = new DBusInterface(store);

    EXPECT_NE(nullptr, dbus);

    delete dbus;
    return;
}

TEST_F(DbusInterfaceTests, NullStoreTests)
{
    core::testing::CrossProcessSync cps1;

    auto service = [this, &cps1]()
    {
        auto trap = core::posix::trap_signals_for_all_subsequent_threads(
        {
            core::posix::Signal::sig_int,
            core::posix::Signal::sig_term
        });

        trap->signal_raised().connect([trap](core::posix::Signal)
        {
            trap->stop();
        });

        auto null_store = std::make_shared<Item::NullStore>();
        auto pay_service = std::make_shared<DBusInterface>(null_store);

        pay_service->connectionReady.connect([&cps1]()
        {
            cps1.try_signal_ready_for(std::chrono::seconds {2});
        });

        trap->run();

        /* Force deallocation so we can catch stuff from it */
        null_store.reset();
        pay_service.reset();

        return ::testing::Test::HasFailure() ? core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    auto client = [this, &cps1]()
    {
        EXPECT_EQ(1u,cps1.wait_for_signal_ready_for(std::chrono::seconds {2}));

        /* Service function test */
        auto service = proxy_pay_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                        G_DBUS_PROXY_FLAGS_NONE,
                                                        "com.canonical.pay",
                                                        "/com/canonical/pay",
                                                        nullptr, nullptr);
        EXPECT_NE(nullptr, service);

        /* Should have no packages starting out */
        gchar** packages = nullptr;
        EXPECT_TRUE(proxy_pay_call_list_packages_sync(service,
                                                      &packages,
                                                      nullptr, nullptr));
        EXPECT_EQ(0, g_strv_length(packages));
        g_strfreev(packages);

        /* Package proxy and getting status */
        auto package = proxy_pay_package_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                                G_DBUS_PROXY_FLAGS_NONE,
                                                                "com.canonical.pay",
                                                                "/com/canonical/pay/foopkg",
                                                                nullptr, nullptr);
        EXPECT_NE(nullptr, package);

        GVariant* itemslist = nullptr;
        EXPECT_TRUE(proxy_pay_package_call_list_items_sync(package,
                                                           &itemslist,
                                                           nullptr,
                                                           nullptr));

        EXPECT_STREQ("a(sst)", g_variant_get_type_string(itemslist));
        EXPECT_EQ(0, g_variant_n_children(itemslist));

        g_variant_unref(itemslist);

        /* Try to check on an item */
        EXPECT_FALSE(proxy_pay_package_call_verify_item_sync(package,
                                                             "bar-item-id",
                                                             nullptr, nullptr));

        /* Try to purchase an item */
        EXPECT_FALSE(proxy_pay_package_call_purchase_item_sync(package,
                                                               "bar-item-id",
                                                               nullptr, nullptr));


        g_clear_object(&service);

        return ::testing::Test::HasFailure() ? core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    EXPECT_EQ(core::testing::ForkAndRunResult::empty, core::testing::fork_and_run(service, client));
}

void signalAppend (GObject* obj, const gchar* itemid, const gchar* status, guint64 refundtime, std::vector<std::string>& list)
{
    std::cout << "Signal append: " << itemid << ", " << status << std::endl;
    ASSERT_STREQ("fooitem", itemid);
    list.push_back(status);
}

int loop_quit (void* data)
{
    auto loop = reinterpret_cast<GMainLoop*>(data);
    g_main_loop_quit(loop);
    return G_SOURCE_REMOVE;
}

TEST_F(DbusInterfaceTests, ItemSignalTests)
{
    core::testing::CrossProcessSync cps1;
    core::testing::CrossProcessSync cps2;

    auto service = [this, &cps1, &cps2]()
    {
        auto test_store = std::make_shared<Item::TestStore>();
        auto pay_service = std::make_shared<DBusInterface>(test_store);

        pay_service->connectionReady.connect([&cps1]()
        {
            cps1.try_signal_ready_for(std::chrono::seconds {2});
        });

        EXPECT_EQ(1u,cps2.wait_for_signal_ready_for(std::chrono::seconds {4}));
        usleep(100);

        std::string appname("foopkg");
        std::string itemname("fooitem");

        auto item = test_store->getItem(appname, itemname);
        EXPECT_NE(nullptr, item);

        auto titem = std::dynamic_pointer_cast<Item::TestItem, Item::Item>(item);

        titem->test_setStatus(Item::Item::VERIFYING, true);
        titem->test_setStatus(Item::Item::PURCHASING, true);
        titem->test_setStatus(Item::Item::NOT_PURCHASED, true);
        titem->test_setStatus(Item::Item::PURCHASED, true);

        /* Let the signals escape before we shut things down */
        usleep(100000);

        /* Force deallocation so we can catch stuff from it */
        test_store.reset();
        pay_service.reset();

        return ::testing::Test::HasFailure() ? core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    auto client = [this, &cps1, &cps2]() -> core::posix::exit::Status
    {
        GMainContext* context = g_main_context_new();
        g_main_context_push_thread_default(context);

        auto trap = core::posix::trap_signals_for_all_subsequent_threads(
        {
            core::posix::Signal::sig_int,
            core::posix::Signal::sig_term
        });

        trap->signal_raised().connect([trap](core::posix::Signal)
        {
            trap->stop();
        });

        /* Wait for the service to setup */
        EXPECT_EQ(1u,cps1.wait_for_signal_ready_for(std::chrono::seconds {2}));

        /* Package proxy and getting status */
        auto package = proxy_pay_package_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                                G_DBUS_PROXY_FLAGS_NONE,
                                                                "com.canonical.pay",
                                                                "/com/canonical/pay/foopkg",
                                                                nullptr, nullptr);
        EXPECT_NE(nullptr, package);

        std::vector<std::string> itemsignals;
        g_signal_connect(G_OBJECT(package), "item-status-changed", G_CALLBACK(signalAppend), &itemsignals);

        cps2.try_signal_ready_for(std::chrono::seconds {2});

        trap->run();

        /* Pull the events through */
        auto loop = g_main_loop_new(context, FALSE);
        auto timeout = g_timeout_source_new(200);
        g_source_set_callback(timeout, loop_quit, loop, nullptr);
        g_source_attach(timeout, context);
        g_main_loop_run(loop);
        g_main_loop_unref(loop);

        /* Can't use assert in lambdas */
        if (4 != itemsignals.size())
        {
            std::cerr << "ERROR: Item signals isn't correct size (4): " << itemsignals.size() << std::endl;
            throw;
        }
        EXPECT_EQ("verifying", itemsignals[0]);
        EXPECT_EQ("purchasing", itemsignals[1]);
        EXPECT_EQ("not purchased", itemsignals[2]);
        EXPECT_EQ("purchased", itemsignals[3]);

        g_clear_object(&package);
        g_clear_pointer(&context, g_main_context_unref);

        return ::testing::Test::HasFailure() ? core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    EXPECT_EQ(core::testing::ForkAndRunResult::empty, core::testing::fork_and_run(client, service));
}


TEST_F(DbusInterfaceTests, encodeDecode)
{
    /* Pass through */
    EXPECT_EQ("fine", DBusInterface::encodePath(std::string("fine")));
    EXPECT_EQ("fine", DBusInterface::decodePath(std::string("fine")));

    /* Number as first characeter */
    EXPECT_EQ("_331337", DBusInterface::encodePath(std::string("31337")));
    EXPECT_EQ("31337", DBusInterface::decodePath(std::string("_331337")));

    /* Underscore test */
    EXPECT_EQ("this_5Fis_5Fc_5Fstyle_5Fnamespacing", DBusInterface::encodePath(std::string("this_is_c_style_namespacing")));
    EXPECT_EQ("this_is_c_style_namespacing", DBusInterface::decodePath(std::string("this_5Fis_5Fc_5Fstyle_5Fnamespacing")));

    /* Hyphen test */
    EXPECT_EQ("typical_2Dapplication", DBusInterface::encodePath(std::string("typical-application")));
    EXPECT_EQ("typical-application", DBusInterface::decodePath(std::string("typical_2Dapplication")));

    /* Japanese test */
    EXPECT_EQ("_E6_97_A5_E6_9C_AC_E8_AA_9E", DBusInterface::encodePath(std::string("日本語")));
    EXPECT_EQ("日本語", DBusInterface::decodePath(std::string("_E6_97_A5_E6_9C_AC_E8_AA_9E")));
}
