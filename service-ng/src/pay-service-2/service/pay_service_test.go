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
    "os"
    "testing"

    "github.com/godbus/dbus"
    "launchpad.net/go-trust-store/trust/fakes"
)


func TestAcknowledgeItemConsumable(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := NewFakeTimer(ShutdownTimeout)
    client := new(FakeWebClient)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client, false)
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

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client, false)
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

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client, false)
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

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client, false)
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

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client, false)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/click_2dscope")
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

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client, false)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/click_2dscope")
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

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client, false)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/click_2dscope")
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

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client, false)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/click_2dscope")
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

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client, false)
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

func TestGetPurchasedItemsEmpty(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := NewFakeTimer(ShutdownTimeout)
    client := new(FakeWebClient)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client, false)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/empty")
    reply, dbusErr := payiface.GetPurchasedItems(m)
    if dbusErr != nil {
        t.Errorf("Unexpected error listing purchased items: %s", dbusErr)
    }

    if len(reply) != 0 {
        t.Errorf("Expected 0 items in list, got %d instead.", len(reply))
    }

    if !timer.stopCalled {
        t.Errorf("Timer was not stopped.")
    }

    if !timer.resetCalled {
        t.Errorf("Timer was not reset.")
    }
}

func TestGetPurchasedItemsEmptyInvalid(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := NewFakeTimer(ShutdownTimeout)
    client := new(FakeWebClient)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client, false)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/empty_2einvalid")
    reply, dbusErr := payiface.GetPurchasedItems(m)
    if dbusErr != nil {
        t.Errorf("Unexpected error listing purchased items: %s", dbusErr)
    }

    if len(reply) != 0 {
        t.Errorf("Expected 0 items in list, got %d instead.", len(reply))
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

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client, false)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    // Setup fake trust store agent
    fakeAgent := &fakes.Agent{GrantAuthentication: true}
    payiface.trustStoreAgent = fakeAgent
    payiface.useTrustStore = true

    // Setup fake trust-store-related functions
    payiface.tripletToAppIdFunction = func(string, string, string) string {
        return "foo_bar_1.2.3"
    }

    payiface.getPrimaryPidFunction = func(string) uint32 {
        return 42
    }

    // Setup fake pay-ui launcher
    launchCalled := false
    payiface.launchPayUiFunction = func(string, string) PayUiFeedback {
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

    if !fakeAgent.AuthenticateRequestWithParametersCalled {
        t.Error("Expected agent AuthenticateRequestWithParameters() to be called.")
    }

    parameters := fakeAgent.AuthenticationRequestParameters
    if parameters.Application.Uid != os.Getuid() {
        t.Errorf("Authentication request uid was %d, expected %d",
                 parameters.Application.Uid, os.Getuid())
    }

    if parameters.Application.Pid != 42 {
        t.Errorf("Authentication request pid was %d, expected 42",
                 parameters.Application.Pid)
    }

    if parameters.Application.Id != "foo_bar_1.2.3" {
        t.Errorf(`Authentication request id was "%s", expected "foo_bar_1.2.3"`,
                 parameters.Application.Id)
    }

    if parameters.Feature != FeaturePurchaseItem {
        t.Errorf("Authentication request feature was %d, expected %d",
                 parameters.Feature, FeaturePurchaseItem)
    }
}

