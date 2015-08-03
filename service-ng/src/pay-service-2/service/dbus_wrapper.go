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

import "github.com/godbus/dbus"

// DbusWrapper is an interface to be implemented by any struct that wants to be
// injectable into this daemon for dbus communication.
type DbusWrapper interface {
    Connect() error
    Stop() error
    Names() []string
    RequestName(name string, flags dbus.RequestNameFlags) (dbus.RequestNameReply, error)
    GetNameOwner(name string) (string, error)
    Export(object interface{}, path dbus.ObjectPath, iface string) error
    Emit(path dbus.ObjectPath, name string, values ...interface{}) error
}
