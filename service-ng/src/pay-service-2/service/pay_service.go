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
    "strconv"
    "time"
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

    // Fail if calling AcknowledgeItem for click-scope
    if packageName == "click-scope" {
        return nil, dbus.NewError(
            "Unsupported: AcknowledgeItem not supported for packages.", nil)
    }


    item, err := iface.GetItem(message, itemName)
    if err != nil {
        return nil, dbus.NewError(
            fmt.Sprintf("Unable to find item '%s' for package '%s': %s",
                itemName, packageName, err), nil)
    }

    id := item["id"].Value().(int64)
    idString := fmt.Sprintf("%d", id)

    // To set any extra headers we need (signature, accept, etc)
    headers := make(http.Header)

    url := getPayInventoryUrl() + "/" + packageName + "/items/" +
        idString

    body := `{"state": "acknowledged"}`
    headers.Set("Content-Type", "application/json")

    data, neterr := iface.getDataForUrl(url, "POST", headers, body)
    if neterr != nil {
        return nil, dbus.NewError(fmt.Sprintf("%s", neterr), nil)
    }

    details := parseItemMap(data.(map[string]interface{}))
    return details, nil
}

func (iface *PayService) GetItem(message dbus.Message, itemName string) (ItemDetails, *dbus.Error) {
    iface.pauseTimer()
    defer iface.resetTimer()
    packageName := packageNameFromPath(message)

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
        if packageName == "click-scope" {
            item := make(ItemDetails)
            item["sku"] = dbus.MakeVariant(itemName)
            item["state"] = dbus.MakeVariant("available")
            item["refundable_until"] = dbus.MakeVariant(uint64(0))
            return item, nil
        }
        return nil, dbus.NewError(fmt.Sprintf("%s", err), nil)
    }

    item := parseItemMap(data.(map[string]interface{}))
    // Need to special case click-scope here as server returns 404 and {}
    // when the app is not purchased, but golang HTTP lib does not treat
    // 404 as an error.
    if packageName == "click-scope" {
        _, skuOk := item["sku"]
        if !skuOk {
            item["sku"] = dbus.MakeVariant(itemName)
            item["state"] = dbus.MakeVariant("available")
            item["refundable_until"] = dbus.MakeVariant(uint64(0))
        }
    }

    return item, nil
}

