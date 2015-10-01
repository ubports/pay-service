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

#include <memory>
#include <core/trust/agent.h>
#include <core/trust/dbus_agent.h>
#include <core/dbus/dbus.h>

#include "agent_shim.h"
#include "agent.h"

namespace
{
	core::dbus::Bus::Ptr createDbusConnection(WellKnownBus bus)
	{
		switch(bus)
		{
			case SYSTEM:
				return core::dbus::Bus::Ptr(new core::dbus::Bus(core::dbus::WellKnownBus::system));
			default:
				return core::dbus::Bus::Ptr(new core::dbus::Bus(core::dbus::WellKnownBus::session));
		}
	}

	void fromShimRequestParameters(const RequestParameters &from,
		core::trust::Agent::RequestParameters &to)
	{
		to.application.uid = core::trust::Uid(from.application.uid);
		to.application.pid = core::trust::Pid(from.application.pid);
		to.application.id = std::string(from.application.id);

		to.feature = core::trust::Feature(from.feature);
		to.description = std::string(from.description);
	}
}

Agent *createPerUserAgentForBusConnection(WellKnownBus bus,
                                          const char *const serviceName,
                                          char **error)
{
	try
	{
		auto busConnection = createDbusConnection(bus);

		return new Agent(
			busConnection,
			core::trust::dbus::create_per_user_agent_for_bus_connection(
				busConnection, std::string(serviceName)));
	}
	catch(const std::exception &exception)
	{
		std::string message(exception.what());
		*error = static_cast<char*>(malloc(message.size()));
		message.copy(*error, message.size());
		return NULL;
	}
}

Agent *createMultiUserAgentForBusConnection(WellKnownBus bus,
                                            const char *const serviceName,
                                            char **error)
{
	try
	{
		auto busConnection = createDbusConnection(bus);

		return new Agent(
			busConnection,
			core::trust::dbus::create_multi_user_agent_for_bus_connection(
				busConnection, std::string(serviceName)));
	}
	catch(const std::exception &exception)
	{
		std::string message(exception.what());
		*error = static_cast<char*>(malloc(message.size()));
		message.copy(*error, message.size());
		return NULL;
	}
}

void destroyAgent(Agent *agent)
{
	delete agent;
}

Answer authenticateRequestWithParameters(Agent *agent, const RequestParameters * const parameters)
{
	core::trust::Agent::RequestParameters agentParameters;

	fromShimRequestParameters(*parameters, agentParameters);

	switch (agent->agent()->authenticate_request_with_parameters(agentParameters))
	{
		case core::trust::Request::Answer::granted:
			return GRANTED;
		default:
			return DENIED;
	}
}
