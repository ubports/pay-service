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

#include "verification-curl.hpp"

#include <curl/curl.h>
#include <curl/easy.h>

namespace Verification {

class CurlItem : public IItem {
public:
	CurlItem (void) {
	}

	~CurlItem (void) {
	}

	virtual bool run (void) {
		return false;
	}
};


bool
CurlFactory::running ()
{
	return false;
}

IItem::Ptr
CurlFactory::verifyItem (std::string& appid, std::string& itemid)
{
	CurlItem * item = new CurlItem();
	return IItem::Ptr(item);
}

} // ns Verification
