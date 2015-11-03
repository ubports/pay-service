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

import (
	"testing"
)

// Test typical CreatePerUserAgentForBusConnection usage.
func TestCreatePerUserAgentForBusConnection(t *testing.T) {
	agent, err := CreatePerUserAgentForBusConnection(WellKnownBusSession, "foo")
	if err != nil {
		t.Fatalf("Unexpected error while creating per-user agent: %s", err)
	}

	if agent == nil {
		t.Fatal("Agent was unexpectedly nil")
	}
}

// Test typical CreateMultiUserAgentForBusConnection usage.
func TestCreateMultiUserAgentForBusConnection(t *testing.T) {
	agent, err := CreateMultiUserAgentForBusConnection(WellKnownBusSession,
		"foo")
	if err != nil {
		t.Fatalf("Unexpected error while creating multi-user agent: %s", err)
	}

	if agent == nil {
		t.Fatal("Agent was unexpectedly nil")
	}
}
