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
// #include "request_shim.h"
import "C"

import (
	"testing"
	"time"
	"unsafe"
)

// Test typical ToShim usage.
func testRequest_ToShim(t *testing.T) {
	// First, create a trust.Request with known values
	now := time.Now()
	nowUnixSeconds := now.Unix()
	nowUnixNanoseconds := now.Nanosecond()

	request := Request{
		Answer:  AnswerGranted,
		From:    "foo",
		Feature: 1,
		When:    now,
	}

	// Now convert it to a shim instance
	shim := (*C.Request)(request.ToShim())
	if shim == nil {
		t.Fatal("Shim was unexpectedly nil")
	}

	from := C.GoString(shim.from)
	whenSeconds := int64(shim.when.seconds)
	whenNanoseconds := int(shim.when.nanoseconds)

	// Verify that the values are still the known values
	if shim.answer != C.GRANTED {
		t.Errorf("Answer was %d, expected %d", shim.answer, C.GRANTED)
	}

	if from != "foo" {
		t.Errorf(`From was "%s", expected "foo"`, from)
	}

	if shim.feature != 1 {
		t.Errorf("Feature was %d, expected 1", shim.feature)
	}

	if whenSeconds != nowUnixSeconds {
		t.Errorf("When's seconds were %d, expected %d", whenSeconds,
			nowUnixSeconds)
	}

	if whenNanoseconds != nowUnixNanoseconds {
		t.Errorf("When's nanoseconds were %d, expected %d", whenNanoseconds,
			nowUnixNanoseconds)
	}
}

// Test typical FromShim usage.
func testRequest_FromShim(t *testing.T) {
	fooCstring := C.CString("foo")
	defer C.free(unsafe.Pointer(fooCstring))

	// First, create shim with known values
	shim := new(C.Request)
	shim.answer = C.GRANTED
	shim.from = fooCstring
	shim.feature = 1

	// Getting time from Go so we don't have to worry about 32-bit times
	now := time.Now()

	shim.when.seconds = C.int64_t(now.Unix())
	shim.when.nanoseconds = C.int32_t(now.Nanosecond())

	shimSeconds := int64(shim.when.seconds)
	shimNanoseconds := int(shim.when.nanoseconds)

	// Now convert it to a trust.Request instance
	var request Request
	request.FromShim(unsafe.Pointer(shim))

	whenUnixSeconds := request.When.Unix()
	whenUnixNanoseconds := request.When.Nanosecond()

	// Verify that the values are still the known values
	if request.Answer != AnswerGranted {
		t.Errorf("Answer was %s, expected %s", request.Answer, AnswerGranted)
	}

	if request.From != "foo" {
		t.Errorf(`From was "%s", expected "foo"`, request.From)
	}

	if request.Feature != 1 {
		t.Errorf("Feature was %d, expected 1", request.Feature)
	}

	if whenUnixSeconds != shimSeconds {
		t.Errorf("When's seconds were %d, expected %d", whenUnixSeconds,
			shimSeconds)
	}

	if whenUnixNanoseconds != shimNanoseconds {
		t.Errorf("When's nanoseconds were %d, expected %d",
			whenUnixNanoseconds, shimNanoseconds)
	}
}
