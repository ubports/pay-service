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
    "net/http"
)

const (
    // HTTP header field names
    AcceptHeader        = "Accept"
    AuthorizationHeader = "Authorization"
    ContentTypeHeader   = "Content-Type"
    DeviceIdHeader      = "X-Device-Id"
)


type WebClientIface interface {
    Call(iri string, method string, headers http.Header, data string) (string, error)
    GetDeviceId() (string)
}
