#include "item-memory.hpp"

#include <algorithm>

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

	typedef std::shared_ptr<MemoryItem> Ptr;
private:
	std::string id;
};

std::list<std::string>
MemoryStore::listApplications (void)
{
	std::list<std::string> apps;

	std::transform(data.begin(),
	               data.end(),
	               std::back_inserter(apps),
	               [](const std::pair<std::string, std::shared_ptr<std::map<std::string, IItem::Ptr>>> &pair){return pair.first;});

	return apps;
}

std::shared_ptr<std::map<std::string, IItem::Ptr>>
MemoryStore::getItems (std::string& application)
{
	auto app = data[application];

	if (app == nullptr) {
		app = std::make_shared<std::map<std::string, IItem::Ptr>>();
		data[application] = app;
	}

	return app;
}

IItem::Ptr
MemoryStore::getItem (std::string& application, std::string& itemid)
{
	auto app = getItems(application);
	IItem::Ptr item = (*app)[itemid];

	if (item == nullptr) {
		item = std::shared_ptr<IItem>(new MemoryItem(itemid));
		(*app)[itemid] = item;
	}

	return item;
}

} // namespace Item
