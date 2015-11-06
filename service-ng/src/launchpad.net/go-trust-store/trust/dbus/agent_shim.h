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

#ifndef GO_TRUST_STORE_DBUS_AGENT_SHIM_H
#define GO_TRUST_STORE_DBUS_AGENT_SHIM_H

#include "../request_shim.h"
#include "../request_parameters_shim.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Agent Agent;
typedef struct Bus Bus;

typedef enum
{
	SESSION,
	SYSTEM
} WellKnownBus;

// Create an Agent implementation that communicates with a remote agent living
// in the same user session. This makes use of forward declaration to mask the
// fact that the Agent is actually a C++ class.
Agent *createPerUserAgentForBusConnection(WellKnownBus bus,
                                          const char *const serviceName,
                                          char **error);

// Create an Agent implementation that communicates with a user-specific remote
// agent living in user sessions. This makes use of forward declaration to mask
// the fact that the Agent is actually a C++ class.
Agent *createMultiUserAgentForBusConnection(WellKnownBus bus,
                                            const char *const serviceName,
                                            char **error);

// Destroy the agent and free all resources.
void destroyAgent(Agent *agent);

// Authenticate the given request and return the user's answer.
Answer authenticateRequestWithParameters(
	Agent *agent, const RequestParameters *const parameters);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // GO_TRUST_STORE_DBUS_AGENT_SHIM_H
