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
    "encoding/json"
    "fmt"
    "github.com/godbus/dbus"
    "net/http"
    "os"
    "path"
)


const (
    // PayBaseUrl is the default base URL for the REST API.
    PayBaseUrl = "https://myapps.developer.ubuntu.com"
)

type ItemDetails map[string]dbus.Variant

type PayService struct {
    dbusConnection DbusWrapper
    baseObjectPath dbus.ObjectPath
    shutdownTimer  Timer
    client         WebClientIface
}

func NewPayService(dbusConnection DbusWrapper,
    interfaceName string, baseObjectPath dbus.ObjectPath,
    shutdownTimer Timer, client WebClientIface) (*PayService, error) {
    payiface := &PayService{
        dbusConnection: dbusConnection,
        shutdownTimer: shutdownTimer,
        client: client,
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

func (iface *PayService) GetPurchasedItems(message dbus.Message) ([]ItemDetails, *dbus.Error) {
    iface.pauseTimer()
    defer iface.resetTimer()

    packageName := packageNameFromPath(message)

    fmt.Println("DEBUG - GetPurchasedItems called for package:", packageName)

    // Get the purchased items, and their properties, for the package.
    purchasedItems := make([]ItemDetails, 0)

    var headers http.Header
    if packageName == "click-scope" {
        url := getPayClickUrl()
        url += "/purchases/"

        result, err := iface.client.Call(url, "GET", headers, "")
        if err != nil {
            errs := make([]interface{}, 0)
            errs = append(errs, err)
            return nil, dbus.NewError("RequestError", errs)
        }
        var data interface{}
        err = json.Unmarshal([]byte(result), &data)
        m := data.([]map[string]interface{})
        for index := range m {
            itemMap := m[index]
            details := make(ItemDetails)
            for k, v := range itemMap {
                switch vv := v.(type) {
                case string: {
                    switch k {
                    case "refundable_until":
                        // TODO: Parse the time to an int64
                    default:
                        details[k] = dbus.MakeVariant(vv)
                    }
                }
                case bool, int:
                    details[k] = dbus.MakeVariant(vv)
                }
            }
            purchasedItems = append(purchasedItems, details)
        }
        fmt.Println("Returning details: %s", purchasedItems)

        return purchasedItems, nil
    } else {
        url := getPayIventoryUrl()
        url += "/" + packageName + "/purchases/"

        return purchasedItems, nil
    }
    errs := make([]interface{}, 0)
    errs = append(errs,
        fmt.Errorf("Failed to find get purchases for package: %s",
            packageName))
    return nil, dbus.NewError("InvalidPackage", errs)
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

/* Get the full URL for an API path
 */
func getPayBaseUrl() string {
    baseUrl := os.Getenv("PAY_BASE_URL")
    if baseUrl == "" {
        baseUrl = PayBaseUrl
    }
    return baseUrl
}

func getPayClickUrl() string {
    url := getPayBaseUrl()
    url += "/api/2.0/click"
    return url
}

func getPayIventoryUrl() string {
    url := getPayBaseUrl()
    url += "/packages"
    return url
}
