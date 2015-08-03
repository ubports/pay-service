/* -*- mode: go; tab-width: 4; indent-tabs-mode: nil -*- */ 
/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

package service

import (
    "fmt"
    "github.com/godbus/dbus"
    "testing"
)

// Test typical Connect and Names usage.
func TestNames(t *testing.T) {
    server := new(DbusServer)
    err := server.Connect()
    if err != nil {
        t.Errorf("Unexpected error while connecting: %s", err)
    }

    // Verify that we have a name on the bus
    names := server.Names()
    if len(names) == 0 {
        t.Error("Got 0 names, expected at least 1")
    }
}

// Test lack of names before the server is connected.
func TestNames_beforeConnect(t *testing.T) {
    server := new(DbusServer)
    names := server.Names()
    if names != nil {
        t.Error("Expected no names to be returned")
    }
}

// Test typical RequestName usage.
func TestRequestName(t *testing.T) {
    server := new(DbusServer)
    err := server.Connect()
    if err != nil {
        t.Errorf("Unexpected error while connecting: %s", err)
    }

    // Obtain our own unique name
    names := server.Names()
    if len(names) < 1 {
        // Exit here so we don't index out of bounds
        t.Fatalf("Got %d names, expected at least 1", len(names))
    }

    reply, err := server.RequestName("com.example.DbusTest", dbus.NameFlagDoNotQueue)
    if err != nil {
        t.Errorf("Unable to request name: %s", err)
    }

    if reply != dbus.RequestNameReplyPrimaryOwner {
        t.Error("Reply implies that name was unexpectedly already taken")
    }

    // Make sure we own the name we expect
    owner, err := server.GetNameOwner("com.example.DbusTest")
    if err != nil {
        t.Error("Unexpected error while requesting name owner")
    }

    // Make sure we own that name
    if owner != names[0] {
        t.Errorf(`Name owner was "%s", expected the owner to be us ("%s")`, owner, names[0])
    }
}

// Test that a name request before the server is connected results in an error.
func TestRequestName_beforeConnect(t *testing.T) {
    server := new(DbusServer)
    _, err := server.RequestName("foo", dbus.NameFlagDoNotQueue)
    if err == nil {
        t.Error("Expected an error due to name request before server was connected")
    }
}

// Test that a name owner request before the server is connected results in an
// error.
func TestGetNameOwner_beforeConnect(t *testing.T) {
    server := new(DbusServer)
    _, err := server.GetNameOwner("foo")
    if err == nil {
        t.Error("Expected an error due to name owner request before server was connected")
    }
}

// Test typical Export usage.
func TestExport(t *testing.T) {
    server := new(DbusServer)

    err := server.Connect()
    if err != nil {
        t.Errorf("Unexpected error while connecting: %s", err)
    }

    var object struct{ Foo string }
    err = server.Export(object, "/foo/bar", "baz")
    if err != nil {
        t.Errorf("Unexpected error while exporting: %s", err)
    }
}

// Test that an export before the server is connected results in an error.
func TestExport_beforeConnect(t *testing.T) {
    server := new(DbusServer)

    var object struct{ Foo string }
    err := server.Export(object, "foo", "bar")
    if err == nil {
        t.Error("Expected an error due to export before server was connected")
    }
}

// Test typical Emit usage.
func TestEmit(t *testing.T) {
    server := new(DbusServer)
    err := server.Connect()
    if err != nil {
        t.Errorf("Unexpected error while connecting: %s", err)
    }

    names := server.Names()
    if len(names) == 0 {
        t.Fatal("Got 0 names, expected at least 1")
    }

    match := fmt.Sprintf("type='signal',sender='%s'", names[0])

    server.connection.BusObject().Call("org.freedesktop.DBus.AddMatch", 0, match)

    signals := make(chan *dbus.Signal, 10)
    server.connection.Signal(signals)

    done := make(chan struct{})

    go func() {
        signal := <-signals // Wait for the signal

        if signal.Path != "/foo/bar/baz" {
            t.Error(`Path was "%s", expected "/foo/bar/baz"`, signal.Path)
        }

        close(done)
    }()

    err = server.Emit("/foo/bar/baz", "com.example.ExampleInterface.Signal", "qux")

    <-done
}

// Test that an emit before the server is connected results in an error.
func TestEmit_beforeConnect(t *testing.T) {
    server := new(DbusServer)

    err := server.Emit("foo", "bar", "baz")
    if err == nil {
        t.Error("Expected an error due to emit before server was connected")
    }
}

// Test that Stop stops.
/*
func TestStop(t *testing.T) {
    server := new(DbusServer)

    err := server.Connect()
    if err != nil {
        t.Errorf("Unexpected error while connecting: %s", err)
    }

    err := server.Stop()
    if err != nil {
        t.Errorf("Error wile disconnecting: %s", err)
    }

    names := server.Names()
    if names != nil {
        t.Errorf("Expected no names to be returned.")
    }
}
*/
