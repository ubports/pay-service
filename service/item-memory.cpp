/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Ted Gould <ted.gould@canonical.com>
 */

#include "item-memory.h"

#include <algorithm>

namespace Item {

class MemoryItem : public Item {
public:
	MemoryItem (std::string& in_id) :
		id(in_id)
	{
	}

	std::string &getId (void) {
		return id;
	}

	Item::Status getStatus (void) {
		return Item::Status::UNKNOWN;
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
	               [](const std::pair<std::string, std::shared_ptr<std::map<std::string, Item::Ptr>>> &pair){return pair.first;});

	return apps;
}

std::shared_ptr<std::map<std::string, Item::Ptr>>
MemoryStore::getItems (std::string& application)
{
	auto app = data[application];

	if (app == nullptr) {
		app = std::make_shared<std::map<std::string, Item::Ptr>>();
		data[application] = app;
	}

	return app;
}

Item::Ptr
MemoryStore::getItem (std::string& application, std::string& itemid)
{
	auto app = getItems(application);
	Item::Ptr item = (*app)[itemid];

	if (item == nullptr) {
		item = std::shared_ptr<Item>(new MemoryItem(itemid));
		(*app)[itemid] = item;
	}

	return item;
}

} // namespace Item
