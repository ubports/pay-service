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

#include <core/dbus/announcer.h>
#include <core/dbus/service.h>
#include <core/dbus/skeleton.h>
#include <core/dbus/stub.h>
#include <vector>

#include "dbus-interface.h"

class ApplicationsSkeleton : public core::dbus::Skeleton<DBusInterface::IApplications>
{
public:
	ApplicationsSkeleton(const core::dbus::Bus::Ptr& bus) : core::dbus::Skeleton<DBusInterface::IApplications>(bus),
		object(access_service()->add_object_for_path(core::dbus::types::ObjectPath("/com/canonical/pay")))
	{
		object->install_method_handler<DBusInterface::IApplications::GetApplications>(std::bind(&ApplicationsSkeleton::handle_get_applications, this, std::placeholders::_1));
	}

private:
	void handle_get_applications (const core::dbus::Message::Ptr& msg)
	{
		auto out = get_applications();
		auto reply = core::dbus::Message::make_method_return(msg);
		reply->writer() << out;

		access_bus()->send(reply);
	}

	core::dbus::Object::Ptr object;
};

class Applications : public ApplicationsSkeleton
{
public:
	typedef std::shared_ptr<Applications> Ptr;

	Applications (const core::dbus::Bus::Ptr& bus, Item::Store::Ptr in_items) : 
		ApplicationsSkeleton(bus),
		items(in_items) { }

	std::vector<core::dbus::types::ObjectPath> get_applications () {
		std::vector<core::dbus::types::ObjectPath> retval;
		auto applist = items->listApplications();

		for (auto app : applist) {
			/* TODO: encode into valid path charset */
			core::dbus::types::ObjectPath op("/com/canonical/pay/application/" + app);
			retval.push_back(op);
		}

		return retval;
	}

private:
	Item::Store::Ptr items;
};

DBusInterface::DBusInterface (core::dbus::Bus::Ptr& in_bus, Item::Store::Ptr in_items) :
		bus(in_bus),
		items(in_items),
		base(core::dbus::announce_service_on_bus<DBusInterface::IApplications, Applications>(in_bus, in_items))
{
	items->itemChanged.connect([this](std::string& app, std::string& item, Item::Item::Status status) {
		std::string app_encode = encodePath(app);
		std::string item_encode = encodePath(item);

		auto signal = core::dbus::Message::make_signal("/com/canonical/pay/application/" + app_encode + "/" + item_encode,
		                                               "com.canonical.pay.item",
		                                               "statusChanged");

		/* TODO: Get status string from the obj */
		std::string strstatus("status");
		signal->writer() << strstatus;
		bus->send(signal);
	});

	return;
}

std::string
DBusInterface::encodePath (const std::string& input)
{
	std::string output;

	std::transform(input.cbegin(),
	               input.cend(),
	               std::back_inserter(output),
	               [](const char c){
	                   std::string retval;

	                   if ((c >= 'a' && c <= 'z') ||
		                   (c >= 'A' && c <= 'Z') ||
		                   (c >= '0' && c <= '9')) {
		                   retval = std::string(&c, 1);
	                   } else {
		                   char buffer[4] = {0};
		                   std::snprintf(buffer, 3, "_%2X", c);
		                   retval = std::string(buffer);
	                   }

	                   std::back_insert_iterator<std::string> iterator(retval);
	                   return iterator;
	               });

	return output;
}

std::string
DBusInterface::decodePath (const std::string& input)
{
	std::string output;

	try {
		for (int i = 0; i < input.size(); i++) {
			if (input[i] == '_') {
				char buffer[3] = {0};
				buffer[0] = input[i + 1];
				buffer[1] = input[i + 2];

				unsigned char value = std::stoi(buffer, nullptr, 16);
				output += value;
				i += 2;
			} else {
				output += input[i];
			}
		}
	} catch (...) {
		/* We can get out of bounds on the parsing if the
		   string is invalid. Just return what we have. */
	}

	return output;
}
