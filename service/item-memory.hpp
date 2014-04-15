
#include "item-interface.hpp"
#include <memory>
#include <iostream>

namespace Item {
namespace DB {

class Memory : public Interface {
	public:
		std::list<std::string> listApplications (void);
		std::list<std::shared_ptr<Item::Interface>> getItems (std::string& application);
		std::shared_ptr<Item::Interface> newItem (std::string& application, std::string& itemid);
};

} // namespace DB
} // namespace Item
