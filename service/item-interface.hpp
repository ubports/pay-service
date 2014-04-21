
#include <list>
#include <string>
#include <memory>

#ifndef ITEM_INTERFACE_HPP__
#define ITEM_INTERFACE_HPP__ 1

namespace Item {

namespace Item {
class Interface {
	public:
		enum Status {
			UNKNOWN
		};

		virtual std::string& getId (void) = 0;
		virtual Status getStatus (void) = 0;
}; }

namespace DB {
class Interface {
	public:
		virtual std::list<std::string> listApplications (void) = 0;
		virtual std::list<std::shared_ptr<Item::Interface>> getItems (std::string& application) = 0;
};}

} // namespace Item

#endif // ITEM_INTERFACE_HPP__

