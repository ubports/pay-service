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

type PayService struct {
    dbusConnection DbusWrapper
    baseObjectPath dbus.ObjectPath
}

func NewPayService(dbusConnection DbusWrapper,
    interfaceName string, baseObjectPath dbus.ObjectPath) (*PayService, error) {
    payiface := &PayService{dbusConnection: dbusConnection}

    if !baseObjectPath.IsValid() {
        return nil, fmt.Errorf(`Invalid base object path: "%s"`, baseObjectPath)
    }

    payiface.baseObjectPath = baseObjectPath

    return payiface, nil
}

func (iface *PayService) ListPurchasedItems(message dbus.Message) (map[string]string, *dbus.Error) {
    purchasedItems := make(map[string]string)

    // Need to actually get the items from somewhere, then validate and return
    return purchasedItems, nil
}
