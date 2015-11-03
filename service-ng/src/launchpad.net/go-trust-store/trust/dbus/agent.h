/* Copyright (C) 2015 Canonical Ltd.
 *
 * This file is part of go-trust-store.
 *
 * go-trust-store is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the Free Software Foundation.
 *
 * go-trust-store is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with go-trust-store. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kyle Fazzari <kyle@canonical.com>
 */

#ifndef GO_TRUST_STORE_DBUS_AGENT_H
#define GO_TRUST_STORE_DBUS_AGENT_H

#include <memory>

namespace core
{
	namespace trust
	{
		class Agent;
	}

	namespace dbus
	{
		class Bus;
	}
}

// A shim DBus Agent. It exists simply to keep the shared_ptr of the bus and
// agent alive and control them from C (thus Go). It also takes care of running
// and stopping the DBus bus so we don't require dbus-cpp bindings.
class Agent
{
	public:
		// Run the DBus bus.
		Agent(const std::shared_ptr<core::dbus::Bus> &bus,
		      const std::shared_ptr<core::trust::Agent> &agent);

		// Stop the DBus bus.
		~Agent();

		std::shared_ptr<core::trust::Agent> agent() const;

	private:
		std::shared_ptr<core::dbus::Bus> m_bus;
		std::shared_ptr<core::trust::Agent> m_agent;

		std::thread m_dbusThread;
};

 #endif // GO_TRUST_STORE_DBUS_AGENT_H
