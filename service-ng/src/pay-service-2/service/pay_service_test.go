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
    "github.com/godbus/dbus"
    "testing"
    "time"
)

func fake_timeout() {    
}

func TestAcknowledgeItemConsumable(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := time.AfterFunc(time.Duration(10) * time.Second, fake_timeout)
    
    payiface, err := NewPayService(dbusServer, "foo", "/foo", *timer)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/foo")
    reply, dbusErr := payiface.AcknowledgeItem(m, "bar")
    if dbusErr != nil {
        t.Errorf("Unexpected error listing purchased items: %s", dbusErr)
    }

    if len(reply) == 0 {
        t.Errorf("Expected values in map, got none instead.")
    }
}

func TestAcknowledgeItemUnlockable(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := time.AfterFunc(time.Duration(10) * time.Second, fake_timeout)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", *timer)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/foo")
    reply, dbusErr := payiface.AcknowledgeItem(m, "bar")
    if dbusErr != nil {
        t.Errorf("Unexpected error listing purchased items: %s", dbusErr)
    }

    if len(reply) == 0 {
        t.Errorf("Expected values in map, got none instead.")
    }
}

func TestGetItem(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := time.AfterFunc(time.Duration(10) * time.Second, fake_timeout)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", *timer)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/foo")
    reply, dbusErr := payiface.GetItem(m, "bar")
    if dbusErr != nil {
        t.Errorf("Unexpected error listing purchased items: %s", dbusErr)
    }

    if len(reply) == 0 {
        t.Errorf("Expected values in map, got none instead.")
    }
}

func TestGetPurchasedItems(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := time.AfterFunc(time.Duration(10) * time.Second, fake_timeout)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", *timer)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/foo")
    reply, dbusErr := payiface.GetPurchasedItems(m)
    if dbusErr != nil {
        t.Errorf("Unexpected error listing purchased items: %s", dbusErr)
    }

    if len(reply) != 0 {
        t.Errorf("Expected 0 values in map, got %d instead.", len(reply))
    }
}

func TestPurchaseItem(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := time.AfterFunc(time.Duration(10) * time.Second, fake_timeout)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", *timer)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/foo")
    reply, dbusErr := payiface.PurchaseItem(m, "bar")
    if dbusErr != nil {
        t.Errorf("Unexpected error listing purchased items: %s", dbusErr)
    }

    if len(reply) == 0 {
        t.Errorf("Expected 0 values in map, got none instead.")
    }
}

func TestRefundItem(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := time.AfterFunc(time.Duration(10) * time.Second, fake_timeout)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", *timer)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/foo")
    reply, dbusErr := payiface.RefundItem(m, "bar")
    if dbusErr != nil {
        t.Errorf("Unexpected error listing purchased items: %s", dbusErr)
    }

    if len(reply) == 0 {
        t.Errorf("Expected values in map, got none instead.")
    }
}
