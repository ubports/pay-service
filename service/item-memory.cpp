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
		vitem(nullptr),
		status(IItem::Status::UNKNOWN)
	{
		/* We init into the unknown state and then wait for someone
		   to ask us to do something about it. */
		// vfactory->running() ? printf("Running") : printf("Not running");
	}

	std::string &getApp (void) {
		return app;
	}

	std::string &getId (void) {
		return id;
	}

	IItem::Status getStatus (void) {
		return status;
	}

	bool verify (void) {
		if (vitem != nullptr)
			return true;
		if (!vfactory->running())
			return false;

		vitem = vfactory->verifyItem(app, id);
		if (vitem == nullptr) {
			/* Uhg, failed */
			return false;
		}

		/* New verification instance, tell the world! */
		setStatus(IItem::Status::VERIFYING);

		/* When the verification item has run it's course we need to
		   update our status */
		/* NOTE: This will execute on the verification item's thread */
		vitem->verificationComplete.connect([this](Verification::IItem::Status status) {
			switch (status) {
			case Verification::IItem::PURCHASED:
				setStatus(IItem::Status::PURCHASED);
				break;
			case Verification::IItem::NOT_PURCHASED:
				setStatus(IItem::Status::NOT_PURCHASED);
				break;
			case Verification::IItem::ERROR:
			default: /* Fall through, an error is same as status we don't know */
				setStatus(IItem::Status::UNKNOWN);
				break;
			}

			vitem = nullptr;
		});

		return vitem->run();
	}

	typedef std::shared_ptr<MemoryItem> Ptr;
	core::Signal<IItem::Status> statusChanged;

private:
	void setStatus (IItem::Status in_status) {
		std::unique_lock<std::mutex> ul(status_mutex);
		bool signal = (status != in_status);

		status = in_status;
		ul.unlock();

		if (signal)
			/* NOTE: in_status here as it's on the stack and the status
			   that this signal should be associated with */
			statusChanged(in_status);
	}

	/***** Only set at init *********/
	/* Item ID */
	std::string id;
	/* Application ID */
	std::string app;
	/* Pointer to the factory to use */
	Verification::IFactory::Ptr vfactory;

	/****** std::shared_ptr<> is threadsafe **********/
	/* Verification item if we're in the state of verifying or null otherwise */
	Verification::IItem::Ptr vitem;

	/****** status is protected with it's own mutex *******/
	std::mutex status_mutex;
	IItem::Status status;
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
	if (verificationFactory == nullptr)
		return IItem::Ptr(nullptr);

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
