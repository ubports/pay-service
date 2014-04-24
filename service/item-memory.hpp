
#include "item-interface.hpp"
#include <memory>
#include <iostream>
#include <map>

namespace Item {

class MemoryStore : public IStore {
	public:
		std::list<std::string> listApplications (void);
		std::list<IItem::Ptr> getItems (std::string& application);
		IItem::Ptr newItem (std::string& application, std::string& itemid);

	private:
		std::map<std::string, std::shared_ptr<std::map<std::string, IItem::Ptr>>> data;
};

} // namespace Item
