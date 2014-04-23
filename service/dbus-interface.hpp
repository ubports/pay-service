
#include <core/dbus/bus.h>
#include "item-interface.hpp"

#ifndef DBUS_INTERFACE_HPP__
#define DBUS_INTERFACE_HPP__ 1

class Applications;

class DBusInterface {

public:
	DBusInterface (core::dbus::Bus::Ptr& in_bus, Item::IStore::Ptr in_items);
	~DBusInterface () { };


private:
	Item::IStore::Ptr items;
	core::dbus::Bus::Ptr bus;
	std::shared_ptr<Applications> base;
};

#endif // DBUS_INTERFACE_HPP__
