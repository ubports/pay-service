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
    "testing"
)

func TestNew(t *testing.T) {
    client := new(FakeWebClient)
    timer := NewFakeTimer(ShutdownTimeout)
    service, err := New(client, timer)
    if err != nil {
        t.Fatalf("Unexpected error instantiating service: %s", err)
    }

    if service.server == nil {
        t.Error("Service dbus server was nil")
    }
}

func TestServiceRunStop(t *testing.T) {
    client := new(FakeWebClient)
    timer := NewFakeTimer(ShutdownTimeout)
    service, err := New(client, timer)
    if err != nil {
        t.Fatalf("Unexpected error instantiating service: %s", err)
    }

    err = service.Run()
    if err != nil {
        t.Error("Unexpected error running service: %s", err)
    }

    names := service.server.Names()
    if len(names) < 1 {
        t.Errorf("Got %d names, expected at least 1", len(names))
    }

    name, err := service.server.GetNameOwner(busName)
    if err != nil {
        t.Error("Unexpected error requesting name owner: %s", err)
    }

    if name != names[0] {
        t.Errorf("Name owner was '%s', expected to be '%s'.", name, names[0])
    }

    err = service.Shutdown()
    if err != nil {
        t.Errorf("Error shutting down service: %s", err)
    }
}
