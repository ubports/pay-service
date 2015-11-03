/* Copyright (C) 2015 Canonical Ltd.
 *
 * This file is part of go-ual.
 *
 * go-ual is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 3, as published by the
 * Free Software Foundation.
 *
 * go-ual is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with go-ual. If not, see <http://www.gnu.org/licenses/>.
 */

package ual

import (
	"testing"
	"time"

	"github.com/godbus/dbus"
	"github.com/ziutek/glib"
)

func keepTryingWithTimeout(function func() bool, duration time.Duration) bool {
	timeout := make(chan struct{})
	go func() {
		time.Sleep(duration)
		close(timeout)
	}()

	for !function() {
		select {
		case <-timeout: // Timed out
			return false
		default:
			// Keep looping
		}
	}

	return true
}

// Test that helper stop observers can be added and removed, and are called at
// the right times.
func TestStopObservers(t *testing.T) {
	dbusConnection, err := dbus.SessionBus()
	if err != nil {
		t.Fatalf("Unable to connect to DBus session bus: %s", err)
	}

	called := false
	id, err := ObserverAddHelperStop("foo", func(appId, instanceId, helperType string) {
		called = true
		if appId != "com.bar_bar_44.32" {
			t.Errorf(`appId was "%s", expected "com.bar_bar_44.32"`, appId)
		}

		if instanceId != "1234" {
			t.Errorf(`instanceId was "%s", expected "1234"`, instanceId)
		}

		if helperType != "foo" {
			t.Errorf(`helperType was "%s", expected "foo"`, helperType)
		}
	})
	if err != nil {
		t.Errorf("Unexpected error adding stop observer: %s", err)
	}

	// Spoof an Upstart EventEmitted so UAL thinks "foo" has stopped
	err = dbusConnection.Emit("/com/ubuntu/Upstart",
		"com.ubuntu.Upstart0_6.EventEmitted",
		"stopped", []string{"JOB=untrusted-helper", "INSTANCE=foo:1234:com.bar_bar_44.32"})
	if err != nil {
		t.Error("Unexpected error emitting signal: %s", err)
	}

	// Wait until there are pending events, but with a timeout
	context := glib.DefaultMainContext()
	if !keepTryingWithTimeout(context.Pending, 1*time.Second) {
		t.Fatal("Unexpected timeout: No glib events")
	}

	for context.Pending() {
		context.Iteration(true)
	}

	if !called {
		t.Error("Expected observer to be called")
	}

	err = ObserverDeleteHelperStop(id)
	if err != nil {
		t.Errorf("Unexpected error removing stop observer: %s", err)
	}
}
