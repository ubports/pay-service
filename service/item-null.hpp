#include <string>
#include "item-interface.hpp"

namespace Item {

class NullItem : public IItem {
		std::string _id;

	public:
		NullItem (std::string id) : _id(id) {};

		std::string &getId (void) {
			return _id;
		}

		IItem::Status getStatus (void) {
			return IItem::Status::UNKNOWN;
		}
};

class NullStore : public IStore {
	public:
		std::list<std::string> listApplications (void) {
			return std::list<std::string>();
		}

		std::list<IItem::Ptr> getItems (std::string& application) {
			return std::list<IItem::Ptr>();
		}

		IItem::Ptr newItem (std::string application, std::string itemid) {
			IItem::Ptr retval(new NullItem(itemid));
			return retval;
		}
};

}; // ns Item
