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
// static gchar **makeGCharArray(int size)
// {
//         return calloc(sizeof(gchar*), size);
// }
//
// static void setArrayString(gchar **array, char *str, int index)
// {
//         array[index] = str;
// }
//
// static void freeGCharArray(gchar **array, int size)
// {
//     int i;
//     for (i = 0; i < size; i++)
//     {
//         free(array[i]);
//     }
//
//     free(array);
// }
import "C"

import (
	"unsafe"

	"launchpad.net/go-mir/mir"
)

// GetPrimaryPid checks to see if an application is running and returns its main
// PID if so. If the application isn't running this will return 0.
func GetPrimaryPid(appId string) uint32 {
	appIdCstring := (*C.gchar)(C.CString(appId))
	defer C.free(unsafe.Pointer(appIdCstring))

	return uint32(C.ubuntu_app_launch_get_primary_pid(appIdCstring))
}

// StartSessionHelper starts an untrusted helper for a specific type of a given
// appid running under a Mir Trusted Prompt Session. The helper's MIR_SOCKET
// environment variable will be set appropriately so that the helper will draw
// on the correct surfaces. This function returns the generated instance ID
// or an empty string upon error.
func StartSessionHelper(helperType string, session *mir.PromptSession, appId string, uris []string) string {
	helperTypeCstring := (*C.gchar)(C.CString(helperType))
	defer C.free(unsafe.Pointer(helperTypeCstring))

	appIdCstring := (*C.gchar)(C.CString(appId))
	defer C.free(unsafe.Pointer(appIdCstring))

	// Well this is annoying.
	gcharArray := C.makeGCharArray(C.int(len(uris)))
	defer C.freeGCharArray(gcharArray, C.int(len(uris)))
	for index, uri := range uris {
		C.setArrayString(gcharArray, C.CString(uri), C.int(index))
	}

	return C.GoString((*C.char)(C.ubuntu_app_launch_start_session_helper(
		helperTypeCstring,
		(*C.struct_MirPromptSession)(session.ToMirPromptSession()),
		appIdCstring,
		gcharArray)))
}

// StopMultipleHelper asks Upstart to kill a given helper. Returns whether or
// not it was successfully stopped.
func StopMultipleHelper(helperType string, appId string, instanceId string) bool {
	helperTypeCstring := (*C.gchar)(C.CString(helperType))
	defer C.free(unsafe.Pointer(helperTypeCstring))

	appIdCstring := (*C.gchar)(C.CString(appId))
	defer C.free(unsafe.Pointer(appIdCstring))

	instanceIdCstring := (*C.gchar)(C.CString(instanceId))
	defer C.free(unsafe.Pointer(instanceIdCstring))

	return int(C.ubuntu_app_launch_stop_multiple_helper(
		helperTypeCstring,
		appIdCstring,
		instanceIdCstring)) != 0
}
