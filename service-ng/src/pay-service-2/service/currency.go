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

// #cgo pkg-config: Qt5Core
// #cgo CXXFLAGS: -Wall
// #include "currency.h"
// #include <stdlib.h>
import "C"

import (
    "os"
    "unsafe"
)

const (
    defaultCurrency = "USD"
)


var currencySymbolMap = map[string]string {
    "CNY": "RMB",
    "EUR": "€",
    "GBP": "₤",
    "HKD": "HK$",
    "TWD": "TW$",
    "USD": "US$",
}


func CurrencyString(price float64, symbol string) (string) {
    symbolCstring := C.CString(symbol)
    defer C.free(unsafe.Pointer(symbolCstring))

    result := C.toCurrencyString(C.double(price), symbolCstring)
    defer C.free(unsafe.Pointer(result))

    return C.GoString(result)
}

func IsSupportedCurrency(currencyCode string) (bool) {
    _, symbolOk := currencySymbolMap[currencyCode]
    return symbolOk
}

func SymbolForCurrency(currencyCode string) (string) {
    if IsSupportedCurrency(currencyCode) {
        return currencySymbolMap[currencyCode]
    }
    return currencyCode
}

func currencyInValidCurrencies(currency string, currencies []string) (bool) {
    for _, b := range currencies {
        if b == currency {
            return true
        }
    }
    return false
}

func PreferredCurrencyCode(suggestedCurrency string, validCurrencies []string) (string) {
    preferredCurrency := os.Getenv("U1_SEARCH_CURRENCY")
    if preferredCurrency == "" {
        if currencyInValidCurrencies(suggestedCurrency, validCurrencies) {
            return suggestedCurrency
        }
        return defaultCurrency
    }
    if currencyInValidCurrencies(preferredCurrency, validCurrencies) {
        return preferredCurrency
    }
    return defaultCurrency
}
