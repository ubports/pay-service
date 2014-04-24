
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
