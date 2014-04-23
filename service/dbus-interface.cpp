
#include <core/dbus/announcer.h>
#include <core/dbus/service.h>
#include <core/dbus/skeleton.h>
#include <core/dbus/stub.h>
#include <vector>

#include "dbus-interface.hpp"

class IApplications
{
protected:
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

namespace core { namespace dbus { namespace traits {
template<> struct Service<IApplications>
{
	inline static const std::string& interface_name() {
		static const std::string s {"com.canonical.pay"};
		return s;
	}
};
}}} /* namespaces */

class ApplicationsSkeleton : public core::dbus::Skeleton<IApplications>
{
public:
	ApplicationsSkeleton(const core::dbus::Bus::Ptr& bus) : core::dbus::Skeleton<IApplications>(bus),
		object(access_service()->add_object_for_path(core::dbus::types::ObjectPath("/com/canonical/pay")))
	{
		object->install_method_handler<IApplications::GetApplications>(std::bind(&ApplicationsSkeleton::handle_get_applications, this, std::placeholders::_1));
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

	Applications (const core::dbus::Bus::Ptr& bus, Item::DB::Interface::Ptr in_items) : 
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
	Item::DB::Interface::Ptr items;
};

DBusInterface::DBusInterface (core::dbus::Bus::Ptr& in_bus, std::shared_ptr<Item::DB::Interface> in_items) :
		bus(in_bus),
		items(in_items),
		base(core::dbus::announce_service_on_bus<IApplications, Applications>(in_bus, in_items))
{
	return;
}

