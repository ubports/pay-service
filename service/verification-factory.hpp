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

namespace Verification {

class IItem {
public:
	virtual ~IItem() = default;

	typedef std::shared_ptr<IItem> Ptr;
};

class IFactory {
public:
	virtual ~IFactory() = default;

	virtual bool running () = 0;
	virtual IItem& verifyItem (std::string& appid, std::string& itemid) = 0;

	typedef std::shared_ptr<IFactory> Ptr;
};

} // ns Verification
