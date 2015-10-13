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


func TestAcknowledgeItemConsumable(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := NewFakeTimer(ShutdownTimeout)
    client := new(FakeWebClient)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/foo_2Eexample")
    result, dbusErr := payiface.AcknowledgeItem(m, "consumable")
    if dbusErr != nil {
        t.Errorf("Unexpected error: %s", dbusErr)
    }

    if result["state"].Value().(string) != "available" {
        t.Errorf("Acknowledge of consumable item failed.")
    }

    if !timer.stopCalled {
        t.Errorf("Timer was not stopped.")
    }

    if !timer.resetCalled {
        t.Errorf("Timer was not reset.")
    }
}

func TestAcknowledgeItemUnlockable(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := NewFakeTimer(ShutdownTimeout)
    client := new(FakeWebClient)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/foo_2Eexample")
    result, dbusErr := payiface.AcknowledgeItem(m, "unlockable")
    if dbusErr != nil {
        t.Errorf("Unexpected error: %s", dbusErr)
    }

    if result["state"].Value().(string) != "purchased" {
        t.Errorf("Acknowledge of unlockable item failed.")
    }
    if !timer.stopCalled {
        t.Errorf("Timer was not stopped.")
    }

    if !timer.resetCalled {
        t.Errorf("Timer was not reset.")
    }
}

func TestGetItemConsumable(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := NewFakeTimer(ShutdownTimeout)
    client := new(FakeWebClient)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/foo_2Eexample")
    _, dbusErr := payiface.GetItem(m, "consumable")
    if dbusErr != nil {
        t.Errorf("Unexpected error geting item details: %s", dbusErr)
    }

    if !timer.stopCalled {
        t.Errorf("Timer was not stopped.")
    }

    if !timer.resetCalled {
        t.Errorf("Timer was not reset.")
    }
}

func TestGetItemClickScope(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := NewFakeTimer(ShutdownTimeout)
    client := new(FakeWebClient)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/click_2Dscope")
    _, dbusErr := payiface.GetItem(m, "foo.example")
    if dbusErr != nil {
        t.Errorf("Unexpected error geting item details: %s", dbusErr)
    }

    if !timer.stopCalled {
        t.Errorf("Timer was not stopped.")
    }

    if !timer.resetCalled {
        t.Errorf("Timer was not reset.")
    }
}

func TestGetItemAppNotRefundable(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := new(FakeTimer)
    client := new(FakeWebClient)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/click_2Dscope")
    item, dbusErr := payiface.GetItem(m, "foo.example")
    if dbusErr != nil {
        t.Errorf("Unexpected error geting item details: %s", dbusErr)
    }

    _, skuOk := item["sku"]
    if !skuOk {
        t.Errorf("Package name not normalized to 'sku' key.")
    }

    refundableUntil, ok := item["refundable_until"]
    if !ok {
        t.Errorf("Value for 'refundable_until' not included in map.")
    }

    if refundableUntil.Value().(uint64) != 0 {
        t.Errorf("Excpected refundable_until of '0', got '%d' instead.",
            refundableUntil)
    }

    if !timer.stopCalled {
        t.Errorf("Timer was not stopped.")
    }

    if !timer.resetCalled {
        t.Errorf("Timer was not reset.")
    }
}

func TestGetItemAppRefundable(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := new(FakeTimer)
    client := new(FakeWebClient)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/click_2Dscope")
    item, dbusErr := payiface.GetItem(m, "bar.example")
    if dbusErr != nil {
        t.Errorf("Unexpected error geting item details: %s", dbusErr)
    }

    refundableUntil, ok := item["refundable_until"]
    if !ok {
        t.Errorf("Value for 'refundable_until' not included in map.")
    }

    if refundableUntil.Value().(uint64) != 4102444799 {
        t.Errorf("Excpected refundable_until in the future, got '%d' instead.",
            refundableUntil.Value().(uint64))
    }

    if !timer.stopCalled {
        t.Errorf("Timer was not stopped.")
    }

    if !timer.resetCalled {
        t.Errorf("Timer was not reset.")
    }
}

func TestGetItemAppCancelled(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := new(FakeTimer)
    client := new(FakeWebClient)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/click_2Dscope")
    item, dbusErr := payiface.GetItem(m, "cancelled.example")
    if dbusErr != nil {
        t.Errorf("Unexpected error geting item details: %s", dbusErr)
    }

    state, ok := item["state"]
    if !ok {
        t.Errorf("Value for 'state' not included in map.")
    }

    if state.Value().(string) != "available" {
        t.Errorf("Excpected stated to be 'available', got '%s' instead.",
            state.Value().(string))
    }

    if !timer.stopCalled {
        t.Errorf("Timer was not stopped.")
    }

    if !timer.resetCalled {
        t.Errorf("Timer was not reset.")
    }
}

