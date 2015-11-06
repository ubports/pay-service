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
// #include "answer_shim.h"
import "C"

import (
	"testing"
)

var answerTests = []struct {
	answer Answer
	shim   C.Answer
}{
	{AnswerDenied, C.DENIED},
	{AnswerGranted, C.GRANTED},
}

// Test typical ToShim usage.
func testAnswer_ToShim(t *testing.T) {
	for i, test := range answerTests {
		shim := C.Answer(test.answer.ToShim())

		if shim != test.shim {
			t.Errorf("Test case %d: Shim was %d, expected %d", i, shim,
				test.shim)
		}
	}
}

// Test typical AnswerFromShim usage.
func testAnswer_FromShim(t *testing.T) {
	for i, test := range answerTests {
		answer := AnswerFromShim(int(test.shim))

		if answer != test.answer {
			t.Errorf("Test case %d: Answer was %s, expected %s", i, answer,
				test.answer)
		}
	}
}
