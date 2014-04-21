#include "dbus-interface.hpp"

DBusInterface::DBusInterface (std::shared_ptr<Item::DB::Interface> in_items) :
		items(in_items)
{
	return;
}

