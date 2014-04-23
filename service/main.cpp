
#include <core/dbus/bus.h>
#include <core/dbus/asio/executor.h>
#include <sys/types.h>
#include <signal.h>

#include <unistd.h>

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
	auto bus = the_session_bus();
	bus->install_executor(core::dbus::asio::make_executor(bus));
	std::thread t {std::bind(&dbus::Bus::run, bus)};

	std::shared_ptr<Item::DB::Null> items(new Item::DB::Null);
	DBusInterface * dbus(new DBusInterface(bus, items));

	/* If we get an INT or TERM we're just gonna shut this whole thing down! */
	sigset_t signal_set;
	sigemptyset(&signal_set);
	sigaddset(&signal_set, SIGINT);
	sigaddset(&signal_set, SIGTERM);
	int signal;
	sigwait(&signal_set, &signal);

	delete dbus;
	bus->stop();

	if (t.joinable())
		t.join();

	return EXIT_SUCCESS;
}
