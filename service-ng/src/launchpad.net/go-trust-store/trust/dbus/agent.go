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

package dbus

// #cgo pkg-config: trust-store dbus-cpp
// #cgo CXXFLAGS: -std=c++11 -Wall
// #include <stdlib.h>
// #include "agent_shim.h"
import "C"

import (
	"errors"
	"runtime"
	"unsafe"

	"launchpad.net/go-trust-store/trust"
)

// WellKnownBus represents the two busses on which the agent can be run (system
// or session).
type WellKnownBus int

const (
	WellKnownBusSession WellKnownBus = iota
	WellKnownBusSystem
)

// Agent represents an agent that communicates to a remote trust store via dbus.
type Agent struct {
	agent *C.Agent
}

// CreatePerUserAgentForBusConnection creates a trust.Agent implementation
// that communicates with a remote agent living in the same user session.
func CreatePerUserAgentForBusConnection(bus WellKnownBus, serviceName string) (trust.Agent, error) {
	serviceNameCstring := C.CString(serviceName)
	defer C.free(unsafe.Pointer(serviceNameCstring))

	var err *C.char
	cAgent := C.createPerUserAgentForBusConnection(
		wellKnownBusToShim(bus), serviceNameCstring, &err)
	if cAgent == nil {
		defer C.free(unsafe.Pointer(err))
		return nil, errors.New(C.GoString(err))
	}

	agent := &Agent{agent: cAgent}

	// Set the finalizer so we can deal with the user not calling Destroy()
	// manually.
	runtime.SetFinalizer(agent, (*Agent).Destroy)

	return agent, nil
}

// CreateMultiUserAgentForBusConnection creates a trust.Agent implementation
// that communicates with a user-specific remote agent living in user sessions.
func CreateMultiUserAgentForBusConnection(bus WellKnownBus, serviceName string) (trust.Agent, error) {
	serviceNameCstring := C.CString(serviceName)
	defer C.free(unsafe.Pointer(serviceNameCstring))

	var err *C.char
	cAgent := C.createMultiUserAgentForBusConnection(
		wellKnownBusToShim(bus), serviceNameCstring, &err)
	if cAgent == nil {
		defer C.free(unsafe.Pointer(err))
		return nil, errors.New(C.GoString(err))
	}

	agent := &Agent{agent: cAgent}

	// Set the finalizer so we can deal with the user not calling Destroy()
	// manually.
	runtime.SetFinalizer(agent, (*Agent).Destroy)

	return agent, nil
}

// wellKnownBusToShim converts a WellKnownBus to a shim well known bus.
func wellKnownBusToShim(bus WellKnownBus) C.WellKnownBus {
	switch bus {
	case WellKnownBusSystem:
		return C.SYSTEM
	default:
		return C.SESSION
	}
}

// AuthenticateRequestWithParameters authenticates the given request and returns
// the user's answer.
func (agent *Agent) AuthenticateRequestWithParameters(parameters *trust.RequestParameters) trust.Answer {
	return trust.AnswerFromShim(int(C.authenticateRequestWithParameters(
		agent.agent, (*C.RequestParameters)(parameters.ToShim()))))
}

// Destroy destroys the agent. This function isn't necessary to manually call,
// but is idempotent.
func (agent *Agent) Destroy() {
	C.destroyAgent(agent.agent)
	agent.agent = nil
}
