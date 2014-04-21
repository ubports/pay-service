
#include "item-interface.hpp"

#ifndef DBUS_INTERFACE_HPP__
#define DBUS_INTERFACE_HPP__ 1

class DBusInterface {

public:
	DBusInterface (std::shared_ptr<Item::DB::Interface> in_items);
	~DBusInterface () { };


private:
	std::shared_ptr<Item::DB::Interface> items;

};

#endif // DBUS_INTERFACE_HPP__
