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
    "io/ioutil"
    "net/http"
    "os"
    "strings"
)


type WebClient struct {
    client *http.Client
    auth    AuthIface
}

func NewWebClient(auth AuthIface) *WebClient {
    client := new(WebClient)
    client.auth = auth

    // FIXME: Need to add redirect handler to re-sign new URLs
    client.client = http.DefaultClient

    return client
}

func (client *WebClient) Call(iri string, method string,
    headers http.Header, data string) (string, error) {

    // Create a reader for the data string
    reader := strings.NewReader(data)
    request, err := http.NewRequest(method, iri, reader)
    if err != nil {
        return "", fmt.Errorf("Error creating request: %s", err)
    }
    request.Header = headers

    // Add the X-Device-Id header
    deviceId := client.GetDeviceId()
    if deviceId != "" {
        request.Header.Set(DeviceIdHeader, deviceId)
    }

    // Sign the request
    signature := client.auth.signUrl(iri)
    if signature != "" {
        request.Header.Set(AuthorizationHeader, signature)
    }

    // Run the request
    response, err := client.client.Do(request)
    if err != nil {
        return "", fmt.Errorf("Error in response: %s", err)
    }
    defer response.Body.Close()

    body, err := ioutil.ReadAll(response.Body)
    if err != nil {
        return "", fmt.Errorf("Error reading response body: %s", err)
    }

    return string(body), nil
}

func (client *WebClient) GetDeviceId() (string) {
    deviceId, err := ioutil.ReadFile(WhoopsieIdFile)
    if err != nil {
        fmt.Fprintln(os.Stderr, "ERROR - Failed to get device ID:", err)
        return ""
    }
    return strings.TrimSpace(string(deviceId))
}
