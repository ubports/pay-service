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
Some services on the Ubuntu Phone deal with resources and functionality that can
be abused. As a result, the trust model requires that any access to them be
specifically requested and granted by the user. The resulting user interaction
is implemented by a "trust prompt." If the user explicitly grants the trust
request via the prompt, the service grants access to the requested resource. Any
service executing this trust prompting is called a "trusted helper." The trust
model also makes the assumption that a given answer for a trust request remains
valid until otherwise told. This information is stored using the "trust store."
For more technical information regarding the trust store and sessions, see
https://wiki.ubuntu.com/Security/TrustStoreAndSessions.

This package serves as Go bindings to the trust-store
(https://launchpad.net/trust-store), allowing Go programs to be trusted helpers.
Note that only the DBus agent is currently implemented in the bindings. For
examples and other DBus-agent-specific information, see the dbus package.
*/
package trust
