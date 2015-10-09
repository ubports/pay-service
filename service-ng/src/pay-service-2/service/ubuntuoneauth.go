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
    "os/exec"
    "strings"
)

const (
    // signingHelper is the path to the ubuntu-push signing-helper
    signingHelper = "/usr/lib/ubuntu-push-client/signing-helper"
)

type UbuntuOneAuth struct {
}

func (auth *UbuntuOneAuth) signUrl(iri string, method string) string {
    result, err := exec.Command(signingHelper, iri, method).Output()
    if err != nil {
        fmt.Println("ERROR - Signing URL failed. No token?")
        return ""
    }
    return strings.TrimSpace(string(result))
}
