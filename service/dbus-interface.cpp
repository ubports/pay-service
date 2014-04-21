#include "dbus-interface.hpp"

DBusInterface::DBusInterface (core::dbus::Bus::Ptr in_bus, std::shared_ptr<Item::DB::Interface> in_items) :
		bus(in_bus),
		items(in_items)
{
	return;
}

