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
    "reflect"
)


const (
    // payBaseUrl is the default base URL for the REST API.
    payBaseUrl = "https://myapps.developer.ubuntu.com"
)

type ItemDetails map[string]dbus.Variant
type LaunchPayUiFunction func(appId string, purchaseUrl string) PayUiFeedback

type PayService struct {
    dbusConnection DbusWrapper
    baseObjectPath dbus.ObjectPath
    shutdownTimer  Timer
    client         WebClientIface

    launchPayUiFunction LaunchPayUiFunction
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

    payiface.launchPayUiFunction = LaunchPayUi

    return payiface, nil
}

func (iface *PayService) AcknowledgeItem(message dbus.Message, itemName string) (ItemDetails, *dbus.Error) {
    iface.pauseTimer()
    defer iface.resetTimer()
    packageName := packageNameFromPath(message)

    fmt.Println("DEBUG - AcknowledgeItem called for package:", packageName)

    // Fail if calling AcknowledgeItem for click-scope
    if packageName == "click-scope" {
        return nil, dbus.NewError(
            "Unsupported: AcknowledgeItem not supported for packages.", nil)
    }


    purchases, err := iface.GetPurchasedItems(message)
    if err != nil {
        return nil, dbus.NewError(
            fmt.Sprintf("Unable to find item '%s': %s", packageName, err), nil)
    }

    for x := range purchases {
        sku := purchases[x]["sku"].Value().(string)
        if sku == itemName {
            // BUG: golang json is parsing all numberas as floats :(
            id := int64(purchases[x]["id"].Value().(float64))
            idString := fmt.Sprintf("%d", id)

            // To set any extra headers we need (signature, accept, etc)
            headers := make(http.Header)

            url := getPayInventoryUrl() + "/" + packageName +
                "/items/" + idString

            body := `{"state": "acknowledged"}`
            headers.Set(ContentTypeHeader, "application/json")

            data, err := iface.getDataForUrl(url, "PUT", headers, body)
            if err != nil {
                return nil, dbus.NewError(fmt.Sprintf("%s", err), nil)
            }

            item := parseItemMap(data.(map[string]interface{}))
            return item, nil
        }
    }

    return nil, dbus.NewError(
        fmt.Sprintf("Unable to find item '%s'", packageName), nil)
}