func (iface *PayService) GetPurchasedItems(message dbus.Message) ([]ItemDetails, *dbus.Error) {
    iface.pauseTimer()
    defer iface.resetTimer()

    packageName := packageNameFromPath(message)

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
            details := parseItemMap(m[index].(map[string]interface{}))
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
                details := parseItemMap(itemList[index].(map[string]interface{}))
                details["requested_device"] = dbus.MakeVariant(
                    purchaseMap["requested_device"])
                details["purchase_id"] = dbus.MakeVariant(
                    uint64(purchaseMap["id"].(float64)))

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

    if packageName != "click-scope" {
        return nil, dbus.NewError(
            "Unsupported: Refunds only supported for packages.", nil)
    }

    // To set any extra headers we need (signature, accept, etc)
    headers := make(http.Header)

    url := getPayClickUrl() + "/refunds/"
    body := `{"name": "` + itemName + `"}`
    headers.Set(ContentTypeHeader, "application/json")
    headers.Set(AcceptHeader, "application/json")

    _, err := iface.getDataForUrl(url, "POST", headers, body)
    if err != nil {
        return nil, dbus.NewError(fmt.Sprintf("%s", err), nil)
    }

    return iface.GetItem(message, itemName)
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
        switch vv := v.(type) {
        case string, bool, int64:
            details[k] = dbus.MakeVariant(vv)
        case float64:
            // BUG: golang's json module parses all numbers as float64
            if k == "id" {
                details[k] = dbus.MakeVariant(int64(vv))
            } else {
                details[k] = dbus.MakeVariant(vv)
            }
        case map[string]interface{}:
            details[k] = dbus.MakeVariant(parseItemMap(vv))
            // Get and set the "price" key too.
            if k == "prices" {
                // FIXME: Need to get a suggested currency from server.
                priceMap := details[k].Value().(ItemDetails)
                currencyCode := PreferredCurrencyCode(defaultCurrency,
                    priceMap)
                currencySymbol := SymbolForCurrency(currencyCode)
                priceString := priceMap[currencyCode].Value().(string)
                price, parseErr := strconv.ParseFloat(priceString, 32)
                if parseErr != nil {
                    fmt.Fprintf(os.Stderr,
                        "ERROR - Failed to parse price '%s': %s",
                        priceString, parseErr)
                } else {
                    details["price"] = dbus.MakeVariant(
                        CurrencyString(price, currencySymbol))
                }
            }
        case nil:
            // If refundable_until is null, set it to empty string instead
            if k == "refundable_until" {
                details[k] = dbus.MakeVariant("")
            }
        default:
            fmt.Fprintf(os.Stderr,
                "WARNING - Unable to parse item key '%s' of type '%s'.\n",
                k, reflect.ValueOf(vv).Kind().String())
        }
    }

    // Normalize the state for apps to "purchased" instead of "Complete"
    itemState, stateOk := details["state"]
    if stateOk && itemState.Value().(string) == "Complete" {
        details["state"] = dbus.MakeVariant("purchased")
    }

    // Normalize the state for apps to "available" instead of "Cancelled"
    itemState, stateOk = details["state"]
    if stateOk && itemState.Value().(string) == "Cancelled" {
        details["state"] = dbus.MakeVariant("available")
    }

    // Normalize the package_name for apps to the "sku" key
    pkgName, pkgOk := details["package_name"]
    if pkgOk {
        details["sku"] = pkgName
        delete(details, "package_name")
    }

    // Parse the acknowledged_timestamp string to a unix timestamp
    acknowledged, ackOk := details["acknowledged_timestamp"]
    if ackOk {
        if acknowledged.Value().(string) == "" {
            details["acknowledged_timestamp"] = dbus.MakeVariant(uint64(0))
        } else {
            acknowledgedTime, ackerr := time.Parse(time.RFC3339,
                acknowledged.Value().(string))
            if ackerr != nil {
                fmt.Fprintf(os.Stderr,
                    "ERROR - Unable to parse acknowledged time '%s': %s\n",
                    acknowledged, ackerr)
            } else {
                details["acknowledged_timestamp"] = dbus.MakeVariant(
                    uint64(acknowledgedTime.Unix()))
            }
        }
    }

    // Parse the completed_timestamp string to a unix timestamp
    completed, compOk := details["completed_timestamp"]
    if compOk {
        if completed.Value().(string) == "" {
            details["completed_timestamp"] = dbus.MakeVariant(uint64(0))
        } else {
            completedTime, compErr := time.Parse(time.RFC3339,
                completed.Value().(string))
            if compErr != nil {
                fmt.Fprintf(os.Stderr,
                    "ERROR - Unable to parse completed time '%s': %s\n",
                    completed, compErr)
            } else {
                details["completed_timestamp"] = dbus.MakeVariant(
                    uint64(completedTime.Unix()))
            }
        }
    }

    // Parse the refundable_until string to a unix timestamp
    refundableUntil, refundOk := details["refundable_until"]
    if refundOk {
        if refundableUntil.Value().(string) == "" {
            details["refundable_until"] = dbus.MakeVariant(uint64(0))
        } else {
            refundableTime, err := time.Parse(time.RFC3339,
                refundableUntil.Value().(string))
            if err != nil {
                fmt.Fprintf(os.Stderr,
                    "ERROR - Unable to parse refund timeout '%s': %s\n",
                    refundableUntil, err)
            } else {
                details["refundable_until"] = dbus.MakeVariant(
                    uint64(refundableTime.Unix()))
            }
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
