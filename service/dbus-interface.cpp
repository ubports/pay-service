
#include <core/dbus/announcer.h>
#include <core/dbus/service.h>
#include <core/dbus/skeleton.h>
#include <core/dbus/stub.h>
#include <vector>

#include "dbus-interface.hpp"

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

	Applications (const core::dbus::Bus::Ptr& bus, Item::IStore::Ptr in_items) : 
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
	Item::IStore::Ptr items;
};

DBusInterface::DBusInterface (core::dbus::Bus::Ptr& in_bus, Item::IStore::Ptr in_items) :
		bus(in_bus),
		items(in_items),
		base(core::dbus::announce_service_on_bus<DBusInterface::IApplications, Applications>(in_bus, in_items))
{
	return;
}

