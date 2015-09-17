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

// #cgo pkg-config: ubuntu-app-launch-2
// #include <stdlib.h>
// #include <ubuntu-app-launch.h>
//
// #include "gateway_observers.h"
import "C"

import (
	"fmt"
	"unsafe"
)

var helperStopObservers ObserversCollection

//export callHelperStopObserver
func callHelperStopObserver(appId *C.char, instanceId *C.char, helperType *C.char, observerPointer unsafe.Pointer) {
	// Extract the real observer out of the user data
	observer := *(*Observer)(observerPointer)

	// Call the observer
	observer(C.GoString(appId), C.GoString(instanceId), C.GoString(helperType))
}

// ObserverAddHelperStop sets up a callback to be called each time a specific
// type of helper stops.
func ObserverAddHelperStop(helperType string, observer Observer) (ObserverId, error) {
	helperTypeCstring := (*C.gchar)(C.CString(helperType))
	defer C.free(unsafe.Pointer(helperTypeCstring))

	id := helperStopObservers.AddObserver(helperType, observer)
	observerPointer, err := helperStopObservers.Observer(id)
	if err != nil {
		return 0, fmt.Errorf("Unable to retrieve observer from collection")
	}

	added := int(C.ubuntu_app_launch_observer_add_helper_stop(
		(C.UbuntuAppLaunchHelperObserver)(C.gatewayHelperStopObserver),
		helperTypeCstring,
		(C.gpointer)(observerPointer))) != 0

	if !added {
		// Make sure we remove it from our collection as well
		helperStopObservers.RemoveObserver(id)
		return 0, fmt.Errorf("Failed to add observer")
	}

	return id, nil
}

// ObserverDeleteHelperStop removes a previously-registered callback to ensure
// it no longer gets signaled.
func ObserverDeleteHelperStop(id ObserverId) error {
	observerPointer, err := helperStopObservers.Observer(id)
	if err != nil {
		return err
	}

	helperType, err := helperStopObservers.ObserverHelperType(id)
	if err != nil {
		return err
	}

	helperTypeCstring := (*C.gchar)(C.CString(helperType))
	defer C.free(unsafe.Pointer(helperTypeCstring))

	removed := int(C.ubuntu_app_launch_observer_delete_helper_stop(
		(C.UbuntuAppLaunchHelperObserver)(C.gatewayHelperStopObserver),
		helperTypeCstring,
		(C.gpointer)(observerPointer))) != 0

	if !removed {
		return fmt.Errorf("Failed to remove observer with ID %d", id)
	}

	if !helperStopObservers.RemoveObserver(id) {
		return err
	}

	return nil
}
