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

#include <string>
#include "item-interface.h"

namespace Item {

class NullItem : public Item {
		std::string _id;

	public:
		NullItem (std::string id) : _id(id) {};

		std::string &getId (void) {
			return _id;
		}

		Item::Status getStatus (void) {
			return Item::Status::UNKNOWN;
		}
};

class NullStore : public Store {
	public:
		std::list<std::string> listApplications (void) {
			return std::list<std::string>();
		}

		std::shared_ptr<std::map<std::string, Item::Ptr>> getItems (std::string& application) {
			return std::make_shared<std::map<std::string, Item::Ptr>>();
		}

		Item::Ptr getItem (std::string& application, std::string& itemid) {
			Item::Ptr retval(new NullItem(itemid));
			return retval;
		}
};

}; // ns Item
