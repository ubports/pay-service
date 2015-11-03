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

// #include <stdlib.h>
// #include <sys/types.h>
// #include "request_shim.h"
import "C"

import (
	"runtime"
	"time"
	"unsafe"
)

// Feature represents a trusted-helper-specific feature to which access is
// requested via the trust store.
type Feature uint64

// Request represents a timestamped permission request and answer to be stored
// in the trust store.
type Request struct {
	Answer  Answer
	From    string
	Feature Feature
	When    time.Time
}

// FromShim converts a shim request into a Request.
func (request *Request) FromShim(shimPointer unsafe.Pointer) {
	shim := (*C.Request)(shimPointer)

	request.Answer = AnswerFromShim(int(shim.answer))
	request.From = C.GoString(shim.from)
	request.Feature = Feature(shim.feature)
	request.When = time.Unix(int64(shim.when.seconds),
		int64(shim.when.nanoseconds))
}

// ToShim converts a Request into a shim request.
func (request Request) ToShim() unsafe.Pointer {
	shim := new(C.Request)

	shim.answer = C.Answer(request.Answer.ToShim())
	shim.from = C.CString(request.From)
	shim.feature = C.uint64_t(request.Feature)
	shim.when.seconds = C.int64_t(request.When.Unix())
	shim.when.nanoseconds = C.int32_t(request.When.Nanosecond())

	runtime.SetFinalizer(shim, destroyRequestShim)

	return unsafe.Pointer(shim)
}

// destroyRequestShim frees the resources of a shim request.
func destroyRequestShim(shim *C.Request) {
	C.free(unsafe.Pointer(shim.from))
}
