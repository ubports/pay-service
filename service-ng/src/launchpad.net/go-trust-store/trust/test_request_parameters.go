/* Copyright (C) 2015 Canonical Ltd.
 *
 * This file is part of go-trust-store.
 *
 * go-trust-store is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the Free Software Foundation.
 *
 * go-trust-store is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with go-trust-store. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kyle Fazzari <kyle@canonical.com>
 */

package trust

// Since cgo cannot be used within Go test files, this file holds the actual
// tests and the corresponding test file calls them (i.e. the tests contained
// within this file are not run directly, but are called from other tests).

// #include <stdlib.h>
// #include "request_parameters_shim.h"
import "C"

import (
	"testing"
	"unsafe"
)

// Test typical ToShim usage.
func testRequestParameters_ToShim(t *testing.T) {
	// First, create a trust.RequestParameters with known values
	parameters := RequestParameters{
		Application: Application{
			Uid: 1,
			Pid: 2,
			Id:  "foo",
		},
		Feature:     3,
		Description: "bar",
	}

	// Now convert it to a shim instance
	shim := (*C.RequestParameters)(parameters.ToShim())
	if shim == nil {
		t.Fatal("Shim was unexpectedly nil")
	}

	id := C.GoString(shim.application.id)
	description := C.GoString(shim.description)

	// Verify that the values are still the known values
	if shim.application.uid != 1 {
		t.Errorf("Application Uid was %d, expected 1",
			shim.application.uid)
	}

	if shim.application.pid != 2 {
		t.Errorf("Application Uid was %d, expected 2",
			shim.application.pid)
	}

	if id != "foo" {
		t.Errorf(`Application Id was "%s", expected "foo"`, id)
	}

	if shim.feature != 3 {
		t.Errorf("Feature was %d, expected 3",
			shim.feature)
	}

	if description != "bar" {
		t.Errorf(`Description was "%s", expected "bar"`, description)
	}
}

// Test typical FromShim usage.
func testRequestParameters_FromShim(t *testing.T) {
	fooCstring := C.CString("foo")
	defer C.free(unsafe.Pointer(fooCstring))

	barCstring := C.CString("bar")
	defer C.free(unsafe.Pointer(barCstring))

	// First, create shim with known values
	shim := new(C.RequestParameters)
	shim.application.uid = 1
	shim.application.pid = 2
	shim.application.id = fooCstring
	shim.feature = 3
	shim.description = barCstring

	// Now convert it to a trust.RequestParameters instance
	var parameters RequestParameters
	parameters.FromShim(unsafe.Pointer(shim))

	// Verify that the values are still the known values
	if parameters.Application.Uid != 1 {
		t.Errorf("Application Uid was %d, expected 1",
			parameters.Application.Uid)
	}

	if parameters.Application.Pid != 2 {
		t.Errorf("Application Pid was %d, expected 2",
			parameters.Application.Pid)
	}

	if parameters.Application.Id != "foo" {
		t.Errorf(`Application Id was "%s", expected "foo"`,
			parameters.Application.Id)
	}

	if parameters.Feature != 3 {
		t.Errorf("Feature was %d, expected 3",
			parameters.Feature)
	}

	if parameters.Description != "bar" {
		t.Errorf(`Description was "%s", expected "bar"`,
			parameters.Description)
	}
}
