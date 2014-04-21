
#include "dbus-interface.hpp"
#include "item-null.hpp"

int
main (int argv, char * argc[])
{
	std::shared_ptr<Item::DB::Null> items(new Item::DB::Null);
	DBusInterface * dbus(new DBusInterface(items));


	return 0;
}