func (iface *PayService) GetItem(message dbus.Message, itemName string) (ItemDetails, *dbus.Error) {
    iface.pauseTimer()
    defer iface.resetTimer()
    packageName := packageNameFromPath(message)

    fmt.Println("DEBUG - GetItem called for package:", packageName)

    // To set any extra headers we need (signature, accept, etc)
    headers := make(http.Header)

    url := ""
    if packageName == "click-scope" {
        url = getPayClickUrl() + "/purchases/" + itemName + "/"
    } else {
        url = getPayInventoryUrl() + "/" + packageName +
            "/items/by-sku/" + itemName
    }
    data, err := iface.getDataForUrl(url, "GET", headers, "")
    if err != nil {
        return nil, dbus.NewError(fmt.Sprintf("%s", err), nil)
    }

    item := parseItemMap(data.(map[string]interface{}))
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
    headers := make(http.Header)

    // TODO: When cache is available abstract this away to avoid extraneous
    // network activity. For now we must always hit network.
    if packageName == "click-scope" {
        url := getPayClickUrl() + "/purchases/"
        headers.Set(AcceptHeader, "application/json")

        data, err := iface.getDataForUrl(url, "GET", headers, "")
        if err != nil {
            return nil, dbus.NewError(fmt.Sprintf("%s", err), nil)
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
                case bool, int64:
                    details[k] = dbus.MakeVariant(vv)
                case nil:
                    // For refundable_until, set the value to 0
                    if k == "refundable_until" {
                        details[k] = dbus.MakeVariant(int64(0))
                    }
                    // Just ignore nulls in the JSON
                default:
                    fmt.Println("WARNING - Unable to parse purchase key:", k)
                }
            }
            purchasedItems = append(purchasedItems, details)
        }

        return purchasedItems, nil
    } else {
        url := getPayInventoryUrl() + "/" + packageName + "/purchases"

        data, err := iface.getDataForUrl(url, "GET", headers, "")
        if err != nil {
            return nil, dbus.NewError(fmt.Sprintf("%s", err), nil)
        }
        headers.Set(AcceptHeader, "application/hal+json")

        if reflect.ValueOf(data).Kind() != reflect.Map {
            fmt.Println("ERROR - Invalid content:", reflect.ValueOf(data).String())
            return purchasedItems, nil
        }

        m := data.(map[string]interface{})["_embedded"].(map[string]interface{})
        q := m["purchases"].([]interface{})
        for purchase := range q {
            purchaseMap := q[purchase].(map[string]interface{})
            itemList := purchaseMap["_embedded"].(
                map[string]interface{})["items"].([]interface{})
            for index := range itemList {
                itemMap := itemList[index].(map[string]interface{})
                details := parseItemMap(itemMap)
                details["requested_device"] = dbus.MakeVariant(
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

func (iface *PayService) PurchaseItem(message dbus.Message, itemName string) (ItemDetails, *dbus.Error) {
    iface.pauseTimer()
    defer iface.resetTimer()
    packageName := packageNameFromPath(message)

    fmt.Println("DEBUG - PurchaseItem called for package:", packageName)

    // Purchase the item and return the item info and status.
    purchaseUrl := "purchase://"
    if packageName != "click-scope" {
        purchaseUrl += packageName + "/"
    }
    purchaseUrl += itemName

    // Launch the Pay UI to handle this purchase (e.g. get credit card info,
    // etc.)
    feedback := iface.launchPayUiFunction(packageName, purchaseUrl)

    // Sit here and wait for Pay UI to close. The feedback consists of two
    // channels-- Finished and Error. Finished is always closed, so we'll
    // wait for that before we check for errors (the errors are buffered).
    <-feedback.Finished

    // Now check to see if any errors occured
    err := <-feedback.Error
    if err != nil {
        return nil, dbus.NewError("org.freedesktop.DBus.Error.Failed",
            []interface{}{err.Error()})
    }

    // Pay UI has been closed without error, but we don't know if the user
    // actually made the purchase or just canceled, so we'll verify with
    // GetItem():
    return iface.GetItem(message, itemName)
}

func (iface *PayService) RefundItem(message dbus.Message, itemName string) (ItemDetails, *dbus.Error) {
    iface.pauseTimer()
    defer iface.resetTimer()

    packageName := packageNameFromPath(message)

    fmt.Println("DEBUG - RefundItem called for package:", packageName)

    if packageName != "click-scope" {
        return nil, dbus.NewError(
            "Unsupported: Refunds only supported for packages.", nil)
    }

    // To set any extra headers we need (signature, accept, etc)
    headers := make(http.Header)

    url := getPayClickUrl() + "/refunds/"
    body := `{"name": "` + packageName + `"}`
    headers.Set(ContentTypeHeader, "application/json")

    data, err := iface.getDataForUrl(url, "POST", headers, body)
    if err != nil {
        return nil, dbus.NewError(fmt.Sprintf("%s", err), nil)
    }

    // Refund the item and return the item info and status.
    item := make(map[string]dbus.Variant)

    m := data.(map[string]interface{})
    item["success"] = dbus.MakeVariant(m["success"].(bool))

    return item, nil
}

func (iface *PayService) pauseTimer() bool {
    return iface.shutdownTimer.Stop()
}

func (iface *PayService) resetTimer() bool {
    return iface.shutdownTimer.Reset(ShutdownTimeout)
}

/* Make the call to the URL and return either the data or an error
 */
func (iface *PayService) getDataForUrl(iri string, method string, headers http.Header, body string) (interface{}, error) {
    result, err := iface.client.Call(iri, method, headers, body)
    if err != nil {
        return nil, fmt.Errorf("RequestError: %s", err)
    }
    var data interface{}
    err = json.Unmarshal([]byte(result), &data)
    if err != nil {
        return nil, fmt.Errorf("ParseError: %s", err)
    }
    return data, nil
}

func parseItemMap(itemMap map[string]interface{}) (ItemDetails) {
    details := make(ItemDetails)
    for k, v := range itemMap {
        // BUG: Unfortunately golang's json parses all numbers as float64
        switch vv := v.(type) {
        case string, bool, int64, float64:
            details[k] = dbus.MakeVariant(vv)
        case nil:
            // Just ignore nulls in the JSON
        default:
            fmt.Printf("WARNING - Unable to parse item key '%s' of type '%s'.\n", k, reflect.TypeOf(vv))
        }
    }
    return details
}

/* Get the decoded packageName from a path
 */
func packageNameFromPath(message dbus.Message) (string) {
    // Get the package ID
    calledPath := DecodeDbusPath(message.Headers[dbus.FieldPath].String())
    packageName := path.Base(calledPath)

    // Strip the ending " which godbus leaves us here
    if packageName[len(packageName) - 1] == '"' {
        packageName = packageName[:len(packageName) - 1]
    }

    return packageName
}

/* Get the base URL for the API server
 */
func getPayBaseUrl() string {
    baseUrl := os.Getenv("PAY_BASE_URL")
    if baseUrl == "" {
        baseUrl = payBaseUrl
    }
    return baseUrl
}

/* Get the base URL for the Click Purchases API
 */
func getPayClickUrl() string {
    url := getPayBaseUrl() + "/api/2.0/click"
    return url
}

/* Get the base URL for the In App Purchases API
 */
func getPayInventoryUrl() string {
    url := getPayBaseUrl() + "/inventory/api/v1/packages"
    return url
}
