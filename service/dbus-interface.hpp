
#include <core/dbus/bus.h>
#include <core/dbus/service.h>
#include "item-interface.hpp"

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
	DBusInterface (core::dbus::Bus::Ptr& in_bus, Item::IStore::Ptr in_items);
	~DBusInterface () { };


private:
	Item::IStore::Ptr items;
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
