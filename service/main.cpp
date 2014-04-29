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

#include <core/dbus/bus.h>
#include <core/dbus/asio/executor.h>

#include <core/posix/signal.h>

#include "dbus-interface.hpp"
#include "item-null.hpp"

namespace dbus = core::dbus;

namespace
{
dbus::Bus::Ptr the_session_bus()
{
	static dbus::Bus::Ptr session_bus = std::make_shared<dbus::Bus>(dbus::WellKnownBus::session);
	return session_bus;
}
}

int
main (int argv, char * argc[])
{
    auto trap = core::posix::trap_signals_for_all_subsequent_threads(
    {
        core::posix::Signal::sig_int,
        core::posix::Signal::sig_term
    });

    trap->signal_raised().connect([trap](core::posix::Signal) { trap->stop(); });

	auto bus = the_session_bus();
	bus->install_executor(core::dbus::asio::make_executor(bus));
	std::thread t {std::bind(&dbus::Bus::run, bus)};

	std::shared_ptr<Item::NullStore> items(new Item::NullStore);
    std::shared_ptr<DBusInterface> dbus(new DBusInterface(bus, items));

    trap->run();
	bus->stop();

	if (t.joinable())
		t.join();

	return EXIT_SUCCESS;
}
