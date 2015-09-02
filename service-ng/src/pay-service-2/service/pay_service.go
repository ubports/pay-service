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
    "path"
)

type PayService struct {
    dbusConnection DbusWrapper
    baseObjectPath dbus.ObjectPath
    shutdownTimer  Timer
}

func NewPayService(dbusConnection DbusWrapper,
    interfaceName string, baseObjectPath dbus.ObjectPath,
    shutdownTimer Timer) (*PayService, error) {
    payiface := &PayService{
        dbusConnection: dbusConnection,
        shutdownTimer: shutdownTimer,
    }

    if !baseObjectPath.IsValid() {
        return nil, fmt.Errorf(`Invalid base object path: "%s"`, baseObjectPath)
    }

    payiface.baseObjectPath = baseObjectPath

    return payiface, nil
}

func (iface *PayService) AcknowledgeItem(message dbus.Message, item_id string) (map[string]dbus.Variant, *dbus.Error) {
    iface.pause_timer()
    package_id := package_id_from_path(message)

    fmt.Println("DEBUG - AcknowledgeItem called for package:", package_id)

    // Acknowledge the item and return the item info and status.
    item := make(map[string]dbus.Variant)
    item["id"] = dbus.MakeVariant(item_id)

    // Reset the timeout
    iface.reset_timer()
    return item, nil
}

func (iface *PayService) GetItem(message dbus.Message, item_id string) (map[string]dbus.Variant, *dbus.Error) {
    iface.pause_timer()
    package_id := package_id_from_path(message)

    fmt.Println("DEBUG - GetItem called for package:", package_id)

    // Get the item and return its info.
    item := make(map[string]dbus.Variant)
    item["id"] = dbus.MakeVariant(item_id)

    return item, nil
}

func (iface *PayService) GetPurchasedItems(message dbus.Message) ([]map[string]dbus.Variant, *dbus.Error) {
    iface.pause_timer()
    package_id := package_id_from_path(message)

    fmt.Println("DEBUG - GetPurchasedItems called for package:", package_id)

    // Get the purchased items, and their properties, for the package.
    purchasedItems := make([]map[string]dbus.Variant, 0)

    // Reset the timeout
    iface.reset_timer()
    // Need to actually get the items from somewhere, then validate and return
    return purchasedItems, nil
}

func (iface *PayService) PurchaseItem(message dbus.Message, item_id string) (map[string]dbus.Variant, *dbus.Error) {
    iface.pause_timer()
    package_id := package_id_from_path(message)

    fmt.Println("DEBUG - PurchaseItem called for package:", package_id)

    // Purchase the item and return the item info and status.
    item := make(map[string]dbus.Variant)
    item["id"] = dbus.MakeVariant(item_id)

    // Reset the timeout
    iface.reset_timer()
    return item, nil
}

func (iface *PayService) RefundItem(message dbus.Message, item_id string) (map[string]dbus.Variant, *dbus.Error) {
    iface.pause_timer()
    package_id := package_id_from_path(message)

    fmt.Println("DEBUG - RefundItem called for package:", package_id)

    // Refund the item and return the item info and status.
    item := make(map[string]dbus.Variant)
    item["id"] = dbus.MakeVariant(item_id)

    // Reset the timeout
    iface.reset_timer()
    return item, nil
}

func (iface *PayService) pause_timer() bool {
    return iface.shutdownTimer.Stop()
}

func (iface *PayService) reset_timer() bool {
    return iface.shutdownTimer.Reset(ShutdownTimeout)
}

/* Get the decoded package_id from a path
 */
func package_id_from_path(message dbus.Message) (string) {
    // Get the package ID
    called_path := message.Headers[dbus.FieldPath].String()
    package_id := path.Base(called_path)

    return package_id
}
