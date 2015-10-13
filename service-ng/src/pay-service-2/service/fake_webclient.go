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

    if parsed.Path == "/api/2.0/click/purchases/" {
        return `[
            {
                "open_id": "https://login.ubuntu.com/+id/open_id",
                "package_name": "foo.example",
                "refundable_until": null,
                "state": "Complete"
            },
            {
                "open_id": "https://login.ubuntu.com/+id/open_id",
                "package_name": "bar.example",
                "refundable_until": "2099-12-31T23:59:59Z",
                "state": "Complete"
            }
        ]`, nil
    }

    // A non-refundable app purchase
    if parsed.Path == "/api/2.0/click/purchases/foo.example/" {
        return `
            {
                "open_id": "https://login.ubuntu.com/+id/open_id",
                "package_name": "foo.example",
                "refundable_until": null,
                "state": "Complete"
            }
        `, nil
    }

    // A refundable app purchase
    if parsed.Path == "/api/2.0/click/purchases/bar.example/" {
        return `
            {
                "open_id": "https://login.ubuntu.com/+id/open_id",
                "package_name": "bar.example",
                "refundable_until": "2099-12-31T23:59:59Z",
                "state": "Complete"
            }
        `, nil
    }

    // A cancelled app purchase
    if parsed.Path == "/api/2.0/click/purchases/cancelled.example/" {
        return `
            {
                "open_id": "https://login.ubuntu.com/+id/open_id",
                "package_name": "cancelled.example",
                "refundable_until": null,
                "state": "Cancelled"
            }
        `, nil
    }

    // The refunded item should be returned as a 404
    if parsed.Path == "/api/2.0/click/purchases/foo.example/" {
        return "", fmt.Errorf("404 Not Found")
    }

    if parsed.Path == "/api/2.0/click/refunds/" && method == "POST" {
        return `{"success": true}`, nil
    }

    if parsed.Path == "/inventory/api/v1/packages/foo.example/purchases" {
        return `
        {
            "_links": {
                "self": {"href": "/packages/app.example/purchases"},
                "package": {"href": "/packages/app.example"}
            },
            "_embedded": {
                "purchases": [
                    {
                        "id": 1,
                        "requested_timestamp": "2015-03-12T14:33:23.000Z",
                        "requested_device": "device123",
                        "user_id": "user1",
                        "status": "successful",
                        "completed_timestamp": "2015-03-12T14:38:11.000Z",
                        "_links": {
                            "self": {"href": "/packages/app.example/purchases/1"},
                            "package": {"href": "/packages/app.example"}
                        },
                        "_embedded": {
                            "items": [
                                {
                                    "id": 1,
                                    "sku": "consumable",
                                    "title": "Item 1 Title",
                                    "description": "Item 1 Description",
                                    "icon": "http://example.com/icons/item1.png",
                                    "type": "consumable",
                                    "state": "approved",
                                    "_links": {
                                        "self": {"href": "/packages/app.example/items/1"}
                                    }
                                }
                            ]
                        }
                    },
                    {
                        "id": 2,
                        "requested_timestamp": "2015-03-12T14:33:23.000Z",
                        "requested_device": "device123",
                        "user_id": "user1",
                        "status": "successful",
                        "completed_timestamp": "2015-03-12T14:38:11.000Z",
                        "_links": {
                            "self": {"href": "/packages/app.example/purchases/2"},
                            "package": {"href": "/packages/app.example"}
                        },
                        "_embedded": {
                            "items": [
                                {
                                    "id": 2,
                                    "sku": "unlockable",
                                    "title": "Item 2",
                                    "description": "Item 2 Description",
                                    "icon": "http://example.com/icons/item2.png",
                                    "type": "unlockable",
                                    "state": "approved",
                                    "_links": {
                                        "self": {"href": "/packages/app.example/items/2"}
                                    }
                                }
                            ]
                        }
                    }
                ]
            }
        }`, nil
    }

    // Details for the consumable item
    if parsed.Path == "/inventory/api/v1/packages/foo.example/items/by-sku/consumable" {
        return `
        {
            "id": 1,
            "sku": "consumable",
            "title": "Item 1 Title",
            "description": "Item 1 Description",
            "icon": "http://example.com/icons/item1.png",
            "prices": {
                "USD": "1.99",
                "GBP": "0.99"
            },
            "type": "consumable",
            "state": "available",
            "_links": {
                "self": {"href": "/packages/app.example/items/1"}
            }
        }`, nil
    }

    // Details for the unlockable item
    if parsed.Path == "/inventory/api/v1/packages/foo.example/items/by-sku/unlockable" {
        return `
        {
            "id": 2,
            "sku": "unlockable",
            "title": "Item 2",
            "description": "Item 2 Description",
            "icon": "http://example.com/icons/item2.png",
            "type": "unlockable",
            "state": "approved",
            "_links": {
                "self": {"href": "/packages/app.example/items/1"}
            }
        }`, nil
    }

    // Acknowledge the consumable item
    if parsed.Path == "/inventory/api/v1/packages/foo.example/items/1" && method == "PUT" {
        return `
        {
            "id": 1,
            "sku": "consumable",
            "title": "Item 1 Title",
            "description": "Item 1 Description",
            "icon": "http://example.com/icons/item1.png",
            "type": "consumable",
            "state": "available",
            "_links": {
                "self": {"href": "/packages/app.example/items/1"}
            }
        }`, nil
    }

    // Acknowledge the unlockable item
    if parsed.Path == "/inventory/api/v1/packages/foo.example/items/2" && method == "PUT" {
        return `
        {
            "id": 2,
            "sku": "unlockable",
            "title": "Item 2",
            "description": "Item 2 Description",
            "icon": "http://example.com/icons/item2.png",
            "type": "unlockable",
            "state": "purchased",
            "_links": {
                "self": {"href": "/packages/app.example/items/2"}
            }
        }`, nil
    }

    return "", nil
}
