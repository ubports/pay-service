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
)


type WebClient struct {
    client *http.Client
}

func NewWebClient() *WebClient {
    client := new(WebClient)
    client.client = http.DefaultClient

    return client
}

func (client *WebClient) Call(iri string, method string,
    headers http.Header, data string) (string, error) {

    // FIXME: Need io.ReadCloser for body, not nil
    request, err := http.NewRequest(method, iri, nil)
    if err != nil {
        return "", fmt.Errorf("Error creating request: %s", err)
    }
    request.Header = headers

    // FIXME: Sign the URL and set Authorization header
    // FIXME: Will also need to add redirect handler to re-sign new URLs
    // request.Header.Set("Authorization", signature)

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
