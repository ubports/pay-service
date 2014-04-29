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

#include "item-memory.hpp"

#include <algorithm>
#include <core/signal.h>

namespace Item {

class MemoryItem : public IItem {
public:
	MemoryItem (std::string& in_app, std::string& in_id, Verification::IFactory::Ptr& in_vfactory) :
		app(in_app),
		id(in_id),
		vfactory(in_vfactory),
		vitem(nullptr)
	{
	}

	std::string &getApp (void) {
		return app;
	}

	std::string &getId (void) {
		return id;
	}

	IItem::Status getStatus (void) {
		if (vitem != nullptr)
			return IItem::Status::VERIFYING;

		return IItem::Status::UNKNOWN;
	}

	bool verify (void) {
		if (vitem != nullptr)
			return true;
		if (!vfactory->running())
			return false;

		vitem = std::make_shared<Verification::IItem>(vfactory->verifyItem(app, id));
		if (vitem != nullptr) {
			/* New verification instance, tell the world! */
			statusChanged(IItem::Status::VERIFYING);
			return true;
		} else {
			/* Uhg, failed */
			return false;
		}
	}

	typedef std::shared_ptr<MemoryItem> Ptr;
	core::Signal<IItem::Status> statusChanged;

private:
	std::string id;
	std::string app;
	Verification::IFactory::Ptr vfactory;
	Verification::IItem::Ptr vitem;
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
		auto mitem = new MemoryItem(application, itemid, verificationFactory);
		mitem->statusChanged.connect([this, mitem](IItem::Status status) {
			itemChanged(mitem->getApp(), mitem->getId(), status);
		});

		item = std::shared_ptr<IItem>(mitem);
		(*app)[itemid] = item;
	}

	return item;
}

} // namespace Item
