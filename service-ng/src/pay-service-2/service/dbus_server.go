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
)

// DbusServer satisfies the DbusWrapper interface for DBus communication within
// the daemon.
type DbusServer struct {
    connection *dbus.Conn // Connection to the dbus bus
}

// Connect simply initializes a connection to the DBus session bus
//
// Returns:
// - Error (nil if none)
func (server *DbusServer) Connect() error {
    var err error
    server.connection, err = dbus.SessionBus()
    return err
}

func (server *DbusServer) Stop() error {
    return server.connection.Close()
}

// Names returns a list of names owned by the DBus connection.
//
// Returns:
// - Slice of names
func (server *DbusServer) Names() []string {
    if server.connection == nil {
        return nil
    }

    return server.connection.Names()
}

// RequestName requests a specific name for this connection. This can be called
// multiple times.
//
// Parameters:
// name: Name to request.
// flags: Flags to use for request.
//
// Returns:
// - dbus.RequestNameReply to inform caller of request result
// - Error (nil if none)
func (server *DbusServer) RequestName(name string, flags dbus.RequestNameFlags) (dbus.RequestNameReply, error) {
    if server.connection == nil {
        return 0, fmt.Errorf("Server is not connected")
    }

    return server.connection.RequestName(name, flags)
}

// GetNameOwner requests the unique name on the bus that owns a specific name.
//
// Parameters:
// name: Name for which to query.
//
// Returns:
// - Unique name of the owner connection (if any).
// - Error (nil if none)
func (server *DbusServer) GetNameOwner(name string) (string, error) {
    var owner string
    if server.connection == nil {
        return owner, fmt.Errorf("Server is not connected")
    }

    object := server.connection.BusObject()
    err := object.Call("org.freedesktop.DBus.GetNameOwner", 0, name).Store(&owner)
    return owner, err
}

// Export exports a given interface to handle incoming requests.
//
// Parameters:
// object: Interface to export.
// path: Object path on which to export the interface.
// iface: Name of the DBus interface being satisfied by `object`.
//
// Returns:
// - Error (nil if none)
func (server *DbusServer) Export(object interface{}, path dbus.ObjectPath, iface string) error {
    if server.connection == nil {
        return fmt.Errorf("Server is not connected")
    }

    return server.connection.Export(object, path, iface)
}

func (server *DbusServer) ExportSubtree(object interface{},
    path dbus.ObjectPath, iface string) error {
    if server.connection == nil {
        return fmt.Errorf("Server is not connected")
    }

    return server.connection.ExportSubtree(object, path, iface)
}

// Emit emits a DBus signal.
//
// Parameters:
// path: Object path on which to emit the signal.
// name: Name of the signal.
// values...: Signal parameters.
//
// Returns:
// - Error (nil if none)
func (server *DbusServer) Emit(path dbus.ObjectPath, name string, values ...interface{}) error {
    if server.connection == nil {
        return fmt.Errorf("Server is not connected")
    }

    return server.connection.Emit(path, name, values...)
}