func TestGetPurchasedItemsClickScope(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := NewFakeTimer(ShutdownTimeout)
    client := new(FakeWebClient)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/click_2Dscope")
    reply, dbusErr := payiface.GetPurchasedItems(m)
    if dbusErr != nil {
        t.Errorf("Unexpected error listing purchased items: %s", dbusErr)
    }

    if len(reply) != 2 {
        t.Errorf("Expected 2 items in list, got %d instead.", len(reply))
    }

    if !timer.stopCalled {
        t.Errorf("Timer was not stopped.")
    }

    if !timer.resetCalled {
        t.Errorf("Timer was not reset.")
    }
}

func TestGetPurchasedItemsInAppPurchase(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := NewFakeTimer(ShutdownTimeout)
    client := new(FakeWebClient)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/foo_2Eexample")
    reply, dbusErr := payiface.GetPurchasedItems(m)
    if dbusErr != nil {
        t.Errorf("Unexpected error listing purchased items: %s", dbusErr)
    }

    if len(reply) != 2 {
        t.Errorf("Expected 2 items in list, got %d instead.", len(reply))
    }

    if !timer.stopCalled {
        t.Errorf("Timer was not stopped.")
    }

    if !timer.resetCalled {
        t.Errorf("Timer was not reset.")
    }
}

func TestPurchaseItem(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := NewFakeTimer(ShutdownTimeout)
    client := new(FakeWebClient)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    launchCalled := false
    payiface.launchPayUiFunction = func(appId string, purchaseUrl string) PayUiFeedback {
        launchCalled = true
        feedback := PayUiFeedback{
            Finished: make(chan struct{}),
            Error: make(chan error, 1),
        }

        // Finished
        close(feedback.Error)
        close(feedback.Finished)

        return feedback
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/foo_2Eexample")
    _, dbusErr := payiface.PurchaseItem(m, "consumable")
    if dbusErr != nil {
        t.Errorf("Unexpected error when purchasing item: %s", dbusErr)
    }

    if !timer.stopCalled {
        t.Errorf("Timer was not stopped.")
    }

    if !timer.resetCalled {
        t.Errorf("Timer was not reset.")
    }

    if !launchCalled {
        t.Error("Expected LaunchPayUi() to be called.")
    }
}

func TestPurchaseItem_payUiError(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := NewFakeTimer(ShutdownTimeout)
    client := new(FakeWebClient)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    launchCalled := false
    payiface.launchPayUiFunction = func(appId string, purchaseUrl string) PayUiFeedback {
        launchCalled = true
        feedback := PayUiFeedback{
            Finished: make(chan struct{}),
            Error: make(chan error, 1),
        }

        // Failure
        feedback.Error <- fmt.Errorf("Failed at user request")

        // Finished
        close(feedback.Error)
        close(feedback.Finished)

        return feedback
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/foo_2Eexample")
    _, dbusErr := payiface.PurchaseItem(m, "consumable")
    if dbusErr == nil {
        t.Error("Expected an error due to Pay UI error")
    }

    if !timer.stopCalled {
        t.Errorf("Timer was not stopped.")
    }

    if !timer.resetCalled {
        t.Errorf("Timer was not reset.")
    }

    if !launchCalled {
        t.Error("Expected LaunchPayUi() to be called.")
    }
}

func TestRefundItem(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := NewFakeTimer(ShutdownTimeout)
    client := new(FakeWebClient)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/click_2Dscope")
    reply, dbusErr := payiface.RefundItem(m, "bar.example")
    if dbusErr != nil {
        t.Errorf("Unexpected error refunding item: %s", dbusErr)
    }

    if len(reply) == 0 {
        t.Errorf("Expected values in map, got none instead.")
    }

    if reply["state"].Value().(string) == "Complete" {
        t.Errorf("Expected successful refund result.")
    }

    if !timer.stopCalled {
        t.Errorf("Timer was not stopped.")
    }

    if !timer.resetCalled {
        t.Errorf("Timer was not reset.")
    }
}

func TestRefundItemInvalid(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := new(FakeTimer)
    client := new(FakeWebClient)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/click_2Dscope")
    _, dbusErr := payiface.RefundItem(m, "click-scope")
    if dbusErr == nil {
        t.Errorf("Expected error refunding item, received none.")
    }

    if !timer.stopCalled {
        t.Errorf("Timer was not stopped.")
    }

    if !timer.resetCalled {
        t.Errorf("Timer was not reset.")
    }
}

func TestPackageNameFromPath(t *testing.T) {
    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    objPath := dbus.ObjectPath("/com/canonical/pay/store/click_2Dscope")
    m.Headers[dbus.FieldPath] = dbus.MakeVariant(objPath)

    expected := "click-scope"
    endPath := packageNameFromPath(m)
    if endPath != expected {
        t.Errorf("Unexpected package name `%s`, expected `%s`.",
            endPath, expected)
    }
}
