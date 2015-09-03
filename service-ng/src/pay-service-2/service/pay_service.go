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

func (iface *PayService) AcknowledgeItem(message dbus.Message, itemName string) (map[string]dbus.Variant, *dbus.Error) {
    iface.pauseTimer()
    packageName := packageNameFromPath(message)

    fmt.Println("DEBUG - AcknowledgeItem called for package:", packageName)

    // Acknowledge the item and return the item info and status.
    item := make(map[string]dbus.Variant)
    item["id"] = dbus.MakeVariant(itemName)

    // Reset the timeout
    iface.resetTimer()
    return item, nil
}

func (iface *PayService) GetItem(message dbus.Message, itemName string) (map[string]dbus.Variant, *dbus.Error) {
    iface.pauseTimer()
    packageName := packageNameFromPath(message)

    fmt.Println("DEBUG - GetItem called for package:", packageName)

    // Get the item and return its info.
    item := make(map[string]dbus.Variant)
    item["id"] = dbus.MakeVariant(itemName)

    // Reset the timeout
    iface.resetTimer()
    return item, nil
}

func (iface *PayService) GetPurchasedItems(message dbus.Message) ([]map[string]dbus.Variant, *dbus.Error) {
    iface.pauseTimer()
    packageName := packageNameFromPath(message)

    fmt.Println("DEBUG - GetPurchasedItems called for package:", packageName)

    // Get the purchased items, and their properties, for the package.
    purchasedItems := make([]map[string]dbus.Variant, 0)

    // Reset the timeout
    iface.resetTimer()
    // Need to actually get the items from somewhere, then validate and return
    return purchasedItems, nil
}

func (iface *PayService) PurchaseItem(message dbus.Message, itemName string) (map[string]dbus.Variant, *dbus.Error) {
    iface.pauseTimer()
    packageName := packageNameFromPath(message)

    fmt.Println("DEBUG - PurchaseItem called for package:", packageName)

    // Purchase the item and return the item info and status.
    item := make(map[string]dbus.Variant)
    item["id"] = dbus.MakeVariant(itemName)

    // Reset the timeout
    iface.resetTimer()
    return item, nil
}

func (iface *PayService) RefundItem(message dbus.Message, itemName string) (map[string]dbus.Variant, *dbus.Error) {
    iface.pauseTimer()
    packageName := packageNameFromPath(message)

    fmt.Println("DEBUG - RefundItem called for package:", packageName)

    // Refund the item and return the item info and status.
    item := make(map[string]dbus.Variant)
    item["id"] = dbus.MakeVariant(itemName)

    // Reset the timeout
    iface.resetTimer()
    return item, nil
}

func (iface *PayService) pauseTimer() bool {
    return iface.shutdownTimer.Stop()
}

func (iface *PayService) resetTimer() bool {
    return iface.shutdownTimer.Reset(ShutdownTimeout)
}

/* Get the decoded packageName from a path
 */
func packageNameFromPath(message dbus.Message) (string) {
    // Get the package ID
    calledPath := DecodeDbusPath(message.Headers[dbus.FieldPath].String())
    packageName := path.Base(calledPath)

    return packageName
}
