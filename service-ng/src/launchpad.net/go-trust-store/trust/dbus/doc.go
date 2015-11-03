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

/*
Bindings for the trust store DBus agent.

When initially designing a trusted helper, the author must decide whether to
host the trust store in-process or out-of-process. This package implements the
bindings for the trust store DBus agent, which is meant to communicate with a
remote (out-of-process) trust store-- specifically, trust-stored-skeleton (from
the trust-store-bin package).

For example, say the trust-stored-skeleton was run with the following
incantation:

	$ trust-stored-skeleton --remote-agent DBusRemoteAgent --bus=session \
	  --local-agent MirAgent \
	  --trusted-mir-socket=/var/run/user/$(id -u)/mir_socket_trusted \
	  --for-service FooBar --with-text-domain foo-bar --store-bus session

One could communicate with that instance and cause a trust store prompt like the following:

	package main

	import (
		"log"
		"launchpad.net/go-trust-store/trust"
		"launchpad.net/go-trust-store/trust/dbus"
	)

	func main() {
		// We need the PID and full app ID of a running app here. This is the
		// app that is requesting permission to something in our example, and
		// it's the app over which the trust prompt will be shown.
		appPid := 1234
		appId := "com.ubuntu.foo_bzr_1.2.3"

		trustAgent, err := dbus.CreateMultiUserAgentForBusConnection(
			dbus.WellKnownBusSession, "FooBar")
		if err != nil {
			log.Fatalf("Unable to create trust store agent: %s", err)
		}

		params := &trust.RequestParameters{
			Application: trust.Application{
				Uid: 1000, // This needs to be the current user ID
				Pid: appPid,
				Id: appId,
			},

			// The unique ID of the feature to which access is requested
			Feature: trust.Feature(0),

			// This is the string that will be shown on the trust prompt. Note
			// that this MUST have %1% in it, which trust-stored-skeleton will
			// replace with the app name. If the string doesn't have it, you'll
			// just get cryptic errors.
			Description: "%1% wants to access my feature",
		}

		answer := trustAgent.AuthenticateRequestWithParameters(params)
		log.Println("Answer:", answer)
	}

If, when running the skeleton, it immediately dies with something like "the name
core.trust.dbus.Agent.FooBar was not provided by any .service files," then you
hit a skeleton bug that requires the trust agent to be running before it will
run. In that case, add a time.Sleep() (or a prompt) to the above code after the
agent creation and before the authentication request.
*/
package dbus
