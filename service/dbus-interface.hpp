
#include <core/dbus/bus.h>
#include "item-interface.hpp"

#ifndef DBUS_INTERFACE_HPP__
#define DBUS_INTERFACE_HPP__ 1

class DBusInterface {

public:
	DBusInterface (core::dbus::Bus::Ptr in_bus, std::shared_ptr<Item::DB::Interface> in_items);
	~DBusInterface () { };


private:
	std::shared_ptr<Item::DB::Interface> items;
	core::dbus::Bus::Ptr bus;

};

#endif // DBUS_INTERFACE_HPP__
