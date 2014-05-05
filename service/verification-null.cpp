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

#include "verification-null.h"

namespace Verification {

class NullItem : public Item {
public:
	NullItem (void) {
	}

	~NullItem (void) {
	}

	virtual bool run (void) {
		return false;
	}
};


bool
NullFactory::running ()
{
	return false;
}

Item::Ptr
NullFactory::verifyItem (std::string& appid, std::string& itemid)
{
	NullItem * item = new NullItem();
	return Item::Ptr(item);
}

} // ns Verification
