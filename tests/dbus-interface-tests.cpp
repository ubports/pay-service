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

struct DbusInterfaceTests : public ::testing::Test
{
};

TEST_F(DbusInterfaceTests, BasicAllocation)
{
    auto store = std::make_shared<Item::NullStore>();
    DBusInterface* dbus = new DBusInterface(store);

    EXPECT_NE(nullptr, dbus);

    delete dbus;
    return;
}

TEST_F(DbusInterfaceTests, is_reachable_on_the_bus)
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
            cps1.try_signal_ready_for(std::chrono::seconds {1});
        });

        trap->run();

        /* Force deallocation so we can catch stuff from it */
        null_store.reset();
        pay_service.reset();

        return ::testing::Test::HasFailure() ? core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    auto client = [this, &cps1]()
    {
        auto bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
        /*
        GError * error = nullptr;
        auto address = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
        GDBusConnection * bus = g_dbus_connection_new_for_address_sync(
        address,
        static_cast<GDBusConnectionFlags>(G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT | G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION),
        nullptr, nullptr, &error
        );
        g_free(address);

        if (error != nullptr) {
            g_warning("Unable to get bus: %s", error->message);
            g_clear_error(&error);
            return core::posix::exit::Status::failure;
        }
        */

        EXPECT_EQ(1u,cps1.wait_for_signal_ready_for(std::chrono::seconds {1}));

        /* Service function test */
        auto service = proxy_pay_proxy_new_sync(bus,
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

        /* Item proxy and getting status */

        g_clear_object(&service);
        g_clear_object(&bus);

        return ::testing::Test::HasFailure() ? core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    EXPECT_EQ(core::testing::ForkAndRunResult::empty, core::testing::fork_and_run(service, client));
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
