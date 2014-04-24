#include "item-memory.hpp"

namespace Item {

class MemoryItem : public IItem {
public:
	MemoryItem (std::string& in_id) :
		id(in_id)
	{
	}

	std::string &getId (void) {
		return id;
	}

	IItem::Status getStatus (void) {
		return IItem::Status::UNKNOWN;
	}
private:
	std::string id;

};

std::list<std::string>
MemoryStore::listApplications (void)
{
	std::list<std::string> retval;
	return retval;
}

std::list<IItem::Ptr>
MemoryStore::getItems (std::string& application)
{
	std::list<IItem::Ptr> retval;
	return retval;
}

IItem::Ptr
MemoryStore::newItem (std::string& application, std::string& itemid)
{
	auto app = data[application];

	if (app == nullptr) {
		app = std::shared_ptr<std::map<std::string, IItem::Ptr>>(new std::map<std::string, IItem::Ptr>());
		data[application] = app;
	}

	MemoryItem::Ptr newitem(MemoryItem::Ptr(new MemoryItem(itemid)));
	(*app)[itemid] = newitem;

	return newitem;
}

} // namespace Item
