/* Copyright (C) 2015 Canonical Ltd.
 *
 * This file is part of go-ual.
 *
 * go-ual is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 3, as published by the
 * Free Software Foundation.
 *
 * go-ual is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with go-ual. If not, see <http://www.gnu.org/licenses/>.
 */

package ual

// #include <stdlib.h>
import "C"

import "unsafe"

// This exported function needs to be here since gccgo can't handle calling
// exported functions from the same source file.

//export callHelperStopObserver
func callHelperStopObserver(appId *C.char, instanceId *C.char, helperType *C.char, observerPointer unsafe.Pointer) {
	// Extract the real observer out of the user data
	observer := *(*Observer)(observerPointer)

	// Call the observer
	observer(C.GoString(appId), C.GoString(instanceId), C.GoString(helperType))
}