func TestPurchaseItem_payUiError(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := NewFakeTimer(ShutdownTimeout)
    client := new(FakeWebClient)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client, false)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    launchCalled := false
    payiface.launchPayUiFunction = func(string, string) PayUiFeedback {
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

func TestPurchaseItem_trustClickScope(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := NewFakeTimer(ShutdownTimeout)
    client := new(FakeWebClient)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client, false)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    // Setup fake trust store agent
    fakeAgent := &fakes.Agent{GrantAuthentication: false}
    payiface.trustStoreAgent = fakeAgent
    payiface.useTrustStore = true

    // Setup fake trust-store-related functions
    payiface.tripletToAppIdFunction = func(string, string, string) string {
        return "foo_bar_1.2.3"
    }

    payiface.getPrimaryPidFunction = func(string) uint32 {
        return 42
    }

    // Setup fake pay-ui launcher
    launchCalled := false
    payiface.launchPayUiFunction = func(string, string) PayUiFeedback {
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
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/click_2Dscope")
    _, dbusErr := payiface.PurchaseItem(m, "app.foo")
    if dbusErr != nil {
        t.Errorf("Unexpected error for click-scope: %s", dbusErr)
    }

    if fakeAgent.AuthenticateRequestWithParametersCalled {
        t.Error("Expected agent AuthenticateRequestWithParameters() to not be called.")
    }
}

func TestPurchaseItem_accessDenied(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := NewFakeTimer(ShutdownTimeout)
    client := new(FakeWebClient)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client, false)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    // Setup fake trust store agent
    fakeAgent := &fakes.Agent{GrantAuthentication: false}
    payiface.trustStoreAgent = fakeAgent
    payiface.useTrustStore = true

    // Setup fake trust-store-related functions
    payiface.tripletToAppIdFunction = func(string, string, string) string {
        return "foo_bar_1.2.3"
    }

    payiface.getPrimaryPidFunction = func(string) uint32 {
        return 42
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/foo_2Eexample")
    _, dbusErr := payiface.PurchaseItem(m, "consumable")
    if dbusErr == nil {
        t.Errorf("Expected an error due to purchase access being denied")
    }

    if !fakeAgent.AuthenticateRequestWithParametersCalled {
        t.Error("Expected agent AuthenticateRequestWithParameters() to be called.")
    }
}

func TestPurchaseItem_errorFindingAppId(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := NewFakeTimer(ShutdownTimeout)
    client := new(FakeWebClient)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client, false)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    // Setup fake trust store agent
    fakeAgent := &fakes.Agent{GrantAuthentication: true}
    payiface.trustStoreAgent = fakeAgent
    payiface.useTrustStore = true

    // Setup fake trust-store-related functions
    payiface.tripletToAppIdFunction = func(string, string, string) string {
        return "" // This is an error
    }

    payiface.getPrimaryPidFunction = func(string) uint32 {
        return 42
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/foo_2Eexample")
    _, dbusErr := payiface.PurchaseItem(m, "consumable")
    if dbusErr == nil {
        t.Errorf("Expected an error due to inability to get application ID")
    }
}

func TestPurchaseItem_errorFindingAppPid(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := NewFakeTimer(ShutdownTimeout)
    client := new(FakeWebClient)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client, false)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    // Setup fake trust store agent
    fakeAgent := &fakes.Agent{GrantAuthentication: true}
    payiface.trustStoreAgent = fakeAgent
    payiface.useTrustStore = true

    // Setup fake trust-store-related functions
    payiface.tripletToAppIdFunction = func(string, string, string) string {
        return "foo_bar_1.2.3"
    }

    payiface.getPrimaryPidFunction = func(string) uint32 {
        return 0 // This is an error
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/foo_2Eexample")
    _, dbusErr := payiface.PurchaseItem(m, "consumable")
    if dbusErr == nil {
        t.Errorf("Expected an error due to inability to get application PID")
    }
}

func TestRefundItem(t *testing.T) {
    dbusServer := new(FakeDbusServer)
    dbusServer.InitializeSignals()
    timer := NewFakeTimer(ShutdownTimeout)
    client := new(FakeWebClient)

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client, false)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/click_2dscope")
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

    payiface, err := NewPayService(dbusServer, "foo", "/foo", timer, client, false)
    if err != nil {
        t.Fatalf("Unexpected error while creating pay service: %s", err)
    }

    if payiface == nil {
        t.Fatalf("Pay service not created.")
    }

    var m dbus.Message
    m.Headers = make(map[dbus.HeaderField]dbus.Variant)
    m.Headers[dbus.FieldPath] = dbus.MakeVariant("/com/canonical/pay/store/click_2dscope")
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
