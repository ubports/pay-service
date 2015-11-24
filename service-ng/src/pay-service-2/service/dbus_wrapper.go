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
    "strconv"
)

// DbusWrapper is an interface to be implemented by any struct that wants to be
// injectable into this daemon for dbus communication.
type DbusWrapper interface {
    Connect() error
    Stop() error
    Names() []string
    RequestName(name string, flags dbus.RequestNameFlags) (dbus.RequestNameReply, error)
    GetNameOwner(name string) (string, error)
    Export(object interface{}, path dbus.ObjectPath, iface string) error
    ExportSubtree(object interface{}, path dbus.ObjectPath, iface string) error
    Emit(path dbus.ObjectPath, name string, values ...interface{}) error
}

// EncodePath encodes a path for DBus.
//
// Parameters:
// - path: string of the object path
//
// Returns:
// - string: encoded object path
func EncodeDbusPath(path string) string {
    output := ""

    for i := 0; i < len(path); i++ {
        c := path[i]
        if (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '/' || c == '_' {
            output += fmt.Sprintf("%c", c)
        } else {
            output += fmt.Sprintf("_%2x", c)
        }
    }
    return output
}

// DecodePath decodes an encoded object path string.
//
// Parameters:
// - path: string of the object path
//
// Returns:
// - string: decoded path
func DecodeDbusPath(path string) string {
    output := ""

    for i := 0; i < len(path); i++ {
        if path[i] == '_' {
            temp := fmt.Sprintf("%c%c", path[i + 1], path[i + 2])
            char, _ := strconv.ParseInt(temp, 16, 0)
            output += fmt.Sprintf("%c", char)
            i += 2
        } else {
            output += fmt.Sprintf("%c", path[i])
        }
    }
    return output
}
