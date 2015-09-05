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

    // To set any extra headers we need (signature, accept, etc)
    var headers http.Header

    if packageName == "click-scope" {
        url := getPayClickUrl()
        url += "/purchases/"

        result, err := iface.client.Call(url, "GET", headers, "")
        if err != nil {
            return nil, dbus.NewError(fmt.Sprintf("RequestError: %s", err), nil)
        }
        var data interface{}
        err = json.Unmarshal([]byte(result), &data)
        if err != nil {
            return nil, dbus.NewError(fmt.Sprintf("ParseError: %s", err), nil)
        }

        m := data.([]interface{})
        for index := range m {
            itemMap := m[index].(map[string]interface{})
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
                case nil:
                    // Just ignore nulls in the JSON
                default:
                    fmt.Println("WARNING - Unable to parse purchase key:", k)
                }
            }
            purchasedItems = append(purchasedItems, details)
        }

        return purchasedItems, nil
    } else {
        url := getPayIventoryUrl()
        url += "/" + packageName + "/purchases/"

        result, err := iface.client.Call(url, "GET", headers, "")
        if err != nil {
            return nil, dbus.NewError(fmt.Sprintf("RequestError: %s", err), nil)
        }

        var data interface{}
        err = json.Unmarshal([]byte(result), &data)
        if err != nil {
            return nil, dbus.NewError(fmt.Sprintf("ParseError: %s", err), nil)
        }

        m := data.(map[string]interface{})["_embedded"].(map[string]interface{})
        q := m["purchases"].([]interface{})
        for purchase := range q {
            purchaseMap := q[purchase].(map[string]interface{})
            itemList := purchaseMap["_embedded"].(
                map[string]interface{})["items"].([]interface{})
            for index := range itemList {
                details := make(ItemDetails)
                itemMap := itemList[index].(map[string]interface{})
                for k, v := range itemMap {
                    switch vv := v.(type) {
                    case string, bool, int:
                        details[k] = dbus.MakeVariant(vv)
                    case nil:
                        // Just ignore nulls in the JSON
                    default:
                        fmt.Println("WARNING - Unable to parse purchase key:", k)
                    }
                }
                details["requeted_device"] = dbus.MakeVariant(
                    purchaseMap["requested_device"])
                // FIXME: parse timestamps and add them here too
                purchasedItems = append(purchasedItems, details)
            }
        }

        return purchasedItems, nil
    }

    // Invalid package, so return an error.
    return nil, dbus.NewError(fmt.Sprintf("InvalidPackage: %s", packageName), nil)
}

func (iface *PayService) PurchaseItem(message dbus.Message, itemName string) (map[string]dbus.Variant, *dbus.Error) {
    iface.pauseTimer()
    packageName := packageNameFromPath(message)

    fmt.Println("DEBUG - PurchaseItem called for package:", packageName)

    // Purchase the item and return the item info and status.
    item := make(map[string]dbus.Variant)

    // Reset the timeout
    iface.resetTimer()
    return item, nil
}

func (iface *PayService) RefundItem(message dbus.Message, itemName string) (map[string]dbus.Variant, *dbus.Error) {
    iface.pauseTimer()
    defer iface.resetTimer()

    packageName := packageNameFromPath(message)

    fmt.Println("DEBUG - RefundItem called for package:", packageName)

    if packageName != "click-scope" {
        return nil, dbus.NewError(
            "Unsupported: Refunds only supported for packages.", nil)
    }

    // Refund the item and return the item info and status.
    item := make(map[string]dbus.Variant)

    // To set any extra headers we need (signature, accept, etc)
    var headers http.Header

    url := getPayClickUrl() + "/refunds/"
    body := `{"name": "` + packageName + `"}`
    result, err := iface.client.Call(url, "POST", headers, body)
    if err != nil {
        return nil, dbus.NewError(fmt.Sprintf("RequestError: %s", err), nil)
    }

    var data interface{}
    err = json.Unmarshal([]byte(result), &data)
    if err != nil {
        return nil, dbus.NewError(fmt.Sprintf("ParseError: %s", err), nil)
    }

    m := data.(map[string]interface{})
    item["success"] = dbus.MakeVariant(m["success"])

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
    if calledPath[0] == '"' && calledPath[len(calledPath) - 1] == '"' {
        calledPath = calledPath[1:len(calledPath) - 1]
    }
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
