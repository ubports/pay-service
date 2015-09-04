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
    "net/http"
    "net/url"
    "strings"
)


type FakeWebClient struct {
}

func (client *FakeWebClient) Call(iri string, method string,
    headers http.Header, data string) (string, error) {

    // FIXME: Will need to return fake JSONs/error results for testing
    parsed, err := url.Parse(iri)
    if err != nil {
        return "", fmt.Errorf("Error parsing URL '%s': %s", iri, err)
    }

    if strings.HasSuffix(parsed.Path, "/click/purchases/") {
        return `[
            {
                "open_id": "https://login.ubuntu.com/+id/open_id",
                "package_name": "foobar.example",
                "refundable_until": null,
                "state": "Complete"
            },
            {
                "open_id": "https://login.ubuntu.com/+id/open_id",
                "package_name": "bazbar.example",
                "refundable_until": "2099-12-31T23:59:59Z",
                "state": "Complete"
            }
        ]`, nil
    }

    return "", nil
}
