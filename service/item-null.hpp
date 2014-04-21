#include <string>
#include "item-interface.hpp"

namespace Item {

namespace Item {
class Null : public Item::Interface {
		std::string _id;

	public:
		Null (std::string id) : _id(id) {};

		std::string &getId (void) {
			return _id;
		}

		Interface::Status getStatus (void) {
			return Interface::Status::UNKNOWN;
		}
};
}; // ns Item

namespace DB {
class Null : public Interface {
	public:
		std::list<std::string> listApplications (void) {
			return std::list<std::string>();
		}

		std::list<std::shared_ptr<Item::Interface>> getItems (std::string& application) {
			return std::list<std::shared_ptr<Item::Interface>>();
		}

		std::shared_ptr<Item::Interface> newItem (std::string application, std::string itemid) {
			std::shared_ptr<Item::Interface> retval(new Item::Null(itemid));
			return retval;
		}
};
}; // ns DB

}; // ns Item
