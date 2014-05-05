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

#include "test_data.h"

#include <service/dbus-interface.h>
#include <service/item-null.h>

#include <core/dbus/fixture.h>
#include <core/dbus/object.h>
#include <core/dbus/service.h>
#include <core/dbus/asio/executor.h>

#include <core/posix/signal.h>

#include <core/testing/fork_and_run.h>
#include <core/testing/cross_process_sync.h>

#include <gtest/gtest.h>

namespace dbus = core::dbus;

namespace
{
struct Service : public core::dbus::testing::Fixture
{
};

auto session_bus_config_file =
        core::dbus::testing::Fixture::default_session_bus_config_file() =
        core::testing::session_bus_configuration_file();

auto system_bus_config_file =
        core::dbus::testing::Fixture::default_system_bus_config_file() =
        core::testing::system_bus_configuration_file();
}

/**
 Doesn't work with DBus Fixture

TEST_F(Service, BasicAllocation)
{
	auto bus = session_bus();
	auto store = std::make_shared<Item::NullStore>();
	DBusInterface * dbus = new DBusInterface(bus, store);

	EXPECT_NE(nullptr, dbus);

	delete dbus;
	return;
}
 **/

TEST_F(Service, is_reachable_on_the_bus)
{
        core::testing::CrossProcessSync cps1;

        static const int64_t expected_value = 42;

        auto service = [this, &cps1]()
        {
            auto bus = session_bus();
            bus->install_executor(core::dbus::asio::make_executor(bus));

            auto trap = core::posix::trap_signals_for_all_subsequent_threads(
            {
                core::posix::Signal::sig_int,
                core::posix::Signal::sig_term
            });

            trap->signal_raised().connect([trap, bus](core::posix::Signal)
            {
                trap->stop();
                bus->stop();
            });

            std::thread t{[bus](){ bus->run(); }};

            auto null_store = std::make_shared<Item::NullStore>();
            auto pay_service = std::make_shared<DBusInterface>(bus, null_store);

            cps1.try_signal_ready_for(std::chrono::milliseconds{500});

            trap->run();

            if (t.joinable())
                t.join();

            return ::testing::Test::HasFailure() ? core::posix::exit::Status::failure : core::posix::exit::Status::success;
        };

        auto client = [this, &cps1]()
        {
            auto bus = session_bus();
            bus->install_executor(core::dbus::asio::make_executor(bus));
            std::thread t{[bus](){ bus->run(); }};

            EXPECT_EQ(1u,cps1.wait_for_signal_ready_for(std::chrono::milliseconds{500}));

            auto service = dbus::Service::use_service(bus, "com.canonical.pay");
            auto object = service->object_for_path(dbus::types::ObjectPath{"/com/canonical/pay"});

			auto reply = object->invoke_method_synchronously<DBusInterface::IApplications::GetApplications, std::vector<dbus::types::ObjectPath>>();
			EXPECT_FALSE(reply.is_error());

			auto retval = reply.value();
			EXPECT_EQ(0, retval.size());

            bus->stop();

            if (t.joinable())
                t.join();

            return ::testing::Test::HasFailure() ? core::posix::exit::Status::failure : core::posix::exit::Status::success;
        };

        EXPECT_EQ(core::testing::ForkAndRunResult::empty, core::testing::fork_and_run(service, client));
}
