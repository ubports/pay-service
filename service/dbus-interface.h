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

#include <core/dbus/bus.h>
#include <core/dbus/service.h>
#include "item-interface.h"

#ifndef DBUS_INTERFACE_HPP__
#define DBUS_INTERFACE_HPP__ 1

class Applications;

class DBusInterface {

public:
	class IApplications
	{
	public:
		struct GetApplications
		{
			typedef IApplications Interface;
			inline static const std::string & name () {
				static const std::string s {"GetApplications"};
				return s;
			}
			inline static const std::chrono::milliseconds default_timeout() {
				return std::chrono::seconds{1};
			}
		};

	public:
		virtual ~IApplications() = default;
		virtual std::vector<core::dbus::types::ObjectPath> get_applications (void) = 0;
	};


public:
	DBusInterface (core::dbus::Bus::Ptr& in_bus, Item::Store::Ptr in_items);
	~DBusInterface () { };


private:
	Item::Store::Ptr items;
	core::dbus::Bus::Ptr bus;
	std::shared_ptr<Applications> base;
};

namespace core { namespace dbus { namespace traits {
template<> struct Service<DBusInterface::IApplications>
{
	inline static const std::string& interface_name() {
		static const std::string s {"com.canonical.pay"};
		return s;
	}
};
}}} /* namespaces */


#endif // DBUS_INTERFACE_HPP__
