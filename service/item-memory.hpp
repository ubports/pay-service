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

#include "item-interface.hpp"
#include "verification-factory.hpp"
#include <memory>
#include <iostream>
#include <map>

namespace Item {

class MemoryStore : public IStore {
	public:
		MemoryStore (const Verification::IFactory::Ptr& factory) :
			verificationFactory(factory)
			{}
		std::list<std::string> listApplications (void);
		std::shared_ptr<std::map<std::string, IItem::Ptr>> getItems (std::string& application);
		IItem::Ptr getItem (std::string& application, std::string& itemid);

	private:
		std::map<std::string, std::shared_ptr<std::map<std::string, IItem::Ptr>>> data;
		Verification::IFactory::Ptr verificationFactory;
};

} // namespace Item
