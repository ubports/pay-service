
#include "item-interface.hpp"
#include <memory>
#include <iostream>

namespace Item {

class MemoryStore : public IStore {
	public:
		std::list<std::string> listApplications (void);
		std::list<IItem::Ptr> getItems (std::string& application);
		IItem::Ptr newItem (std::string& application, std::string& itemid);
};

} // namespace Item
