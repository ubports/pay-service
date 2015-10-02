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
    "os"
    "testing"
)

func TestIsSupportedCurrencyDefault(t *testing.T) {
    if IsSupportedCurrency(defaultCurrency) != true {
        t.Fatalf("The default currency '%s', is not supported!",
            defaultCurrency)
    }
}

func TestIsSupportedCurrencyInvalid(t *testing.T) {
    invalidCurrency := "ZZZ"
    if IsSupportedCurrency(invalidCurrency) != false {
        t.Fatalf("The invalid currency '%s', is supported!", invalidCurrency)
    }
}

func TestSymbolForCurrencyDefault(t *testing.T) {
    result := SymbolForCurrency(defaultCurrency)
    expected := "US$"
    if result != expected {
        t.Fatalf("Expected '%s' for currency '%s', but got '%s' instead.",
            expected, defaultCurrency, result)
    }
}

func TestSymbolForCurrencyInvalid(t *testing.T) {
    invalidCurrency := "ZZZ"
    result := SymbolForCurrency(invalidCurrency)
    if result != invalidCurrency {
        t.Fatalf("Expected '%s' for currency '%s', but got '%s' instead.",
            invalidCurrency, invalidCurrency, result)
    }
}

func TestPreferredCurrencyCodeARS(t *testing.T) {
    currencies := ItemDetails{
        "USD": dbus.MakeVariant("0.99"),
        "GBP": dbus.MakeVariant("0.99"),
        "EUR": dbus.MakeVariant("0.99"),
        "ARS": dbus.MakeVariant("0.99"),
    }
    currencyCode := "ARS"
    result := PreferredCurrencyCode(currencyCode, currencies)
    if result != defaultCurrency {
        t.Fatalf("Expected '%s' as usable currency, got '%s' instead.",
            currencyCode, result)
    }
}

func TestPreferredCurrencyCodeARSFallback(t *testing.T) {
    currencies := ItemDetails{
        "USD": dbus.MakeVariant("0.99"),
        "GBP": dbus.MakeVariant("0.99"),
        "EUR": dbus.MakeVariant("0.99"),
    }
    currencyCode := "ARS"
    result := PreferredCurrencyCode(currencyCode, currencies)
    if result != defaultCurrency {
        t.Fatalf("Expected '%s' as usable currency, got '%s' instead.",
            defaultCurrency, result)
    }
}

func TestPreferredCurrencyCodeEnvironment(t *testing.T) {
    currencies := ItemDetails{
        "USD": dbus.MakeVariant("0.99"),
        "GBP": dbus.MakeVariant("0.99"),
        "EUR": dbus.MakeVariant("0.99"),
        "HKD": dbus.MakeVariant("0.99"),
    }
    currencyCode := "HKD"
    os.Setenv(SearchCurrencyEnvVar, currencyCode)
    // FIXME: Apparently golang on vivid doesn't have Unsetenv :(
    defer os.Setenv(SearchCurrencyEnvVar, "")

    result := PreferredCurrencyCode("ZZZ", currencies)
    if result != currencyCode {
        t.Fatalf("Expected '%s' as usable currency, got '%s' instead.",
            currencyCode, result)
    }
}
