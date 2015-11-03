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
// #include "request_parameters_shim.h"
import "C"

import (
	"runtime"
	"unsafe"
)

// Application represents an application that is requesting access to a feature
// via the trust store.
type Application struct {
	Uid int    // The user under which the application is running
	Pid int    // The PID of the application
	Id  string // The appid of the application
}

// RequestParameters contains the information necessary to prompt the user
// for permission via the trust store.
type RequestParameters struct {
	Application Application // The application requesting access
	Feature     Feature     // The feature the application wants to access
	Description string      // The description to use for prompting the user
}

// FromShim converts from shim request parameters to RequestParameters.
func (parameters *RequestParameters) FromShim(shimPointer unsafe.Pointer) {
	shim := (*C.RequestParameters)(shimPointer)

	parameters.Application.Uid = int(shim.application.uid)
	parameters.Application.Pid = int(shim.application.pid)
	parameters.Application.Id = C.GoString(shim.application.id)

	parameters.Feature = Feature(shim.feature)
	parameters.Description = C.GoString(shim.description)
}

// ToShim converts RequestParameters to shim request parameters.
func (parameters RequestParameters) ToShim() unsafe.Pointer {
	shim := new(C.RequestParameters)

	shim.application.uid = C.int(parameters.Application.Uid)
	shim.application.pid = C.int(parameters.Application.Pid)
	shim.application.id = C.CString(parameters.Application.Id)

	shim.feature = C.uint64_t(parameters.Feature)
	shim.description = C.CString(parameters.Description)

	runtime.SetFinalizer(shim, destroyRequestParametersShim)

	return unsafe.Pointer(shim)
}

// destroyRequestParametersShim frees the resources of shim request
// parameters.
func destroyRequestParametersShim(shim *C.RequestParameters) {
	C.free(unsafe.Pointer(shim.application.id))
	C.free(unsafe.Pointer(shim.description))
}
