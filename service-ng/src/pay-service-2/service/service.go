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
    "github.com/godbus/dbus/introspect"
)

const (
    // busName is the name to be requested from the DBus session bus.
    busName = "com.canonical.payments"

    // baseObjectPath is the base object path 
    baseObjectPath = "/com/canonical/pay/store"

    // interfaceName is the name of the interface being implemented here.
    interfaceName = "com.canonical.pay.store"

    // introspectionXml is the XML to be used for the Introspection interface.
    introspectionXml = `
        <node>
            <interface name="` + interfaceName + `">
            </interface>` +
        introspect.IntrospectDataString +
        `</node>`
)

// Service represents the actual service daemon.
type Service struct {
    server         DbusWrapper
}

/*
 * New creates a new Service object.
 *
 * Returns:
 * - New daemon
 * - Error (nil if none)
 */
func New() (*Service, error) {
    service := new(Service)

    service.server = new(DbusServer)

    return service, nil
}

/*
 * Run connects to the DBus session bus and prepares for receiving requests.
 *
 * Returns:
 * - Error (nil if none)
 */
func (service *Service) Run() error {
    err := service.server.Connect()
    if err != nil {
        return fmt.Errorf("Unable to connect: %s", err)
    }

    err = service.server.Export(
        introspect.Introspectable(introspectionXml), "/",
        "org.freedesktop.DBus.Introspectable")
    if err != nil {
        return fmt.Errorf("Unable to export introspection: %s", err)
    }

    // Now that all interfaces are exported and ready, request our name. Things
    // are done in this order so that our interfaces aren't called before
    // they're exported.
    reply, err := service.server.RequestName(busName, dbus.NameFlagDoNotQueue)
    if err != nil {
        return fmt.Errorf(`Unable to get requested name "%s": %s`, busName, err)
    }

    if reply != dbus.RequestNameReplyPrimaryOwner {
        return fmt.Errorf(`Requested name "%s" was already taken`, busName)
    }

    return nil
}
